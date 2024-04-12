#include <dev/TPS2HB50BQ1.hpp>

namespace LVSS {

// TPS2HB50BQ1(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& senseOut, IO::GPIO& latch, IO::GPIO& diagEn,
//                IO::GPIO& diagSelect1, IO::GPIO& diagSelect2, IO::ADC& adc);
TPS2HB50BQ1::TPS2HB50BQ1(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& senseOut, IO::GPIO& latch, IO::GPIO& diagEn, IO::GPIO& diagSelect1, IO::GPIO& diagSelect2, IO::ADC& adc)
    : EN1(en1), EN2(en2), LATCH(latch), DIAG_EN(diagEn), DIAG_SELECT_1(diagSelect1), DIAG_SELECT_2(diagSelect2), ADC(adc) {
    setDiagnostics(DiagMode::Off);
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

void TPS2HB50BQ1::setLatch(LatchMode mode) {
    if (mode == LatchMode::Latched) {
        LATCH.writePin(IO::GPIO::State::LOW);
    } else if (mode == LatchMode::AutoRetry) {
        LATCH.writePin(IO::GPIO::State::HIGH);
    }
}

uint32_t TPS2HB50BQ1::readSenseOut() {
    return ADC.readRaw();
}

void TPS2HB50BQ1::setDiagnostics(DiagMode diag_mode) {
    switch (diag_mode) {
    case Off:
        setDiagStateEnabled(false);
        DIAG_SELECT_1.writePin(IO::GPIO::State::LOW);
        DIAG_SELECT_2.writePin(IO::GPIO::State::LOW);
        break;
    case FaultStatus:
        setDiagStateEnabled(false);
        DIAG_SELECT_1.writePin(IO::GPIO::State::HIGH);
        DIAG_SELECT_2.writePin(IO::GPIO::State::LOW);
        setDiagStateEnabled(true);
        break;
    case Current:
        setDiagStateEnabled(false);
        DIAG_SELECT_1.writePin(IO::GPIO::State::LOW);
        DIAG_SELECT_2.writePin(IO::GPIO::State::HIGH);
        setDiagStateEnabled(true);
        break;
    case Temp:
        setDiagStateEnabled(false);
        DIAG_SELECT_1.writePin(IO::GPIO::State::HIGH);
        DIAG_SELECT_2.writePin(IO::GPIO::State::HIGH);
        setDiagStateEnabled(true);
        break;
    }
}

uint32_t TPS2HB50BQ1::getCurrent() {
    setDiagnostics(DiagMode::Current);
    uint32_t current = readSenseOut();
    setDiagnostics(DiagMode::Off);

    // TODO: more processing on raw ADC output

    return current;
}

uint32_t TPS2HB50BQ1::getTemp() {
    setDiagnostics(DiagMode::Temp);
    uint32_t temp = readSenseOut();
    setDiagnostics(DiagMode::Off);

    // TODO: more processing on raw ADC output

    return temp;
}

uint32_t TPS2HB50BQ1::getFaultStatus() {
    setDiagnostics(DiagMode::FaultStatus);
    uint32_t fault_status = readSenseOut();
    setDiagnostics(DiagMode::Off);

    // TODO: more processing on raw ADC output

    return fault_status;
}

}// namespace LVSS
