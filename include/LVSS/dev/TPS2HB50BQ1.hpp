// Driver for power switch
#ifndef LVSS_TPS2HB50BQ1_HPP
#define LVSS_TPS2HB50BQ1_HPP

#include "EVT/io/GPIO.hpp"
#include <EVT/io/CAN.hpp>
#include <EVT/io/CANOpenMacros.hpp>
#include <EVT/io/CANopen.hpp>

namespace IO = EVT::core::IO;

namespace LVSS {

/**
 * Power switch for LVSS
 * /datasheets/TPS2HB35-Q1 Power Switch Data Sheet.pdf
 */
class TPS2HB50BQ1 {
public:
    TPS2HB50BQ1(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& senseOut, IO::GPIO& latch, IO::GPIO& diagEn, IO::GPIO& diagSelect1, IO::GPIO& diagSelect2)
        : EN1(en1), EN2(en2), SENSE_OUT(senseOut), LATCH(latch), DIAG_EN(diagEn), DIAG_SELECT_1(diagSelect1), DIAG_SELECT_2(diagSelect2) {
    }

    /**
     * Enable all power switches
     */
    void enableAll() {
        EN1.writePin(IO::GPIO::State::HIGH);
        EN2.writePin(IO::GPIO::State::HIGH);
    }

    /**
     * Disable all power switches
     */
    void disableAll() {
        EN1.writePin(IO::GPIO::State::LOW);
        EN2.writePin(IO::GPIO::State::LOW);
    }

    /**
     * Enable the first power switch
     */
    void enable1() {
        EN1.writePin(IO::GPIO::State::HIGH);
    }

    void disable1() {
        EN1.writePin(IO::GPIO::State::LOW);
    }

    void enable2() {
        EN2.writePin(IO::GPIO::State::HIGH);
    }

    void disable2() {
        EN2.writePin(IO::GPIO::State::LOW);
    }


private:
    IO::GPIO& EN1;
    IO::GPIO& EN2;
    IO::GPIO& SENSE_OUT;
    IO::GPIO& LATCH;
    IO::GPIO& DIAG_EN;
    IO::GPIO& DIAG_SELECT_1;
    IO::GPIO& DIAG_SELECT_2;

    void enableDiag() {
        DIAG_EN.writePin(IO::GPIO::State::HIGH);
    }

    void disableDiag() {
        DIAG_EN.writePin(IO::GPIO::State::LOW);
    }

    void selectDiag1() {
        DIAG_SELECT_1.writePin(IO::GPIO::State::HIGH);
        DIAG_SELECT_2.writePin(IO::GPIO::State::LOW);
    }

    void selectDiag2() {
        DIAG_SELECT_1.writePin(IO::GPIO::State::LOW);
        DIAG_SELECT_2.writePin(IO::GPIO::State::HIGH);
    }
};

}
#endif//LVSS_TPS2HB50BQ1_HPP
