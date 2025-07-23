#ifndef TPS2HB35BQ_HPP
#define TPS2HB35BQ_HPP

#include <EVT/io/ADC.hpp>
#include <EVT/io/GPIO.hpp>

namespace IO = EVT::core::IO;

namespace LVSS {

/**
 * This class is used to control the TPS2HB35BQ power switch.
 * The TPS2HB35BQ is a high side switch that can be used to control
 * the power to up to two devices.
 *
 * The TPS2HB35BQ has a diagnostic mode that can be used to read the
 * current, temperature, or fault status of the device and set an
 * auto retry latch.
 */
class TPS2HB35BQ {
public:
    /**
     * Constructor for the TPS2HB35BQ class
     * @param en1 GPIO pin for first power switch
     * @param en2 GPIO pin for second power switch
     * @param latch GPIO pin for latch, see LatchMode enum for options
     * @param diagEn GPIO pin for diagnostics enable, see setDiagnostics
     * @param diagSelect1 Mux select pin for diagnostics, see setDiagnostics
     * @param diagSelect2 Mux select pin for diagnostics, see setDiagnostics
     */
    TPS2HB35BQ(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& latch, IO::GPIO& diagEn, IO::GPIO& diagSelect1,
               IO::GPIO& diagSelect2, IO::ADC& senseOut, uint32_t rsns = 330);

    enum DiagMode {
        OFF          = 0x00,
        FAULT_STATUS = 0x01,
        CH1CURRENT   = 0x02,
        CH2CURRENT   = 0x03,
        TEMP         = 0x04
    };

    enum LatchMode {
        LATCHED    = 0x00,
        AUTO_RETRY = 0x01
    };

    void setPowerSwitchStates(bool powerSwitchOneEnabled, bool powerSwitchTwoEnabled);

    /**
     * Get the current of the power switch
     *
     * @return The current of the power switch in milli amps
     */
    uint32_t getCurrent(uint8_t channelSelect);

    /**
     * Get the temperature of the power switch.
     * A fault is detected if the temperature >= 300 C
     *
     * @return The temperature of the power switch in Celsius
     */
    int32_t getTempandFault();

    /**
     * Set the latch mode of the power switch
     *
     * @param mode The latch mode to set
     */
    void setLatch(LatchMode mode);

    /**
     * Set the limits for the power switches
     *
     * @param ohms The resistance value of the sense resistor
     * @param ratio The current sense ratio
     * @param milliamps The current limit
     * @param millicelsius The temperature limit
     */
    void setLimits(uint32_t ohms, uint32_t ratio, uint32_t milliamps, int32_t millicelsius);

    /**
     * diagMode::OFF: Sets diagnostics pin  low, along with both diag select pins.
     * diagMode::FAULT_STATUS: Get the fault status of the power switch
     * diagMode::CH1CURRENT: Get the channel 1 current of the power switch
     * diagMode::CH2CURRENT: Get the channel 2 current of the power switch
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

    /** ADC Counts */
    uint32_t counts = 0;

    uint32_t rsns          = 330;    // Resistor that sets the current limit
    uint32_t kcl           = 140;    // Current Limit Ratio
    uint32_t icl           = 9000;   // Current Limit Value in milliamps
    int32_t TemperatureLim = 135000; // Temperature Limit of 135 millicelsius

    /**
     * Controls the diagnostic enable pin
     *
     * @param state Selects the function of the SNS pin
     */
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

} // namespace LVSS
#endif
