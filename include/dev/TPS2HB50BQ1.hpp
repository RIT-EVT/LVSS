// Driver for power switch
#ifndef LVSS_TPS2HB50BQ1_HPP
#define LVSS_TPS2HB50BQ1_HPP

#include <EVT/io/ADC.hpp>
#include <EVT/io/CAN.hpp>
#include <EVT/io/CANOpenMacros.hpp>
#include <EVT/io/CANopen.hpp>
#include <EVT/io/GPIO.hpp>

// latch mode consts
#define LATCH_MODE_LATCHED 0x00
#define LATCH_MODE_AUTO_RETRY 0x01

// diagnostic mode consts
#define DIAG_MODE_OFF 0x00
#define DIAG_MODE_FAULT_STATUS 0x01
#define DIAG_MODE_CURRENT 0x02
#define DIAG_MODE_TEMP 0x03

namespace IO = EVT::core::IO;

namespace LVSS {

/**
 * Power switch for LVSS
 * /datasheets/TPS2HB35-Q1 Power Switch Data Sheet.pdf
 */
class TPS2HB50BQ1 {
public:
    TPS2HB50BQ1(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& senseOut, IO::GPIO& latch, IO::GPIO& diagEn,
                IO::GPIO& diagSelect1, IO::GPIO& diagSelect2, IO::ADC& adc, uint8_t current_diag_mode);
    void enableAll();

    void disableAll();

    void enable1();

    void disable1();

    void enable2();

    void disable2();

private:
    enum DIAG_MODE {
        OFF = 0x00,
        FAULT_STATUS = 0x01,
        CURRENT = 0x02,
        TEMP = 0x03
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

    void enableDiag();

    void disableDiag();

    // should pass in LATCH_MODE_LATCHED or LATCH_MODE_AUTO_RETRY
    void setLatch(uint8_t mode);

    void readSenseOut(uint32_t& senseOut);

    void setDiagnostics(enum DIAG_MODE diag_mode);

    uint32_t getCurrent();
    uint32_t getTemp();
    uint32_t getFaultStatus();
};

}// namespace LVSS
#endif//LVSS_TPS2HB50BQ1_HPP
