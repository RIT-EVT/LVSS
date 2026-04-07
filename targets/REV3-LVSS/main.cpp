/**
 * This is the main target for the LVSS.
 * The LVSS receives commands from the Vehicle Control Unit (VCU)
 * to manage power distribution to various boards.
 * It also transmits data back to the VCU, including current,
 * temperature, and fault status for each board and Vicor.
 */

#include <core/io/CANopen.hpp>
#include <core/io/GPIO.hpp>
#include <core/io/UART.hpp>
#include <core/io/pin.hpp>
#include <core/manager.hpp>
#include <core/utils/log.hpp>
#include <LVSS.hpp>

namespace io    = core::io;
namespace dev   = core::dev;
namespace time  = core::time;
namespace log   = core::log;
namespace types = core::types;

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
void canInterrupt(io::CANMessage& message, void* priv) {
    auto* queue = reinterpret_cast<types::FixedQueue<CANOPEN_QUEUE_SIZE, io::CANMessage>*>(priv);

    if (queue != nullptr) {
        queue->append(message);
    }
}

int main() {
    // Initialize system
    core::platform::init();

    // Setup UART
    io::UART& uart = io::getUART<io::Pin::UART_TX, io::Pin::UART_RX>(9600);
    log::LOGGER.setUART(&uart);
    log::LOGGER.setLogLevel(log::Logger::LogLevel::DEBUG);

    io::GPIO& lvssPowerSwitch0Enable0 = io::getGPIO<io::Pin::PC_14>(io::GPIO::Direction::OUTPUT); // hib
    io::GPIO& lvssPowerSwitch0Enable1 = io::getGPIO<io::Pin::PD_2>(io::GPIO::Direction::OUTPUT);  // battery
    io::GPIO& lvssPowerSwitch0Latch   = io::getGPIO<io::Pin::PC_10>(io::GPIO::Direction::OUTPUT); // latch

    io::GPIO& lvssPowerSwitch1Enable0 = io::getGPIO<io::Pin::PF_1>(io::GPIO::Direction::OUTPUT); // hudl
    io::GPIO& lvssPowerSwitch1Enable1 = io::getGPIO<io::Pin::PA_0>(io::GPIO::Direction::OUTPUT); // tms
    io::GPIO& lvssPowerSwitch1Latch   = io::getGPIO<io::Pin::PC_3>(io::GPIO::Direction::OUTPUT); // latch

    io::GPIO& lvssPowerSwitch2Enable0 = io::getGPIO<io::Pin::PA_1>(io::GPIO::Direction::OUTPUT);  // gub
    io::GPIO& lvssPowerSwitch2Enable1 = io::getGPIO<io::Pin::PC_8>(io::GPIO::Direction::OUTPUT);  // acc
    io::GPIO& lvssPowerSwitch2Latch   = io::getGPIO<io::Pin::PB_14>(io::GPIO::Direction::OUTPUT); // latch

    io::GPIO& diagEnable  = io::getGPIO<io::Pin::PC_13>(io::GPIO::Direction::OUTPUT); // diag enable
    io::GPIO& diagSelect1 = io::getGPIO<io::Pin::PC_15>(io::GPIO::Direction::OUTPUT); // diag select 1
    io::GPIO& diagSelect2 = io::getGPIO<io::Pin::PF_0>(io::GPIO::Direction::OUTPUT);  // diag select 2

    io::ADC& lvssPowerSwitch0SenseOut = io::getADC<io::Pin::PC_1>();
    io::ADC& lvssPowerSwitch1SenseOut = io::getADC<io::Pin::PC_2>();
    io::ADC& lvssPowerSwitch2SenseOut = io::getADC<io::Pin::PC_0>();

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
    dev::Timer& timer = dev::getTimer<dev::MCUTimer::Timer2>(100);

    types::FixedQueue<CANOPEN_QUEUE_SIZE, io::CANMessage> canOpenQueue;

    // Initialize CAN, add an IRQ which will add messages to the queue above
    io::CAN& can = io::getCAN<io::Pin::PA_12, io::Pin::PA_11>();
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
    io::CAN::CANStatus result = can.connect();

    // test that the board is connected to the can network
    if (result != io::CAN::CANStatus::OK) {
        log::LOGGER.log(log::Logger::LogLevel::ERROR, "Failed to connect to CAN network\r\n");
        return 1;
    } else {
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Connected to CAN network\r\n");
    }

    // Initialize all the CANOpen dev.
    io::initializeCANopenDriver(&canOpenQueue, &can, &timer, &canStackDriver, &nvmDriver, &timerDriver, &canDriver);

    // Get vicor fault signal
    io::GPIO& vicorFT = io::getGPIO<LVSS::LVSS::vicorFaultPin>(io::GPIO::Direction::INPUT);

    // setup ADC
    io::ADC& adc0 = io::getADC<io::Pin::PA_4>();

    // Create ACS71240 instance
    LVSS::ACS71240 acs71240(adc0);

    // Initialize LVSS object
    LVSS::LVSS lvss = LVSS::LVSS(powerSwitches, vicorFT, acs71240);

    // Set current and temperature limits on the power switches
    powerSwitch0.setLimits(330, 140, 9000, 135000);
    powerSwitch1.setLimits(330, 140, 9000, 135000);
    powerSwitch2.setLimits(330, 140, 9000, 135000);

    // Initialize the CANOpen node we are using.
    io::initializeCANopenNode(&canNode, &lvss, &canStackDriver, sdoBuffer, appTmrMem);

    CONmtSetMode(&canNode.Nmt, CO_OPERATIONAL);

    while (1) {
        lvss.process();
        io::processCANopenNode(&canNode);
    }
}