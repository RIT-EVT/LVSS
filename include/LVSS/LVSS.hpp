#ifndef _LVSS_
#define _LVSS_

#pragma once

#include <EVT/dev/LCD.hpp>
#include <EVT/io/CANopen.hpp>
#include <EVT/io/GPIO.hpp>
#include <EVT/io/SPI.hpp>
#include <EVT/utils/log.hpp>
#include <LVSS/LVSS.hpp>
#include <cstdio>
#include <cstring>

#include <EVT/io/ADC.hpp>

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
    LVSS(IO::GPIO& lvssEn, IO::ADC& adc0);

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

    /**
     * Handle running the core logic of the LVSS
     */
    void process();

    /**
     * Calculates current using the input voltage rail into the Vicor
     *
     * @return the current
     */
    uint32_t readCurrent(IO::ADC&);

private:
    // TODO: Figure out internal state of LVSS board
    // false = OFF, true = ON?
    // this is the power switch output (TPS2HB35)
    IO::GPIO& LVSS_EN;
    IO::ADC& ADC;
};

}// namespace LVSS
#endif