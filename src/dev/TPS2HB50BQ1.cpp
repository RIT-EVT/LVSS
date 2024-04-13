#include <dev/TPS2HB50BQ1.hpp>

namespace LVSS {


TPS2HB50BQ1::TPS2HB50BQ1(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& latch, IO::GPIO& diagEn, IO::GPIO& diagSelect1, IO::GPIO& diagSelect2, IO::ADC& adc)
    : en1(en1), en2(en2), latchPin(latch), diagEn(diagEn), diagSelect1(diagSelect1), diagSelect2(diagSelect2), senseOut(adc) {
    setDiagnostics(DiagMode::OFF);
}

void TPS2HB50BQ1::setPowerSwitchStates(bool powerSwitchOneEnabled, bool powerSwitchTwoEnabled) {
    if (powerSwitchOneEnabled) {
        en1.writePin(IO::GPIO::State::HIGH);
    } else {
        en1.writePin(IO::GPIO::State::LOW);
    }

    if (powerSwitchTwoEnabled) {
        en2.writePin(IO::GPIO::State::HIGH);
    } else {
        en2.writePin(IO::GPIO::State::LOW);
    }
}

void TPS2HB50BQ1::setDiagStateEnabled(bool state) {
    if (state) {
        diagEn.writePin(IO::GPIO::State::HIGH);
    } else {
        diagEn.writePin(IO::GPIO::State::LOW);
    }
}

void TPS2HB50BQ1::setLatch(LatchMode mode) {
    if (mode == LatchMode::LATCHED) {
        latchPin.writePin(IO::GPIO::State::LOW);
    } else if (mode == LatchMode::AUTO_RETRY) {
        latchPin.writePin(IO::GPIO::State::HIGH);
    }
}

uint32_t TPS2HB50BQ1::readSenseOut() {
    return senseOut.readRaw();
}

void TPS2HB50BQ1::setDiagnostics(DiagMode diag_mode) {
    switch (diag_mode) {
    case OFF:
        setDiagStateEnabled(false);
        diagSelect1.writePin(IO::GPIO::State::LOW);
        diagSelect2.writePin(IO::GPIO::State::LOW);
        break;
    case FAULT_STATUS:
        setDiagStateEnabled(false);
        diagSelect1.writePin(IO::GPIO::State::HIGH);
        diagSelect2.writePin(IO::GPIO::State::LOW);
        setDiagStateEnabled(true);
        break;
    case CURRENT:
        setDiagStateEnabled(false);
        diagSelect1.writePin(IO::GPIO::State::LOW);
        diagSelect2.writePin(IO::GPIO::State::HIGH);
        setDiagStateEnabled(true);
        break;
    case TEMP:
        setDiagStateEnabled(false);
        diagSelect1.writePin(IO::GPIO::State::HIGH);
        diagSelect2.writePin(IO::GPIO::State::HIGH);
        setDiagStateEnabled(true);
        break;
    }
}

uint32_t TPS2HB50BQ1::getCurrent() {
    setDiagnostics(DiagMode::CURRENT);
    uint32_t current = readSenseOut();
    setDiagnostics(DiagMode::OFF);

    // TODO: more processing on raw adc senseOut pin output

    return current;
}

uint32_t TPS2HB50BQ1::getTemp() {
    setDiagnostics(DiagMode::TEMP);
    uint32_t temp = readSenseOut();
    setDiagnostics(DiagMode::OFF);

    // TODO: more processing on raw adc senseOut pin output

    return temp;
}

uint32_t TPS2HB50BQ1::getFaultStatus() {
    setDiagnostics(DiagMode::FAULT_STATUS);
    uint32_t fault_status = readSenseOut();
    setDiagnostics(DiagMode::OFF);

    // TODO: more processing on raw adc senseOut pin output

    return fault_status;
}

}// namespace LVSS
