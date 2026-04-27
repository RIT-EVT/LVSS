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
               io::GPIO& diagSelect2, io::ADC& senseOut, uint32_t rsns = 330);

    /**
     * diagMode::OFF: Sets diagnostics pin  low, along with both diag select pins.
     * diagMode::FAULT_STATUS: Get the fault status of the power switch
     * diagMode::CH1CURRENT: Get the channel 1 current of the power switch
     * diagMode::CH2CURRENT: Get the channel 2 current of the power switch
     * diagMode::TEMP: Get the temperature of the power switch
     */
    enum DiagMode {
        OFF          = 0x00,
        FAULT_STATUS = 0x01,
        CH1CURRENT   = 0x02,
        CH2CURRENT   = 0x03,
        TEMP         = 0x04
    };

    /**
     * Sets the latch mode for the power switches
     */
    enum LatchMode {
        LATCHED    = 0x00, // If there is a fault the power switch will shut off and stay off
        AUTO_RETRY = 0x01  // If there is a fault the power switch will shut off and try to turn back on until there is
                           // no longer a fault
    };

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
     * If a fault is detected an INTMAX_MIN is returned
     *
     * @param channelSelect The channel to sense the current from
     * @return The current of the power switch in milli amps
     */
    uint32_t getCurrentAndFault(uint8_t channelSelect);

    /**
     * Get the temperature of the power switch.
     *
     * @return The temperature of the power switch in millicelcius
     */
    int32_t getTemperature();

    /**
     * Set the latch mode of the power switch.
     *
     * @param mode The latch mode to set
     */
    void setLatch(LatchMode mode);

    /**
     * Set the limits for the power switches at which they shut down.
     *
     * @param ohms The resistance value of the sense resistor
     * @param ratio The current sense ratio
     * @param milliamps The current limit
     * @param millicelsius The temperature limit
     */
    void setLimits(uint32_t ohms, uint32_t ratio, uint32_t milliamps, int32_t millicelsius);

private:
    io::GPIO& en1;
    io::GPIO& en2;
    io::GPIO& latchPin;
    io::GPIO& diagEn;
    io::GPIO& diagSelect1;
    io::GPIO& diagSelect2;
    io::ADC& senseOut;

    uint32_t counts          = 0;      // ADC Counts
    uint32_t rsns            = 360;    // Resistor that sets the current limit
    uint32_t kcl             = 140;    // Current Limit Ratio
    uint32_t icl             = 9000;   // Current Limit Value in milliamps
    int32_t TemperatureLimit = 135000; // Temperature Limit of 135 C in millicelsius

    static constexpr uint32_t adcVoltage               = 3300;  // ADC Voltage
    static constexpr uint32_t dIsnst                   = 11;    // Coefficient 0.011 mA/C in microamps
    static constexpr uint32_t resistanceOnJunctionTemp = 25000; // 25 C in millicelsius
    static constexpr uint32_t adcResolution            = 4096;  // 25 C in millicelsius

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
