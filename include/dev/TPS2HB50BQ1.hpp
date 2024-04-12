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
    TPS2HB50BQ1(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& senseOut, IO::GPIO& latch, IO::GPIO& diagEn,
                IO::GPIO& diagSelect1, IO::GPIO& diagSelect2, IO::ADC& adc);

    void setPowerSwitchStates(bool powerSwitchOneEnabled, bool powerSwitchTwoEnabled);

    /**
     * Get the fault status of the power switch
     *
     * @return The fault status of the power switch
     */
    void getFaultStatus(uint32_t& temp);

    /**
     * Get the current of the power switch
     *
     * @return The current of the power switch
     */
    void getCurrent(uint32_t& temp);

    /**
     * Get the temperature of the power switch
     *
     * @return The temperature of the power switch
     */
    void getTemp(uint32_t& temp);

    /**
     * diagMode::OFF: Sets diagnostics pin to low, along with both diag select pins.
     * diagMode::FAULT_STATUS: Get the fault status of the power switch
     * diagMode::CURRENT: Get the current of the power switch
     * diagMode::TEMP: Get the temperature of the power switch
     */
    enum DiagMode {
        Off = 0x00,
        FaultStatus = 0x01,
        Current = 0x02,
        Temp = 0x03
    };

private:
    enum LatchMode {
        Latched = 0x00,
        AutoRetry = 0x01
    };

    IO::GPIO& EN1;
    IO::GPIO& EN2;
    IO::GPIO& LATCH;
    IO::GPIO& DIAG_EN;
    IO::GPIO& DIAG_SELECT_1;
    IO::GPIO& DIAG_SELECT_2;
    IO::ADC& ADC;

    void setDiagStateEnabled(bool state);

    /**
    * Set the latch mode of the power switch
     *
    * @param mode The latch mode to set
    */
    void setLatch(LatchMode mode);

    /**
     * Read the sense out of the power switch
     *
     * @param senseOut The sense out value
     */
    void readSenseOut(uint32_t& senseOut);

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
