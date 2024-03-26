#include <dev/TPS2HB50BQ1.hpp>

namespace LVSS {

TPS2HB50BQ1::TPS2HB50BQ1(IO::GPIO& en1, IO::GPIO& en2, IO::GPIO& senseOut, IO::GPIO& latch, IO::GPIO& diagEn, IO::GPIO& diagSelect1, IO::GPIO& diagSelect2, IO::ADC& adc, uint8_t current_diag_mode) : EN1(en1), EN2(en2), SENSE_OUT(senseOut), LATCH(latch), DIAG_EN(diagEn), DIAG_SELECT_1(diagSelect1), DIAG_SELECT_2(diagSelect2), ADC(adc) {
    setDiagnostics(DIAG_MODE::OFF);
}

void TPS2HB50BQ1::enableAll() {
    EN1.writePin(IO::GPIO::State::HIGH);
    EN2.writePin(IO::GPIO::State::HIGH);
}

void TPS2HB50BQ1::disableAll() {
    EN1.writePin(IO::GPIO::State::LOW);
    EN2.writePin(IO::GPIO::State::LOW);
}
void TPS2HB50BQ1::enable_ps_one() {
    EN1.writePin(IO::GPIO::State::HIGH);
}

void TPS2HB50BQ1::disable_ps_one() {
    EN1.writePin(IO::GPIO::State::LOW);
}

void TPS2HB50BQ1::enable_ps_two() {
    EN2.writePin(IO::GPIO::State::HIGH);
}

void TPS2HB50BQ1::disable_ps_two() {
    EN2.writePin(IO::GPIO::State::LOW);
}

void TPS2HB50BQ1::enableDiag() {
    DIAG_EN.writePin(IO::GPIO::State::HIGH);
}

void TPS2HB50BQ1::disableDiag() {
    DIAG_EN.writePin(IO::GPIO::State::LOW);
}

/**
 * See TP2HB50BQ1 datasheet for more information on latch modes.
 * According to datasheet, if latch is low, then switch will auto-retry.
 * If latch is high, then switch will latch until latch pin is pulled low.
 * Page 23/27 of the datasheet has a table with the different modes.
 * @param mode
 */
void TPS2HB50BQ1::setLatch(LATCH_MODE mode) {
    if (mode == LATCH_MODE::LATCHED) {
        LATCH.writePin(IO::GPIO::State::LOW);
        current_latch_mode = LATCH_MODE::LATCHED;
    } else if (mode == LATCH_MODE::AUTO_RETRY) {
        LATCH.writePin(IO::GPIO::State::HIGH);
        current_latch_mode = LATCH_MODE::AUTO_RETRY;
    }
}

void TPS2HB50BQ1::readSenseOut(uint32_t& senseOut) {
    senseOut = ADC.readRaw();
}

/**
 * Set the diagnostic mode:
 * DIAG_MODE_OFF: Sets diagnostics pin to low, along with both diag select pins.
 * DIAG_MODE_FAULT_STATUS: Sets diagnostics pin to high, along with the fault status diag select pin.
 * DIAG_MODE_CURRENT: Sets diagnostics pin to high, along with the current diag select pin.
 * DIAG_MODE_TEMP: Sets diagnostics pin to high, along with the temp diag select pin.
 *
 * TODO: MUX IS NOT NECESSARILY CORRECT
 *
 * @param diag_mode the mode to set the diagnostics to; see above for options, or header file for consts.
 */
void TPS2HB50BQ1::setDiagnostics(DIAG_MODE diag_mode) {
    current_diag_mode = diag_mode;
    switch (diag_mode) {
    case OFF:
        disableDiag();
        DIAG_SELECT_1.writePin(IO::GPIO::State::LOW);
        DIAG_SELECT_2.writePin(IO::GPIO::State::LOW);
        break;
    case FAULT_STATUS:
        enableDiag();
        DIAG_SELECT_1.writePin(IO::GPIO::State::HIGH);
        DIAG_SELECT_2.writePin(IO::GPIO::State::LOW);
        break;
    case CURRENT:
        enableDiag();
        DIAG_SELECT_1.writePin(IO::GPIO::State::LOW);
        DIAG_SELECT_2.writePin(IO::GPIO::State::HIGH);
        break;
    case TEMP:
        enableDiag();
        DIAG_SELECT_1.writePin(IO::GPIO::State::HIGH);
        DIAG_SELECT_2.writePin(IO::GPIO::State::HIGH);
        break;
    default:
        break;
    }
}

/**
 * Get the fault status of the TPS2HB50BQ1
 * Currently do not know the adc output for this, so it is a placeholder.
 * @return the fault status of the TPS2HB50BQ1
 */
uint32_t TPS2HB50BQ1::getCurrent() {
    if (current_diag_mode != DIAG_MODE::CURRENT) {
        setDiagnostics(DIAG_MODE::CURRENT);
    }
    uint32_t current = ADC.readRaw();// TODO: create a definition for this
    setDiagnostics(DIAG_MODE::OFF);
    return current;
}

/**
 * Get the temperature of the TPS2HB50BQ1
 * @return the temperature of the TPS2HB50BQ1
 */
uint32_t TPS2HB50BQ1::getTemp() {
    if (current_diag_mode != DIAG_MODE::TEMP) {
        setDiagnostics(DIAG_MODE::TEMP);
    }
    uint32_t temp = ADC.readRaw();// TODO: create a definition for this
    setDiagnostics(DIAG_MODE::OFF);
    return temp;
}

/**
 * Get the fault status of the TPS2HB50BQ1
 * Currently do not know the adc output for this, so it is a placeholder.
 * @return the fault status of the TPS2HB50BQ1
 */
uint32_t TPS2HB50BQ1::getFaultStatus() {
    if (current_diag_mode != DIAG_MODE::FAULT_STATUS) {
        setDiagnostics(DIAG_MODE::FAULT_STATUS);
    }
    uint32_t faultStatus = ADC.readRaw();// TODO: create a definition for this
    setDiagnostics(DIAG_MODE::OFF);
    return faultStatus;
}

}// namespace LVSS
