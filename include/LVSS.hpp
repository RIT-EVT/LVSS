#ifndef _LVSS_
#define _LVSS_

#include "dev/ACS781XLR.hpp"
#include <EVT/dev/LCD.hpp>
#include <EVT/io/CANopen.hpp>
#include <EVT/io/GPIO.hpp>
#include <EVT/io/SPI.hpp>
#include <EVT/utils/log.hpp>
#include <LVSS.hpp>
#include <dev/TPS2HB50BQ1.hpp>
#include <cstdio>
#include <cstring>

namespace IO = EVT::core::IO;
namespace DEV = EVT::core::DEV;

namespace LVSS{

static constexpr uint8_t POWER_SWITCHES_SIZE = 3;

/**
 * This is an example of a class for a board
 */
class LVSS : public CANDevice {
public:

    static constexpr uint8_t NODE_ID = 42;


    /**
     * Placeholder constructor for the LVSS class
     */
    LVSS(TPS2HB50BQ1 powerSwitches[POWER_SWITCHES_SIZE], ACS781XLR currentSensor);
    explicit LVSS(TPS2HB50BQ1* powerSwitches);

    CO_OBJ_T* getObjectDictionary() override;

    /**
     * Gets the size of the Object Dictionary
     *
     * @return size of the Object Dictionary
     */
    uint8_t getNumElements() override;

    /**
    * Get the device's node ID
    *
    * @return The node ID of the can device.
     */
    uint8_t getNodeID() override;

private:
    // TODO: Figure out internal state of LVSS board
    // false = OFF, true = ON?

    TPS2HB50BQ1* PowerSwitches; // a struct for each power switch (of which there are 3)
    ACS781XLR currentSensor;
};

} // namespace LVSS
#endif