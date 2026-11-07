#include <LVSS.hpp>

namespace LVSS {

uint8_t LVSS::getNodeID() {
    return NODE_ID;
}

uint8_t LVSS::getNumElements() {
    return OBJECT_DICTIONARY_SIZE + 1;
}

CO_OBJ_T* LVSS::getObjectDictionary() {
    return &objectDictionary[0];
}

LVSS::LVSS(TPS2HB35BQ* powerSwitches[POWER_SWITCHES_SIZE], io::GPIO& vicorFT, ACS71240 acs71240)
    : vicorFT(vicorFT), wantedEn({0}), activeEn({0}), acs71240(acs71240), powerSwitchState({0}),
      powerSwitchFaults({0}), vcuBoardSig(0), state(State::INITIALIZATION) {
    for (int i = 0; i < POWER_SWITCHES_SIZE; i++) {
        this->powerSwitches[i] = powerSwitches[i];
    }
}

/* LVSS Vicor DCM4623 State Machine */
void LVSS::process() {
    switch (state) {
    case State::INITIALIZATION:
        initState();
        break;

    case State::RUNNING:
        runningState();
        break;
    }
}

void LVSS::initState() {
    /* Check if it is a new state */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering initialization state");
    }

    VICOR_FAULT_ACTIVE_STATE = vicorFT.readPin();

    // set everything to off/starting state
    powerSwitches[0]->setPowerSwitchStates(false, false); // set Battery and HIB switches to off
    powerSwitches[0]->setLatch(TPS2HB35BQ::LATCHED); // when there is a fault, stay off
    powerSwitches[1]->setPowerSwitchStates(false, false); // Turn TMS and HUDL switches to off
    powerSwitches[1]->setLatch(TPS2HB35BQ::LATCHED); // when there is a fault, stay off
    powerSwitches[2]->setPowerSwitchStates(false, false);  // Turn Acc and GUB switches to off
    powerSwitches[2]->setLatch(TPS2HB35BQ::LATCHED); // when there is a fault, stay off

    // When the Vicor starts, after a certain amount of time if the Vicor has not exited a certain state then it has
    // most likely entered a fault state. This information can be found on pgae 8 of the Vicor datasheet.
    if (time::millis() >= 101 && VICOR_FAULT_ACTIVE_STATE == io::GPIO::State::LOW) {
        state      = State::RUNNING;
        isNewState = true;
    }
}

uint32_t lastToggle = 0;

void LVSS::runningState() {
    /* Check if it is a new state */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering running state");
    }
    /* Assigns the VCU signal to the union bit field */
    this->wantedEn.val = vcuBoardSig;

    /* TODO: Debug toggling for hudl output pin */
//    bool toggleTime = time::millis() - lastToggle >= 20000;
//    if (toggleTime) {
//        lastToggle = time::millis();
//        wantedEn.hudl ^= 1;
//    }

    /* Debug prints of wanted values for testing output once a second */
    bool printDebug = time::millis() - lastDebugPrintMs >= 1000;
    if (printDebug) {
        lastDebugPrintMs = time::millis();

        // "\r\033[2K" erases the stray prefix, "\033[H" moves the cursor home to redraw in place
        log::LOGGER.log(log::Logger::LogLevel::DEBUG,
                        "\r\033[2K\033[H        %5s%5s%5s%5s%5s%5s\033[K",
                        "Batt", "HIB", "TMS", "HUDL", "ACC", "GUB");

        log::LOGGER.log(log::Logger::LogLevel::DEBUG,
                        "\rWanted: %5d%5d%5d%5d%5d%5d\033[K",
                        wantedEn.batt, wantedEn.hib, wantedEn.tms, wantedEn.hudl, wantedEn.acc, wantedEn.gub);
    }

    /* address any faults; aka ignore any enable signals for a switch that has a fault */
    wantedEn.batt = (wantedEn.batt && !powerSwitchFaults.battCurrentFault && !powerSwitchFaults.switch0TempFault);
    wantedEn.hib = (wantedEn.hib && !powerSwitchFaults.hibCurrentFault && !powerSwitchFaults.switch0TempFault);
    wantedEn.hudl = (wantedEn.hudl && !powerSwitchFaults.hudlCurrentFault && !powerSwitchFaults.switch1TempFault);
    wantedEn.tms = (wantedEn.tms && !powerSwitchFaults.tmsCurrentFault && !powerSwitchFaults.switch1TempFault);
    wantedEn.acc = (wantedEn.acc && !powerSwitchFaults.accCurrentFault && !powerSwitchFaults.switch2TempFault);
    wantedEn.gub = (wantedEn.gub && !powerSwitchFaults.gubCurrentFault && !powerSwitchFaults.switch2TempFault);

    /* Debug prints of actual values for testing output */
    if (printDebug) {
        // "\033[J" also clears everything below the table (e.g. other logs from between redraws)
        log::LOGGER.log(log::Logger::LogLevel::DEBUG,
                        "\rActual: %5d%5d%5d%5d%5d%5d\033[K\033[J",
                        wantedEn.batt, wantedEn.hib, wantedEn.tms, wantedEn.hudl, wantedEn.acc, wantedEn.gub);
    }

    /* Turn on boards */
    if (activeEn.batt != wantedEn.batt || activeEn.hib != wantedEn.hib) {
        powerSwitches[0]->setPowerSwitchStates(wantedEn.batt, wantedEn.hib); // update Battery and HIB
    }

    if (activeEn.hudl != wantedEn.hudl || activeEn.tms != wantedEn.tms) {
        powerSwitches[1]->setPowerSwitchStates(wantedEn.hudl, wantedEn.tms); // update HUDL and TMS
    }

    if (activeEn.acc != wantedEn.acc || activeEn.gub != wantedEn.gub) {
        powerSwitches[2]->setPowerSwitchStates(wantedEn.acc, wantedEn.gub);  // update Acc and GUB
    }

    // update activeEn
    memcpy(&activeEn, &wantedEn, sizeof(wantedEn));

#ifdef DIAG_ENABLED
    /* Current Checking */
    powerSwitchState.battCurrent = powerSwitches[0]->getCurrent(TPS2HB35BQ::CH1);

    if (!powerSwitchFaults.battCurrentFault && powerSwitchState.battCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        powerSwitchFaults.battCurrentFault = 1;
        activeEn.batt                      = 0;
        powerSwitches[0]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[0]->setPowerSwitchStates(activeEn.batt, activeEn.hib);
    }

    powerSwitchState.hudlCurrent = powerSwitches[1]->getCurrent(TPS2HB35BQ::CH1);

    if (!powerSwitchFaults.hudlCurrentFault && powerSwitchState.hudlCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        powerSwitchFaults.hudlCurrentFault = 1;
        activeEn.hudl                      = 0;
        powerSwitches[1]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[1]->setPowerSwitchStates(activeEn.hudl, activeEn.tms);
    }

    powerSwitchState.accCurrent = powerSwitches[2]->getCurrent(TPS2HB35BQ::CH1);

    if (!powerSwitchFaults.accCurrentFault && powerSwitchState.accCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        powerSwitchFaults.accCurrentFault = 1;
        activeEn.acc                      = 0;
        powerSwitches[2]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[2]->setPowerSwitchStates(activeEn.acc, activeEn.gub);
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO,
                    "Power Switch Channel 1 Current Fault Status\r\nBattery: %d\r\nTMS: %d\r\nAcc: %d\r\n",
                    powerSwitchFaults.battCurrentFault,
                    powerSwitchFaults.tmsCurrentFault,
                    powerSwitchFaults.accCurrentFault);

    log::LOGGER.log(log::Logger::LogLevel::DEBUG,
                    "0-1: %d;\t1-1: %d;\t2-1: %d;\r\n out: %d",
                    powerSwitchState.battCurrent,
                    powerSwitchState.tmsCurrent,
                    powerSwitchState.accCurrent);

    time::wait(2); // Power switches require the ADC to wait a min of 165 micro seconds before sampling SNS pin again

    powerSwitchState.hibCurrent = powerSwitches[0]->getCurrent(TPS2HB35BQ::CH2);

    if (!powerSwitchFaults.hibCurrentFault && powerSwitchState.hibCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        powerSwitchFaults.hibCurrentFault = 1;
        activeEn.hib                      = 0;
        powerSwitches[0]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[0]->setPowerSwitchStates(activeEn.batt, activeEn.hib);
    }

    powerSwitchState.hudlCurrent = powerSwitches[1]->getCurrent(TPS2HB35BQ::CH2);

    if (!powerSwitchFaults.tmsCurrentFault && powerSwitchState.tmsCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        powerSwitchFaults.tmsCurrentFault = 1;
        activeEn.tms                      = 0;
        powerSwitches[1]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[1]->setPowerSwitchStates(activeEn.hudl, activeEn.tms);
    }

    powerSwitchState.gubCurrent = powerSwitches[2]->getCurrent(TPS2HB35BQ::CH2);

    if (!powerSwitchFaults.gubCurrentFault && powerSwitchState.gubCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        powerSwitchFaults.gubCurrentFault = 1;
        activeEn.gub                      = 0;
        powerSwitches[2]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[2]->setPowerSwitchStates(activeEn.acc, activeEn.gub);
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO,
                    "Power Switch Channel 2 Current Fault Status\r\nHIB: %d\r\nHUDL: %d\r\nGUB: %d\r\n",
                    powerSwitchFaults.hibCurrentFault,
                    powerSwitchFaults.hudlCurrentFault,
                    powerSwitchFaults.gubCurrentFault);

    log::LOGGER.log(log::Logger::LogLevel::DEBUG,
                    "Switch 0 Channel 2 Current: %d microAmps\r\nSwitch 1 Channel 2 Current: %d microAmps\r\nSwitch 2 "
                    "Channel 2 Current: %d microAmps\r\n",
                    powerSwitchState.hibCurrent,
                    powerSwitchState.hudlCurrent,
                    powerSwitchState.gubCurrent);

    powerSwitchState.switch0Temp = powerSwitches[0]->getTemperature();

    if (!powerSwitchFaults.switch0TempFault && powerSwitchState.switch0Temp == TPS2HB35BQ::TEMP_FAULT) {
        powerSwitchFaults.switch0TempFault = 1;
        activeEn.batt                      = 0;
        activeEn.hib                       = 0;
        powerSwitches[0]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[0]->setPowerSwitchStates(activeEn.batt, activeEn.hib);
    }

    powerSwitchState.switch1Temp = powerSwitches[1]->getTemperature();

    if (!powerSwitchFaults.switch1TempFault && powerSwitchState.switch1Temp == TPS2HB35BQ::TEMP_FAULT) {
        powerSwitchFaults.switch1TempFault = 1;
        activeEn.hudl                      = 0;
        activeEn.tms                       = 0;
        powerSwitches[1]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[1]->setPowerSwitchStates(activeEn.hudl, activeEn.tms);
    }

    powerSwitchState.switch2Temp = powerSwitches[2]->getTemperature();

    if (!powerSwitchFaults.switch2TempFault && powerSwitchState.switch2Temp == TPS2HB35BQ::TEMP_FAULT) {
        powerSwitchFaults.switch2TempFault = 1;
        activeEn.gub                        = 0;
        activeEn.acc                        = 0;
        powerSwitches[2]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[2]->setPowerSwitchStates(activeEn.acc, activeEn.gub);
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO,
                    "Power Switch Temperature Fault Status\r\nSwitch 0: %d\r\nSwitch 1: %d\r\nSwitch 2: %d\r\n",
                    powerSwitchFaults.switch0TempFault,
                    powerSwitchFaults.switch1TempFault,
                    powerSwitchFaults.switch2TempFault);

    log::LOGGER.log(
        log::Logger::LogLevel::DEBUG,
        "Switch 0 Temperature: %d milliCelsius\r\nSwitch 1 Temperature: %d milliCelsius\r\nSwitch 2 Temperature: "
        "%d milliCelsius\r\n",
        powerSwitchState.switch0Temp,
        powerSwitchState.switch1Temp,
        powerSwitchState.switch2Temp);
#endif // DIAG_ENABLED

    battPackCurrent = acs71240.readCurrent();

//    log::LOGGER.log(log::Logger::LogLevel::DEBUG, "Vicor Current: %d\r\n", battPackCurrent);
}
} // namespace LVSS
