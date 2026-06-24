/**
 * This is a basic sample to show the functionality of the LVSS power switches.
 */

#include <core/io/CANopen.hpp>
#include <core/io/GPIO.hpp>
#include <core/io/UART.hpp>
#include <core/io/pin.hpp>
#include <core/manager.hpp>
#include <core/utils/log.hpp>

#include <LVSS.hpp>
#include <dev/TPS2HB35BQ.hpp>

namespace io    = core::io;
namespace dev   = core::dev;
namespace time  = core::time;
namespace log   = core::log;
namespace types = core::types;
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
    log::LOGGER.setLogLevel(log::Logger::LogLevel::INFO);


    io::ADC& lvssPowerSwitch0SenseOut = io::getADC<io::Pin::PC_1>();
    io::ADC& lvssPowerSwitch1SenseOut = io::getADC<io::Pin::PC_2>();
    io::ADC& lvssPowerSwitch2SenseOut = io::getADC<io::Pin::PC_0>();


    io::GPIO& diagEnable  = io::getGPIO<io::Pin::PC_13>(io::GPIO::Direction::OUTPUT); // diag enable
    io::GPIO& diagSelect1 = io::getGPIO<io::Pin::PC_15>(io::GPIO::Direction::OUTPUT); // diag select 1
    io::GPIO& diagSelect2 = io::getGPIO<io::Pin::PF_0>(io::GPIO::Direction::OUTPUT);  // diag select 2

    io::GPIO& lvssPowerSwitch0Enable0 = io::getGPIO<io::Pin::PC_14>(io::GPIO::Direction::OUTPUT); // hib
    io::GPIO& lvssPowerSwitch0Enable1 = io::getGPIO<io::Pin::PD_2>(io::GPIO::Direction::OUTPUT);  // battery
    io::GPIO& lvssPowerSwitch0Latch   = io::getGPIO<io::Pin::PC_10>(io::GPIO::Direction::OUTPUT); // latch

    LVSS::TPS2HB35BQ powerSwitch0 = LVSS::TPS2HB35BQ(lvssPowerSwitch0Enable0,
                                                     lvssPowerSwitch0Enable1,
                                                     lvssPowerSwitch0Latch,
                                                     diagEnable,
                                                     diagSelect1,
                                                     diagSelect2,
                                                     lvssPowerSwitch0SenseOut);

    io::GPIO& lvssPowerSwitch1Enable0 = io::getGPIO<io::Pin::PF_1>(io::GPIO::Direction::OUTPUT); // hudl
    io::GPIO& lvssPowerSwitch1Enable1 = io::getGPIO<io::Pin::PA_0>(io::GPIO::Direction::OUTPUT); // tms
    io::GPIO& lvssPowerSwitch1Latch   = io::getGPIO<io::Pin::PC_3>(io::GPIO::Direction::OUTPUT); // latch

    LVSS::TPS2HB35BQ powerSwitch1 = LVSS::TPS2HB35BQ(lvssPowerSwitch1Enable0,
                                                     lvssPowerSwitch1Enable1,
                                                     lvssPowerSwitch1Latch,
                                                     diagEnable,
                                                     diagSelect1,
                                                     diagSelect2,
                                                     lvssPowerSwitch1SenseOut);

    powerSwitch1.setPowerSwitchStates(false, false);

    io::GPIO& lvssPowerSwitch2Enable0 = io::getGPIO<io::Pin::PA_1>(io::GPIO::Direction::OUTPUT);  // gub
    io::GPIO& lvssPowerSwitch2Enable1 = io::getGPIO<io::Pin::PC_8>(io::GPIO::Direction::OUTPUT);  // acc
    io::GPIO& lvssPowerSwitch2Latch   = io::getGPIO<io::Pin::PB_14>(io::GPIO::Direction::OUTPUT); // latch

    LVSS::TPS2HB35BQ powerSwitch2 = LVSS::TPS2HB35BQ(lvssPowerSwitch2Enable0,
                                                     lvssPowerSwitch2Enable1,
                                                     lvssPowerSwitch2Latch,
                                                     diagEnable,
                                                     diagSelect1,
                                                     diagSelect2,
                                                     lvssPowerSwitch2SenseOut);

    powerSwitch2.setPowerSwitchStates(false, false);

    // String to store user input
    char buf[1000];

    const char* commands[10] = {"help: Display this help message\r\n",
                                "latch: Set latch mode\r\n",
                                "autoretry: Set auto retry mode\r\n",
                                "en1: Enable power switch 1\r\n",
                                "en2: Enable power switch 2\r\n",
                                "enAll: Enable all power switches\r\n",
                                "disAll: Disable all power switches\r\n",
                                "temp: Get temperature\r\n",
                                "current: Get current\r\n",
                                "fault: Get fault status\r\n"};

    // Display available commands at startup
    uart.printf("\r\nAvailable commands:\r\n");
    for (const char* command : commands) {
        uart.printf("%s", command);
    }

    while (1) {
        // Read user input
        uart.printf("\nEnter command: ");
        uart.gets(buf, 100);

        if (strcmp(buf, "help") == 0) {
            uart.printf("\r\n");
            for (auto& command : commands) {
                uart.printf("%s", command);
            }
        } else if (strcmp(buf, "latch") == 0) {
            uart.printf("\r\nSetting latch\r\n");
            powerSwitch0.setLatch(LVSS::TPS2HB35BQ::LatchMode::LATCHED);
        } else if (strcmp(buf, "autoretry") == 0) {
            uart.printf("\r\nSetting auto retry\r\n");
            powerSwitch0.setLatch(LVSS::TPS2HB35BQ::LatchMode::AUTO_RETRY);
        } else if (strcmp(buf, "enAll") == 0) {
            uart.printf("\r\nEnabling all power switches\r\n");
            powerSwitch0.setPowerSwitchStates(true, true);
        } else if (strcmp(buf, "disAll") == 0) {
            uart.printf("\r\nDisabling all power switches\r\n");
            powerSwitch0.setPowerSwitchStates(false, false);
        } else if (strcmp(buf, "en1") == 0) {
            uart.printf("\r\nEnabling power switch 1\r\n");
            powerSwitch0.setPowerSwitchStates(true, false);
        } else if (strcmp(buf, "en2") == 0) {
            uart.printf("\r\nEnabling power switch 2\r\n");
            powerSwitch0.setPowerSwitchStates(false, true);
        } else if (strcmp(buf, "temp") == 0) {
            uart.printf("\r\ntemp: %d\r\n", powerSwitch0.getTemperature());
        } else if (strcmp(buf, "current") == 0) {
            uart.printf("\r\nCurrent: %u\r\n", powerSwitch0.getCurrent(LVSS::TPS2HB35BQ::Channel::CH1));
        } else {
            uart.printf("\r\nInvalid command\r\n");
            for (auto& command : commands) {
                uart.printf("%s", command);
            }
        }
        uart.printf("\r\n");
        // clear buffer
        memset(buf, 0, sizeof(buf));
    }
}