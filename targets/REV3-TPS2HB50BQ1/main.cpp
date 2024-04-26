/**
 * This is a basic sample of using the UART module. The program provides a
 * basic echo functionality where the uart will write back whatever the user
 * enters.
 */

#include <EVT/io/CANopen.hpp>
#include <EVT/io/GPIO.hpp>
#include <EVT/io/UART.hpp>
#include <EVT/io/pin.hpp>
#include <EVT/manager.hpp>
#include <EVT/utils/log.hpp>
#include <EVT/utils/time.hpp>
#include <LVSS.hpp>

namespace IO = EVT::core::IO;
namespace DEV = EVT::core::DEV;
namespace time = EVT::core::time;
namespace log = EVT::core::log;
namespace types = EVT::core::types;
using namespace std;

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

    // PC14 is a LSE output, so we can't use it as a GPIO, so for testing we'll use PB_7
    IO::GPIO& lvssPowerSwitch0Enable1 = IO::getGPIO<IO::Pin::PB_7>(IO::GPIO::Direction::OUTPUT); // hib

    IO::GPIO& lvssPowerSwitch0Enable2 = IO::getGPIO<IO::Pin::PD_2>(IO::GPIO::Direction::OUTPUT); // battery
    IO::GPIO& lvssPowerSwitch0Latch = IO::getGPIO<IO::Pin::PC_10>(IO::GPIO::Direction::OUTPUT); // latch

//    IO::GPIO& lvssPowerSwitch1Enable1 = IO::getGPIO<IO::Pin::PF_1>(IO::GPIO::Direction::OUTPUT); // hudl
//    IO::GPIO& lvssPowerSwitch1Enable2 = IO::getGPIO<IO::Pin::PA_0>(IO::GPIO::Direction::OUTPUT); // tms
//    IO::GPIO& lvssPowerSwitch1Latch = IO::getGPIO<IO::Pin::PC_3>(IO::GPIO::Direction::OUTPUT); // latch
//
//    IO::GPIO& lvssPowerSwitch2Enable0 = IO::getGPIO<IO::Pin::PA_1>(IO::GPIO::Direction::OUTPUT); // gub
//    IO::GPIO& lvssPowerSwitch2Enable1 = IO::getGPIO<IO::Pin::PC_8>(IO::GPIO::Direction::OUTPUT); // acc
//    IO::GPIO& lvssPowerSwitch2Latch = IO::getGPIO<IO::Pin::PB_14>(IO::GPIO::Direction::OUTPUT); // latch

    IO::GPIO& diagEnable = IO::getGPIO<IO::Pin::PC_13>(IO::GPIO::Direction::OUTPUT); // diag enable

    // PC15 is ALSO a LSE output, so we can't use it as a GPIO, so for testing we'll use PC_3
    IO::GPIO& diagSelect1 = IO::getGPIO<IO::Pin::PC_3>(IO::GPIO::Direction::OUTPUT); // diag select 1

    // PF_0 is a HSE, or high speed clock output so we use PC_4 instead
    IO::GPIO& diagSelect2 = IO::getGPIO<IO::Pin::PC_4>(IO::GPIO::Direction::OUTPUT); // diag select 2

    IO::ADC& lvssPowerSwitch0SenseOut = IO::getADC<IO::Pin::PC_0>();
//    IO::ADC& lvssPowerSwitch1SenseOut = IO::getADC<IO::Pin::PC_1>();
//    IO::ADC& lvssPowerSwitch2SenseOut = IO::getADC<IO::Pin::PC_2>();

    LVSS::TPS2HB50BQ1 powerSwitch0 = LVSS::TPS2HB50BQ1(lvssPowerSwitch0Enable1, lvssPowerSwitch0Enable2,
                                                       lvssPowerSwitch0Latch, diagEnable,
                                                       diagSelect1, diagSelect2,
                                                       lvssPowerSwitch0SenseOut);


    // initialize timer? probably don't need
    DEV::Timerf3xx timer(TIM2, 160);

    // String to store user input
    char buf[1000];

    const char* commands[10] = {
        "help: Display this help message\r\n",
        "latch: Set latch mode\r\n",
        "autoretry: Set auto retry mode\r\n",
        "en1: Enable power switch 1\r\n",
        "en2: Enable power switch 2\r\n",
        "enAll: Enable all power switches\r\n",
        "disAll: Disable all power switches\r\n",
        "temp: Get temperature\r\n",
        "current: Get current\r\n",
        "fault: Get fault status\r\n"};

    while (1) {
        // Read user input
        uart.printf("\nEnter command: ");
        uart.gets(buf, 100);

        if (strcmp(buf, "help") == 0) {
            uart.printf("\r\n");
            for (auto & command : commands) {
                uart.printf("%s", command);
            }
        }
        else if (strcmp(buf, "latch") == 0) {
            uart.printf("\r\nSetting latch\r\n");
            powerSwitch0.setLatch(LVSS::TPS2HB50BQ1::LatchMode::LATCHED);
        }
        else if (strcmp(buf, "autoretry") == 0) {
            uart.printf("\r\nSetting auto retry\r\n");
            powerSwitch0.setLatch(LVSS::TPS2HB50BQ1::LatchMode::AUTO_RETRY);
        }
        else if (strcmp(buf, "enAll") == 0) {
            uart.printf("\r\nEnabling all power switches\r\n");
            powerSwitch0.setPowerSwitchStates(true, true);
        }
        else if (strcmp(buf, "disAll") == 0) {
            uart.printf("\r\nDisabling all power switches\r\n");
            powerSwitch0.setPowerSwitchStates(false, false);
        }
        else if (strcmp(buf, "en1") == 0) {
            uart.printf("\r\nEnabling power switch 1\r\n");
            powerSwitch0.setPowerSwitchStates(true, false);
        }
        else if (strcmp(buf, "en2") == 0) {
            uart.printf("\r\nEnabling power switch 2\r\n");
            powerSwitch0.setPowerSwitchStates(false, true);
        }
        else if (strcmp(buf, "temp") == 0) {
            uart.printf("\r\ntemp: %d\r\n", powerSwitch0.getTemp());
        }
        else if (strcmp(buf, "current") == 0) {
            uart.printf("\r\nCurrent: %d\r\n", powerSwitch0.getCurrent());
        }
        else if (strcmp(buf, "fault") == 0) {
            uart.printf("\r\nFault state: %d\r\n", powerSwitch0.getFaultStatus());
        }
        else {
            uart.printf("\r\nInvalid command\r\n");
            for (auto & command : commands) {
                uart.printf("%s", command);
            }
        }
        uart.printf("\r\n");
        // clear buffer
        memset(buf, 0, sizeof(buf));
    }
}