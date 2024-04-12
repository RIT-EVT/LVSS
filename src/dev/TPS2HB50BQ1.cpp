#include <dev/TPS2HB50BQ1.hpp>

namespace LVSS {


// TPS2HB50BQ1(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& senseOut, IO::GPIO& latch, IO::GPIO& diagEn,
//                IO::GPIO& diagSelect1, IO::GPIO& diagSelect2, IO::ADC& adc);
TPS2HB50BQ1::TPS2HB50BQ1(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& senseOut, IO::GPIO& latch, IO::GPIO& diagEn, IO::GPIO& diagSelect1, IO::GPIO& diagSelect2, IO::ADC& adc)
    : EN1(en1), EN2(en2), LATCH(latch), DIAG_EN(diagEn), DIAG_SELECT_1(diagSelect1), DIAG_SELECT_2(diagSelect2), ADC(adc) {
    setDiagnostics(DIAG_MODE::OFF);
}

void TPS2HB50BQ1::setPowerSwitchStates(bool powerSwitchOneEnabled, bool powerSwitchTwoEnabled) {
    if (powerSwitchOneEnabled) {
        EN1.writePin(IO::GPIO::State::HIGH);
    } else {
        EN1.writePin(IO::GPIO::State::LOW);
    }

    if (powerSwitchTwoEnabled) {
        EN2.writePin(IO::GPIO::State::HIGH);
    } else {
        EN2.writePin(IO::GPIO::State::LOW);
    }
}

void TPS2HB50BQ1::setDiagStateEnabled(bool state) {
    if (state) {
        DIAG_EN.writePin(IO::GPIO::State::HIGH);
    } else {
        DIAG_EN.writePin(IO::GPIO::State::LOW);
    }
}


void TPS2HB50BQ1::setLatch(LATCH_MODE mode) {
    if (mode == LATCH_MODE::LATCHED) {
        LATCH.writePin(IO::GPIO::State::LOW);
    } else if (mode == LATCH_MODE::AUTO_RETRY) {
        LATCH.writePin(IO::GPIO::State::HIGH);
    }
}

void TPS2HB50BQ1::readSenseOut(uint32_t& senseOut) {
    senseOut = ADC.readRaw();
}


void TPS2HB50BQ1::setDiagnostics(DIAG_MODE diag_mode) {
    switch (diag_mode) {
    case OFF:
        setDiagStateEnabled(false);
        DIAG_SELECT_1.writePin(IO::GPIO::State::LOW);
        DIAG_SELECT_2.writePin(IO::GPIO::State::LOW);
        break;
    case FAULT_STATUS:
        setDiagStateEnabled(false);
        DIAG_SELECT_1.writePin(IO::GPIO::State::HIGH);
        DIAG_SELECT_2.writePin(IO::GPIO::State::LOW);
        setDiagStateEnabled(true);
        break;
    case CURRENT:
        setDiagStateEnabled(false);
        DIAG_SELECT_1.writePin(IO::GPIO::State::LOW);
        DIAG_SELECT_2.writePin(IO::GPIO::State::HIGH);
        setDiagStateEnabled(true);
        break;
    case TEMP:
        setDiagStateEnabled(false);
        DIAG_SELECT_1.writePin(IO::GPIO::State::HIGH);
        DIAG_SELECT_2.writePin(IO::GPIO::State::HIGH);
        setDiagStateEnabled(true);
        break;
    }
}


void TPS2HB50BQ1::getCurrent(uint32_t& current) {
    setDiagnostics(DIAG_MODE::CURRENT);
    readSenseOut(current);
    setDiagnostics(DIAG_MODE::OFF);
}


void TPS2HB50BQ1::getTemp(uint32_t& current) {
    setDiagnostics(DIAG_MODE::TEMP);
    readSenseOut(current);
    setDiagnostics(DIAG_MODE::OFF);
}


void TPS2HB50BQ1::getFaultStatus(uint32_t& fault_status) {
    setDiagnostics(DIAG_MODE::FAULT_STATUS);
    readSenseOut(fault_status);
    setDiagnostics(DIAG_MODE::OFF);
}

}// namespace LVSS
