#ifndef _LVSS_
#define _LVSS_

#include <EVT/dev/LCD.hpp>
#include <EVT/io/CANopen.hpp>
#include <EVT/io/GPIO.hpp>
#include <EVT/io/SPI.hpp>
#include <EVT/utils/log.hpp>
#include <LVSS.hpp>
#include <dev/ACS71240.hpp>

namespace IO = EVT::core::IO;
namespace DEV = EVT::core::DEV;
namespace log = EVT::core::log;

namespace LVSS {

/**
 * This is an example of a class for a board
 */
class LVSS : public CANDevice {
public:
    static constexpr uint8_t NODE_ID = 42;

    /**
     * Placeholder constructor for the LVSS class
     */
    LVSS(IO::GPIO& lvssEn);

    CO_OBJ_T* getObjectDictionary() override;

    uint8_t getNumElements() override;

    uint8_t getNodeID() override;

    /**
     * Handle running the core logic of the LVSS
     */
    void process();

private:
    // TODO: Figure out internal state of LVSS board
    // false = OFF, true = ON?
    // this is the power switch output (TPS2HB35)
    IO::GPIO& LVSS_EN;
};

}// namespace LVSS
#endif