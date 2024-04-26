#ifndef TPS2HB50BQ1_HPP
#define TPS2HB50BQ1_HPP

#include <EVT/io/ADC.hpp>
#include <EVT/io/CAN.hpp>
#include <EVT/io/CANOpenMacros.hpp>
#include <EVT/io/CANopen.hpp>
#include <EVT/io/GPIO.hpp>

namespace IO = EVT::core::IO;

namespace LVSS {

/**
 * This class is used to control the TPS2HB50BQ1 power switch.
 * The TPS2HB50BQ1 is a high side switch that can be used to control
 * the power to up to two devices.
 *
 * The TPS2HB50BQ1 has a diagnostic mode that can be used to read the
 * current, temperature, or fault status of the device.
 */
class TPS2HB50BQ1 {
public:
    /**
     * Constructor for the TPS2HB50BQ1 class
     * @param en1 GPIO pin for first power switch
     * @param en2 GPIO pin for second power switch
     * @param latch GPIO pin for latch, see LatchMode enum for options
     * @param diagEn GPIO pin for diagnostics enable, see setDiagnostics
     * @param diagSelect1 Mux select pin for diagnostics, see setDiagnostics
     * @param diagSelect2 Mux select pin for diagnostics, see setDiagnostics
     */
    TPS2HB50BQ1(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& latch, IO::GPIO& diagEn,
                IO::GPIO& diagSelect1, IO::GPIO& diagSelect2, IO::ADC& senseOut);

    enum DiagMode {
        OFF = 0x00,
        FAULT_STATUS = 0x01,
        CURRENT = 0x02,
        TEMP = 0x03
    };

    enum LatchMode {
        LATCHED = 0x00,
        AUTO_RETRY = 0x01
    };

    void setPowerSwitchStates(bool powerSwitchOneEnabled, bool powerSwitchTwoEnabled);

    /**
     * Get the fault status of the power switch
     *
     * @return The fault status of the power switch
     */
    uint32_t getFaultStatus();

    /**
     * Get the current of the power switch
     *
     * @return The current of the power switch
     */
    uint32_t getCurrent();

    /**
     * Get the temperature of the power switch
     *
     * @return The temperature of the power switch
     */
    uint32_t getTemp();

    /**
    * Set the latch mode of the power switch
    *
    * @param mode The latch mode to set
    */
    void setLatch(LatchMode mode);

    /**
     * diagMode::OFF: Sets diagnostics pin to low, along with both diag select pins.
     * diagMode::FAULT_STATUS: Get the fault status of the power switch
     * diagMode::CURRENT: Get the current of the power switch
     * diagMode::TEMP: Get the temperature of the power switch
     */

private:
    IO::GPIO& en1;
    IO::GPIO& en2;
    IO::GPIO& latchPin;
    IO::GPIO& diagEn;
    IO::GPIO& diagSelect1;
    IO::GPIO& diagSelect2;
    IO::ADC& senseOut;

    void setDiagStateEnabled(bool state);

    /**
     * Read the sense out of the power switch
     *
     * @param senseOut The sense out value
     */
    uint32_t readSenseOut();

    /**
     * Set the diagnostic mode
     * See the diagMode enum for the different modes
     *
     * @param diag_mode The diagnostic mode to set
     */
    void setDiagnostics(DiagMode diag_mode);
};

}// namespace LVSS
#endif
