#include <dev/TPS2HB35BQ.hpp>
#include <core/utils/log.hpp>

namespace LVSS {

TPS2HB35BQ::TPS2HB35BQ(io::GPIO& en1, io::GPIO& en2, io::GPIO& latch, io::GPIO& diagEn, io::GPIO& diagSelect1,
                       io::GPIO& diagSelect2, io::ADC& adc)
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

uint16_t TPS2HB35BQ::getCurrent(Channel channelSelect) {
    uint32_t adcOutput;
    switch (channelSelect) {
    case CH1:
        setDiagnostics(DiagMode::CH1CURRENT);
        break;
    case CH2:
        setDiagnostics(DiagMode::CH2CURRENT);
        break;
    default:
        // Not a valid channel...
        core::log::LOGGER.log(core::log::Logger::LogLevel::ERROR, "Invalid channel for current diagnostics");
        return CURRENT_FAULT;
    }

    adcOutput = readSenseOut();

    setDiagnostics(DiagMode::OFF);

    /* volts = (ADC Counts * 3300 millivolts) / 4096 */
    uint32_t milliVolts = (adcOutput * adcVoltage) / adcResolution; // Turn ADC value into milli volts
    uint32_t microAmps  = (milliVolts * 1000) / rsns;               // Turn milli volts into micro amps

    // Fault range is 4mA to 5.3mA
    if (SNS_FAULT_LOWER <= microAmps && microAmps <= SNS_FAULT_UPPER) {
        return CURRENT_FAULT;
    }

    /* Current maths
     * Isns = Iout / Ksns
     * Isns = Iout / 2000
     * Isns * 2000 = Iout
     */
    uint16_t milliAmpsAtSwitch = microAmps * Ksns / 1000;

    return milliAmpsAtSwitch;
}

int16_t TPS2HB35BQ::getTemperature() {
    setDiagnostics(DiagMode::TEMP); // Set diagnostic mode to sense temperature
    uint32_t adcOut = readSenseOut();

    setDiagnostics(DiagMode::OFF);

    uint32_t milliVolts = (adcOut * adcVoltage) / adcResolution; // Turn ADC counts into milli volts
    uint32_t microAmps   = (milliVolts * 1000) / rsns; // Turn milli volts into micro amps

    // Fault range is 4mA to 5.3mA
    if (SNS_FAULT_LOWER <= microAmps && microAmps <= SNS_FAULT_UPPER) {
        return TEMP_FAULT;
    }

    /* Temp maths
     * Isns = (Tj - 25degC) x (DIsnst / dT) + 0.85
     * Isns = (Tj - 25degC) x   (0.011)   + 0.85
     * Tj = (Isns - 0.85mA) / 0.011 + 25degC */
    int32_t milliCelsius = (static_cast<int32_t>(microAmps) - 850) / dIsnstdt + resistanceOnJunctionTemp;

    return static_cast<int16_t>(milliCelsius);
}

} // namespace LVSS
