// Driver for power switch
#ifndef LVSS_TPS2HB50BQ1_HPP
#define LVSS_TPS2HB50BQ1_HPP

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
                IO::GPIO& diagSelect1, IO::GPIO& diagSelect2, IO::ADC& adc, uint8_t current_diag_mode);

    /**
     * Enable both power switches
     */
    void enableAll();

    /**
     * Disable both power switches
     */
    void disableAll();

    /**
     * Enable power switch 1
     */
    void enable_ps_one();

    /**
     * Disable power switch 1
     */
    void disable_ps_one();

    /**
     * Enable power switch 2
     */
    void enable_ps_two();

    /**
     * Disable power switch 2
     */
    void disable_ps_two();

private:
    enum DIAG_MODE {
        OFF = 0x00,
        FAULT_STATUS = 0x01,
        CURRENT = 0x02,
        TEMP = 0x03
    };

    enum LATCH_MODE {
        LATCHED = 0x00,
        AUTO_RETRY = 0x01
    };

    IO::GPIO& EN1;
    IO::GPIO& EN2;
    IO::GPIO& SENSE_OUT;
    IO::GPIO& LATCH;
    IO::GPIO& DIAG_EN;
    IO::GPIO& DIAG_SELECT_1;
    IO::GPIO& DIAG_SELECT_2;
    IO::ADC& ADC;

    DIAG_MODE current_diag_mode;
    LATCH_MODE current_latch_mode;

    /**
     * Enable the diagnostic mode
     */
    void enableDiag();

    /**
     * Disable the diagnostic mode
     */
    void disableDiag();

    /**
    * Set the latch mode of the power switch
    * @param mode The latch mode to set
    */
    void setLatch(LATCH_MODE mode);

    /**
     * Read the sense out of the power switch
     * @param senseOut The sense out value
     */
    void readSenseOut(uint32_t& senseOut);

    /**
     * Set the diagnostic mode
     * Diagnostics modes include:
     * DIAG_MODE::OFF: Sets diagnostics pin to low, along with both diag select pins.
     * DIAG_MODE::FAULT_STATUS: Get the fault status of the power switch
     * DIAG_MODE::CURRENT: Get the current of the power switch
     * DIAG_MODE::TEMP: Get the temperature of the power switch
     * @param diag_mode The diagnostic mode to set
     */
    void setDiagnostics(enum DIAG_MODE diag_mode);

    /**
     * Get the current of the power switch
     * @return The current of the power switch
     */
    uint32_t getCurrent();

    /**
     * Get the temperature of the power switch
     * @return The temperature of the power switch
     */
    uint32_t getTemp();

    /**
     * Get the fault status of the power switch
     * @return The fault status of the power switch
     */
    uint32_t getFaultStatus();
};

}// namespace LVSS
#endif//LVSS_TPS2HB50BQ1_HPP
