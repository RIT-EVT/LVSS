#include <dev/TPS2HB35BQ.hpp>

namespace LVSS {

TPS2HB35BQ::TPS2HB35BQ(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& latch, IO::GPIO& diagEn, IO::GPIO& diagSelect1, IO::GPIO& diagSelect2, IO::ADC& adc)
    : en1(en1), en2(en2), latchPin(latch), diagEn(diagEn), diagSelect1(diagSelect1), diagSelect2(diagSelect2), senseOut(adc) {
    setDiagnostics(DiagMode::OFF);
}

void TPS2HB35BQ::setPowerSwitchStates(bool powerSwitchOneEnabled, bool powerSwitchTwoEnabled) {
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

void TPS2HB35BQ::setDiagStateEnabled(bool state) {
    if (state) {
        diagEn.writePin(IO::GPIO::State::HIGH);
    } else {
        diagEn.writePin(IO::GPIO::State::LOW);
    }
}

void TPS2HB35BQ::setLatch(LatchMode mode) {
    if (mode == LatchMode::LATCHED) {
        latchPin.writePin(IO::GPIO::State::LOW);
    } else if (mode == LatchMode::AUTO_RETRY) {
        latchPin.writePin(IO::GPIO::State::HIGH);
    }
}

uint32_t TPS2HB35BQ::readSenseOut() {
    return senseOut.readRaw();
}

void TPS2HB35BQ::setDiagnostics(DiagMode diag_mode) {
    switch (diag_mode) {
    case OFF:
        setDiagStateEnabled(false);
        diagSelect1.writePin(IO::GPIO::State::LOW);
        diagSelect2.writePin(IO::GPIO::State::LOW);
        break;
    case FAULT_STATUS:
        setDiagStateEnabled(true);
        diagSelect1.writePin(IO::GPIO::State::HIGH);
        diagSelect2.writePin(IO::GPIO::State::LOW);
        break;
    case CURRENT:
        setDiagStateEnabled(true);
        diagSelect1.writePin(IO::GPIO::State::LOW);
        diagSelect2.writePin(IO::GPIO::State::HIGH);
        break;
    case TEMP:
        setDiagStateEnabled(true);
        diagSelect1.writePin(IO::GPIO::State::HIGH);
        diagSelect2.writePin(IO::GPIO::State::LOW);
        break;
    }
}

uint32_t TPS2HB35BQ::getCurrent() {
    setDiagnostics(DiagMode::CURRENT);
    counts = readSenseOut();

    if (counts > 4000) {
        setDiagnostics(DiagMode::OFF);
    }

    // volts = (ADC Counts * 3300 kilovolts) / 4096
    volts = (counts * 3300) / 4096;// Turn ADC counts into voltage
    current = volts * 2;           // Current in nanoamps

    if (current >= icl) {
        setDiagnostics(DiagMode::OFF);
        setLatch(LatchMode::LATCHED);// Latch power switches
    }

    return counts;
}

uint32_t TPS2HB35BQ::getTempandFault() {
    setDiagnostics(DiagMode::TEMP);
    counts = readSenseOut();

    if (counts >= 4095) {
        setDiagnostics(DiagMode::OFF);
        setLatch(LatchMode::LATCHED);// Latch power switches
    }

    volts = (counts * 3300) / 4096;// Turn ADC counts into voltage

    // Since voltage is in millivolts divided by a 1k ohm resistor, and we want milliamps this is implicitly divided by 1
    temp = (1000 * volts - 575000) / 11;

    return temp;
}

}// namespace LVSS
