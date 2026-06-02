#include <dev/TPS2HB35BQ.hpp>

namespace LVSS {

TPS2HB35BQ::TPS2HB35BQ(io::GPIO& en1, io::GPIO& en2, io::GPIO& latch, io::GPIO& diagEn, io::GPIO& diagSelect1,
                       io::GPIO& diagSelect2, io::ADC& adc, uint32_t rsns)
    : en1(en1), en2(en2), latchPin(latch), diagEn(diagEn), diagSelect1(diagSelect1), diagSelect2(diagSelect2),
      senseOut(adc) {
    setDiagnostics(DiagMode::OFF);
}

void TPS2HB35BQ::setPowerSwitchStates(bool powerSwitchOneEnabled, bool powerSwitchTwoEnabled) {
    if (powerSwitchOneEnabled) {
        en1.writePin(io::GPIO::State::HIGH);
    } else {
        en1.writePin(io::GPIO::State::LOW);
    }

    if (powerSwitchTwoEnabled) {
        en2.writePin(io::GPIO::State::HIGH);
    } else {
        en2.writePin(io::GPIO::State::LOW);
    }
}

void TPS2HB35BQ::setDiagStateEnabled(bool state) {
    if (state) {
        diagEn.writePin(io::GPIO::State::HIGH);
    } else {
        diagEn.writePin(io::GPIO::State::LOW);
    }
}

void TPS2HB35BQ::setLatch(LatchMode mode) {
    if (mode == LatchMode::LATCHED) {
        latchPin.writePin(io::GPIO::State::HIGH);
    } else if (mode == LatchMode::AUTO_RETRY) {
        latchPin.writePin(io::GPIO::State::LOW);
    }
}

uint32_t TPS2HB35BQ::readSenseOut() {
    return senseOut.readRaw();
}

void TPS2HB35BQ::setDiagnostics(DiagMode diag_mode) {
    switch (diag_mode) {
    case OFF:
        setDiagStateEnabled(false);
        diagSelect1.writePin(io::GPIO::State::LOW);
        diagSelect2.writePin(io::GPIO::State::LOW);
        break;
    case FAULT_STATUS:
        setDiagStateEnabled(true);
        diagSelect1.writePin(io::GPIO::State::LOW);
        diagSelect2.writePin(io::GPIO::State::HIGH);
        break;
    case CH1CURRENT:
        setDiagStateEnabled(true);
        diagSelect1.writePin(io::GPIO::State::LOW);
        diagSelect2.writePin(io::GPIO::State::LOW);
        break;
    case CH2CURRENT:
        setDiagStateEnabled(true);
        diagSelect1.writePin(io::GPIO::State::LOW);
        diagSelect2.writePin(io::GPIO::State::HIGH);
        break;
    case TEMP:
        setDiagStateEnabled(true);
        diagSelect1.writePin(io::GPIO::State::HIGH);
        diagSelect2.writePin(io::GPIO::State::LOW);
        break;
    }
}

uint16_t TPS2HB35BQ::getCurrentAndFault(uint8_t channelSelect) {
    if (channelSelect == 1) {
        setDiagnostics(DiagMode::CH1CURRENT); // Set diagnostic mode to sense current
    } else {
        setDiagnostics(DiagMode::CH2CURRENT); // Set diagnostic mode to sense current
    }

    counts = readSenseOut();

    if (counts >= 4095) {
        setDiagnostics(DiagMode::OFF);
    }

    /* volts = (ADC Counts * 3300 kilovolts) / 4096 */
    uint32_t milliVolts = (counts * adcVoltage) / adcResolution; // Turn ADC counts into milli volts
    uint32_t microAmps  = (milliVolts * 1000) / rsns;            // Turn milli volts into micro amps

    /* If current is greater than the current limit latch the sns pin */
    if (microAmps >= icl) {
        setDiagnostics(DiagMode::OFF);
        setLatch(LatchMode::LATCHED); // Latch power switches
        return CURRENT_FAULT;
    }

    return static_cast<uint16_t>(microAmps);
}

int16_t TPS2HB35BQ::getTemperature() {
    setDiagnostics(DiagMode::TEMP); // Set diagnostic mode to sense temperature
    counts = readSenseOut();

    if (counts >= 4095) {
        setDiagnostics(DiagMode::OFF);
        setLatch(LatchMode::LATCHED); // Latch power switches
    }

    uint32_t milliVolts = (counts * adcVoltage) / adcResolution; // Turn ADC counts into milli volts
    int32_t microAmps   = (static_cast<int32_t>(milliVolts) * 1000) / rsns;            // Turn milli volts into micro amps

    /* ( Isns (mA) - 0.85 mA ) / (dIsnst/dT) + 25 celsius */
    int32_t milliCelsius =
        (microAmps - 850) / dIsnst + resistanceOnJunctionTemp; // Returns temperature in milli celsius

    if (milliCelsius >= TemperatureLimit) {
        return TEMP_FAULT;
    }

    return static_cast<int16_t>(milliCelsius);
}

void TPS2HB35BQ::setLimits(uint32_t ohms, uint32_t ratio, uint32_t milliamps, int32_t millicelsius) {
    rsns             = ohms;
    kcl              = ratio;
    icl              = milliamps;
    TemperatureLimit = millicelsius;
}

} // namespace LVSS
