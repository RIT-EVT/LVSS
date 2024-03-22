/**
 * This is a basic sample of using the UART module. The program provides a
 * basic echo functionality where the uart will write back whatever the user
 * enters.
 */

#include "LVSS.hpp"
#include <EVT/io/CANopen.hpp>
#include <EVT/io/GPIO.hpp>
#include <EVT/io/UART.hpp>
#include <EVT/io/pin.hpp>
#include <EVT/manager.hpp>
#include <EVT/utils/log.hpp>
#include <EVT/utils/time.hpp>

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
    auto* queue = (types::FixedQueue<CANOPEN_QUEUE_SIZE, IO::CANMessage>*) priv;

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

    IO::GPIO& lvssEn0 = IO::getGPIO<IO::Pin::PA_10>(IO::GPIO::Direction::INPUT);
    IO::GPIO& lvssEn1 = IO::getGPIO<IO::Pin::PA_9>(IO::GPIO::Direction::INPUT);
    IO::GPIO& lvssEn2 = IO::getGPIO<IO::Pin::PA_8>(IO::GPIO::Direction::INPUT);

    DEV::Timerf3xx timer(TIM2, 160);

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

//    // TODO(?) Adds CAN filtering to only allow messages from IDs 1, 5, and 8.
//    can.addCANFilter(0x1, 0b00001111111, 0);
//    can.addCANFilter(0x8, 0b00001111111, 1);
//    can.addCANFilter(0x5, 0b00001111111, 2);

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

    // LVSS::LVSS lvss = LVSS::LVSS(lvssEn0, lvssEn1, lvssEn2);

    // Initialize the CANOpen node we are using.
    // IO::initializeCANopenNode(&canNode, &lvss, &canStackDriver, sdoBuffer, appTmrMem);

    // String to store user input
    char buf[100];

    while (1) {
        // Read user input
        uart.printf("Enter message: ");
        uart.gets(buf, 100);
        uart.printf("\n\recho: %s\n\r", buf);
    }
}
