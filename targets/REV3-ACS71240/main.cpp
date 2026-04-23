/**
 * This is a basic sample to show the current sensing feature of the
 * LVSS using the ACS71240 IC.
 */

#include <core/io/UART.hpp>
#include <core/io/pin.hpp>
#include <core/manager.hpp>
#include <core/utils/log.hpp>
#include <core/utils/time.hpp>

#include <LVSS.hpp>

namespace IO   = core::io;
namespace DEV  = core::dev;
namespace time = core::time;
namespace log  = core::log;

int main() {
    // Initialize system
    core::platform::init();

    // Setup UART
    IO::UART& uart = IO::getUART<IO::Pin::UART_TX, IO::Pin::UART_RX>(9600);
    log::LOGGER.setUART(&uart);
    log::LOGGER.setLogLevel(log::Logger::LogLevel::INFO);

    // ADC
    IO::ADC& adc0 = IO::getADC<IO::Pin::PA_1>();

    // Create ACS71240 instance
    LVSS::ACS71240 acs71240(adc0);

    while (1) {
        uart.printf("ADC0: %dmA\r\n", acs71240.readCurrent());
        time::wait(500);
    }
}
