/**
 * This is the main target for the LVSS.
 * The LVSS receives commands from the Vehicle Control Unit (VCU)
 * to manage power distribution to various boards.
 * It also transmits data back to the VCU, including current,
 * temperature, and fault status for each board and Vicor.
 */

#include <EVT/io/CANopen.hpp>
#include <EVT/io/GPIO.hpp>
#include <EVT/io/UART.hpp>
#include <EVT/io/pin.hpp>
#include <EVT/manager.hpp>
#include <EVT/utils/log.hpp>
#include <LVSS.hpp>

namespace IO    = EVT::core::IO;
namespace DEV   = EVT::core::DEV;
namespace time  = EVT::core::time;
namespace log   = EVT::core::log;
namespace types = EVT::core::types;

/**
 * Interrupt handler to get CAN messages. A function pointer to this function
 * will be passed to the EVT-core CAN interface which will in turn call this
 * function each time a new CAN message comes in.
 *
 * NOTE: For this sample, every non-extended (so 11  bit CAN IDs) will be
 * assumed to be intended to be passed as a CANopen message.
 *
 * @param message[in] The passed in CAN message that was read.
 */
// create a can interrupt handler
void canInterrupt(IO::CANMessage& message, void* priv) {
    auto* queue = reinterpret_cast<types::FixedQueue<CANOPEN_QUEUE_SIZE, IO::CANMessage>*>(priv);

    if (queue != nullptr) {
        queue->append(message);
    }
}

int main() {
    // Initialize system
    EVT::core::platform::init();

    // Setup UART
    IO::UART& uart = IO::getUART<IO::Pin::UART_TX, IO::Pin::UART_RX>(9600);
    log::LOGGER.setUART(&uart);
    log::LOGGER.setLogLevel(log::Logger::LogLevel::INFO);

    IO::GPIO& lvssPowerSwitch0Enable0 = IO::getGPIO<IO::Pin::PC_14>(IO::GPIO::Direction::OUTPUT); // hib
    IO::GPIO& lvssPowerSwitch0Enable1 = IO::getGPIO<IO::Pin::PD_2>(IO::GPIO::Direction::OUTPUT);  // battery
    IO::GPIO& lvssPowerSwitch0Latch   = IO::getGPIO<IO::Pin::PC_10>(IO::GPIO::Direction::OUTPUT); // latch

    IO::GPIO& lvssPowerSwitch1Enable0 = IO::getGPIO<IO::Pin::PF_1>(IO::GPIO::Direction::OUTPUT); // hudl
    IO::GPIO& lvssPowerSwitch1Enable1 = IO::getGPIO<IO::Pin::PA_0>(IO::GPIO::Direction::OUTPUT); // tms
    IO::GPIO& lvssPowerSwitch1Latch   = IO::getGPIO<IO::Pin::PC_3>(IO::GPIO::Direction::OUTPUT); // latch

    IO::GPIO& lvssPowerSwitch2Enable0 = IO::getGPIO<IO::Pin::PA_1>(IO::GPIO::Direction::OUTPUT);  // gub
    IO::GPIO& lvssPowerSwitch2Enable1 = IO::getGPIO<IO::Pin::PC_8>(IO::GPIO::Direction::OUTPUT);  // acc
    IO::GPIO& lvssPowerSwitch2Latch   = IO::getGPIO<IO::Pin::PB_14>(IO::GPIO::Direction::OUTPUT); // latch

    IO::GPIO& diagEnable  = IO::getGPIO<IO::Pin::PC_13>(IO::GPIO::Direction::OUTPUT); // diag enable
    IO::GPIO& diagSelect1 = IO::getGPIO<IO::Pin::PC_15>(IO::GPIO::Direction::OUTPUT); // diag select 1
    IO::GPIO& diagSelect2 = IO::getGPIO<IO::Pin::PF_0>(IO::GPIO::Direction::OUTPUT);  // diag select 2

    IO::ADC& lvssPowerSwitch0SenseOut = IO::getADC<IO::Pin::PC_0>();
    IO::ADC& lvssPowerSwitch1SenseOut = IO::getADC<IO::Pin::PC_1>();
    IO::ADC& lvssPowerSwitch2SenseOut = IO::getADC<IO::Pin::PC_2>();

    LVSS::TPS2HB35BQ powerSwitch0 = LVSS::TPS2HB35BQ(lvssPowerSwitch0Enable0,
                                                     lvssPowerSwitch0Enable1,
                                                     lvssPowerSwitch0Latch,
                                                     diagEnable,
                                                     diagSelect1,
                                                     diagSelect2,
                                                     lvssPowerSwitch0SenseOut);

    LVSS::TPS2HB35BQ powerSwitch1 = LVSS::TPS2HB35BQ(lvssPowerSwitch1Enable0,
                                                     lvssPowerSwitch1Enable1,
                                                     lvssPowerSwitch1Latch,
                                                     diagEnable,
                                                     diagSelect1,
                                                     diagSelect2,
                                                     lvssPowerSwitch1SenseOut);

    LVSS::TPS2HB35BQ powerSwitch2 = LVSS::TPS2HB35BQ(lvssPowerSwitch2Enable0,
                                                     lvssPowerSwitch2Enable1,
                                                     lvssPowerSwitch2Latch,
                                                     diagEnable,
                                                     diagSelect1,
                                                     diagSelect2,
                                                     lvssPowerSwitch2SenseOut);

    LVSS::TPS2HB35BQ* powerSwitches[3] = {&powerSwitch0, &powerSwitch1, &powerSwitch2};

    // initialize timer? probably don't need
    DEV::Timerf3xx timer(TIM2, 100);

    types::FixedQueue<CANOPEN_QUEUE_SIZE, IO::CANMessage> canOpenQueue;

    // Initialize CAN, add an IRQ which will add messages to the queue above
    IO::CAN& can = IO::getCAN<IO::Pin::PA_12, IO::Pin::PA_11>();
    can.addIRQHandler(canInterrupt, reinterpret_cast<void*>(&canOpenQueue));

    // Reserved memory for CANopen stack usage
    uint8_t sdoBuffer[CO_SSDO_N * CO_SDO_BUF_BYTE];
    CO_TMR_MEM appTmrMem[16];

    // Reserve CAN dev
    CO_IF_DRV canStackDriver;
    CO_IF_CAN_DRV canDriver;
    CO_IF_TIMER_DRV timerDriver;
    CO_IF_NVM_DRV nvmDriver;

    // Reserve canNode
    CO_NODE canNode;

    // Attempt to join the CAN network
    IO::CAN::CANStatus result = can.connect();

    // test that the board is connected to the can network
    if (result != IO::CAN::CANStatus::OK) {
        log::LOGGER.log(log::Logger::LogLevel::ERROR, "Failed to connect to CAN network\r\n");
        return 1;
    } else {
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Connected to CAN network\r\n");
    }

    // Initialize all the CANOpen dev.
    IO::initializeCANopenDriver(&canOpenQueue, &can, &timer, &canStackDriver, &nvmDriver, &timerDriver, &canDriver);

    // Get vicor fault signal
    IO::GPIO& vicorFT = IO::getGPIO<LVSS::LVSS::vicorFaultPin>(IO::GPIO::Direction::INPUT);

    // setup ADC
    IO::ADC& adc0 = IO::getADC<IO::Pin::PA_4>();

    // Create ACS71240 instance
    LVSS::ACS71240 acs71240(adc0);

    // Initialize LVSS object
    LVSS::LVSS lvss = LVSS::LVSS(powerSwitches, vicorFT, acs71240);

    // Set current and temperature limits on the power switches
    powerSwitch0.setLimits(330, 140, 9000, 135000);
    powerSwitch1.setLimits(330, 140, 9000, 135000);
    powerSwitch2.setLimits(330, 140, 9000, 135000);

    // Initialize the CANOpen node we are using.
    IO::initializeCANopenNode(&canNode, &lvss, &canStackDriver, sdoBuffer, appTmrMem);

    CONmtSetMode(&canNode.Nmt, CO_OPERATIONAL);

    while (1) {
        lvss.process();
        IO::processCANopenNode(&canNode);
    }
}