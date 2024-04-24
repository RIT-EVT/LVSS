/**
 * This is a basic sample to show how the current sensing feature of the
 * LVSS using the ACS71240 IC
 */

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

int main() {
    // Initialize system
    EVT::core::platform::init();

    // Setup UART
    IO::UART& uart = IO::getUART<IO::Pin::UART_TX, IO::Pin::UART_RX>(9600);
    log::LOGGER.setUART(&uart);
    log::LOGGER.setLogLevel(log::Logger::LogLevel::INFO);

    //ADC
    IO::ADC& adc0 = IO::getADC<IO::Pin::PA_1>();

    //Create ACS71240 instance
    LVSS::ACS71240 acs71240(adc0);

    while (1) {
        uart.printf("ADC0: %dmA\r\n", acs71240.readCurrent());
        time::wait(500);
    }
}
