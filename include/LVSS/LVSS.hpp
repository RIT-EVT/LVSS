#ifndef _LVSS_
#define _LVSS_

#pragma once

#include <EVT/io/CANopen.hpp>
#include <EVT/dev/LCD.hpp>
#include <EVT/io/GPIO.hpp>
#include <EVT/io/SPI.hpp>
#include <EVT/utils/log.hpp>
#include <LVSS/LVSS.hpp>
#include <cstdio>
#include <cstring>

namespace IO = EVT::core::IO;
namespace DEV = EVT::core::DEV;

namespace LVSS{

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
    // this is the power switch output (TPS2HB35)
    IO::GPIO& LVSS_EN;


};

} // namespace LVSS
#endif