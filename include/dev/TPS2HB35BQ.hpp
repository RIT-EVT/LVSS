#ifndef TPS2HB35BQ_HPP
#define TPS2HB35BQ_HPP

#include <core/io/ADC.hpp>
#include <core/io/GPIO.hpp>

namespace io = core::io;

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
    enum Channel {
        CH1,
        CH2
    };

    /**
     * Constructor for the TPS2HB35BQ class
     * @param en1 GPIO pin for first power switch
     * @param en2 GPIO pin for second power switch
     * @param latch GPIO pin for latch, see LatchMode enum for options
     * @param diagEn GPIO pin for diagnostics enable, see setDiagnostics
     * @param diagSelect1 Mux select pin for diagnostics, see setDiagnostics
     * @param diagSelect2 Mux select pin for diagnostics, see setDiagnostics
     */
    TPS2HB35BQ(io::GPIO& en1, io::GPIO& en2, io::GPIO& latch, io::GPIO& diagEn, io::GPIO& diagSelect1,
               io::GPIO& diagSelect2, io::ADC& senseOut);

    /**
     * diagMode::OFF: Sets diagnostics pin  low, along with both diag select pins.
     * diagMode::CH1CURRENT: Get the channel 1 current of the power switch
     * diagMode::CH2CURRENT: Get the channel 2 current of the power switch
     * diagMode::TEMP: Get the temperature of the power switch
     */
    enum DiagMode {
        OFF          = 0x00,
        CH1CURRENT   = 0x01,
        CH2CURRENT   = 0x02,
        TEMP         = 0x03
    };

    /**
     * Sets the latch mode for the power switches
     */
    enum LatchMode {
        LATCHED    = 0x00, // If there is a fault the power switch will shut off and stay off
        AUTO_RETRY = 0x01  // If there is a fault the power switch will shut off and try to turn back on until there is
                           // no longer a fault
    };


    static constexpr uint16_t CURRENT_FAULT = UINT16_MAX;
    static constexpr int16_t TEMP_FAULT = INT16_MIN;

    /**
     * Sets the power switch channel output to high or low.
     *
     * @param powerSwitchOneEnabled State of power switch channel 1
     * @param powerSwitchTwoEnabled State of power switch channel 2
     */
    void setPowerSwitchStates(bool powerSwitchOneEnabled, bool powerSwitchTwoEnabled);

    /**
     * Get the current of the power switch.
     * A fault is detected if the current is somewhere between 4 and 5.3 mA.
     * If a fault is detected CURRENT_FAULT is returned
     *
     * @param channelSelect The channel to sense the current from
     * @return The current of the power switch in milli amps
     */
    uint16_t getCurrent(Channel channelSelect);

    /**
     * Get the temperature of the power switch.
    * A fault is detected if the current is somewhere between 4 and 5.3 mA.
    * If a fault is detected TEMP_FAULT is returned
     *
     * @return The temperature of the power switch in millicelcius
     */
    int16_t getTemperature();

    /**
     * Set the latch mode of the power switch.
     *
     * @param mode The latch mode to set
     */
    void setLatch(LatchMode mode);

    /**
     * Clears current fault from the given switch.
     *
     * Fault conditions (enters FAULT state, SNS outputs 4-5.3 mA):
     *  - Overcurrent       -> switch disabled (depends on switch version; immediate on A/B, at thermal limit on C)
     *  - Thermal shutdown  -> switch disabled (>150C absolute, or >60/80C relative)
     *
     * Other conditions (do NOT enter fault state):
     *  - Undervoltage      -> switch disabled, no SNS indication
     *  - Loss of GND       -> both switches disabled
     *  - Reverse Battery   -> switch is force enabled, protection (thermal shutdown, etc) unavailable
     *  - Open Load         -> diagnostic indicator on SNS only, switch unaffected
     *  - Short to Battery  -> diagnostic indicator on SNS only, switch unaffected
     *
     * To clear a fault two things must happen:
     *  - Latch pin must be low (AUTO_RETRY mode)
     *  - Temperature must be within allowable range
     *
     * The power switch handles checking these, this function simply sets the Latch pin to AUTO_RETRY momentarily
     *  to see if that is all the power switch was waiting for.
     * @return True if fault cleared successfully, else false
     */
    bool clearCurrentFault();

private:
    io::GPIO& en1;
    io::GPIO& en2;
    io::GPIO& latchPin;
    io::GPIO& diagEn;
    io::GPIO& diagSelect1;
    io::GPIO& diagSelect2;
    io::ADC& senseOut;

    static constexpr uint32_t SNS_FAULT_LOWER          = 4000;  // Minimum SNS value to mean a fault in microamps
    static constexpr uint32_t SNS_FAULT_UPPER          = 5300;  // Maximum SNS value to mean a fault in microamps
    static constexpr uint32_t rsns                     = 360;   // Resistor that sets the current limit
    static constexpr uint32_t Ksns                     = 2000;  // Current sense ratio
    static constexpr uint32_t adcVoltage               = 3300;  // ADC Voltage
    static constexpr uint16_t dIsnstdt                 = 11;    // Coefficient 0.011 mA/C in microamps
    static constexpr uint16_t resistanceOnJunctionTemp = 25000; // 25 C in millicelsius
    static constexpr uint32_t adcResolution            = 4096;  // Max ADC value

    /**
     * Controls the diagnostic enable pin
     *
     * @param state Selects the function of the SNS pin
     */
    void setDiagStateEnabled(bool state);

    /**
     * Read the sense out (raw ADC value) of the power switch
     *
     * @return senseOut The sense out value
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
