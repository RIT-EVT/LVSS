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
    : vicorFT(vicorFT), boardEN({0}), acs71240(acs71240), state(State::INITIALIZATION) {
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

void LVSS::runningState() {
    /* Check if it is a new state */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering running state");
    }
    /* Assigns the VCU signal to the union bit field */
    this->boardEN.val = VCUBoardSig;

    log::LOGGER.log(log::Logger::LogLevel::DEBUG,
                    "Battery: %d\r\nHIB: %d\r\nTMS: %d\r\nHUDL: %d\r\nACC: %d\r\nGUB: %d\r\n",
                    boardEN.batt,
                    boardEN.hib,
                    boardEN.tms,
                    boardEN.hudl,
                    boardEN.acc,
                    boardEN.gub);

    /* Turn on boards */
    powerSwitches[0]->setPowerSwitchStates(boardEN.batt, boardEN.hib); // Turn on Battery and HIB
    powerSwitches[1]->setPowerSwitchStates(boardEN.tms, boardEN.hudl); // Turn on TMS and HUDL
    powerSwitches[2]->setPowerSwitchStates(boardEN.acc, boardEN.gub);  // Turn on Acc and GUB

#ifdef DIAG_ENABLED
    /* Current Checking */
    PowerSwitchState.battCurrent = powerSwitches[0]->getCurrent(TPS2HB35BQ::CH1);

    if (PowerSwitchState.battCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        PowerSwitchFaults.battCurrentFault = 1;
        boardEN.batt                       = 0;
        powerSwitches[0]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[0]->setPowerSwitchStates(boardEN.batt, boardEN.hib);
    }

    PowerSwitchState.tmsCurrent  = powerSwitches[1]->getCurrent(TPS2HB35BQ::CH1);

    if (PowerSwitchState.tmsCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        PowerSwitchFaults.tmsCurrentFault = 1;
        boardEN.tms                       = 0;
        powerSwitches[1]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[1]->setPowerSwitchStates(boardEN.tms, boardEN.hudl);
    }

    PowerSwitchState.accCurrent  = powerSwitches[2]->getCurrent(TPS2HB35BQ::CH1);

    if (PowerSwitchState.accCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        PowerSwitchFaults.accCurrentFault = 1;
        boardEN.acc                       = 0;
        powerSwitches[2]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[2]->setPowerSwitchStates(boardEN.acc, boardEN.gub);
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO,
                    "Power Switch Channel 1 Current Fault Status\r\nBattery: %d\r\nTMS: %d\r\nAcc: %d\r\n",
                    PowerSwitchFaults.battCurrentFault,
                    PowerSwitchFaults.tmsCurrentFault,
                    PowerSwitchFaults.accCurrentFault);

    log::LOGGER.log(log::Logger::LogLevel::DEBUG,
                    "Switch 0 Channel 1 Current: %d microAmps\r\nSwitch 1 Channel 1 Current: %d microAmps\r\nSwitch 2 "
                    "Channel 1 Current:"
                    " %d microAmps\r\n",
                    PowerSwitchState.battCurrent,
                    PowerSwitchState.tmsCurrent,
                    PowerSwitchState.accCurrent);

    time::wait(2); // Power switches require the ADC to wait a min of 165 micro seconds before sampling SNS pin again

    PowerSwitchState.hibCurrent  = powerSwitches[0]->getCurrent(TPS2HB35BQ::CH2);

    if (PowerSwitchState.hibCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        PowerSwitchFaults.hibCurrentFault = 1;
        boardEN.hib                       = 0;
        powerSwitches[0]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[0]->setPowerSwitchStates(boardEN.batt, boardEN.hib);
    }

    PowerSwitchState.hudlCurrent = powerSwitches[1]->getCurrent(TPS2HB35BQ::CH2);

    if (PowerSwitchState.hudlCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        PowerSwitchFaults.hudlCurrentFault = 1;
        boardEN.hudl = 0;
        powerSwitches[1]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[1]->setPowerSwitchStates(boardEN.tms, boardEN.hudl);
    }

    PowerSwitchState.gubCurrent  = powerSwitches[2]->getCurrent(TPS2HB35BQ::CH2);

    if (PowerSwitchState.gubCurrent == TPS2HB35BQ::CURRENT_FAULT) {
        PowerSwitchFaults.gubCurrentFault = 1;
        boardEN.gub                       = 0;
        powerSwitches[2]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[2]->setPowerSwitchStates(boardEN.acc, boardEN.gub);
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO,
                    "Power Switch Channel 2 Current Fault Status\r\nHIB: %d\r\nHUDL: %d\r\nGUB: %d\r\n",
                    PowerSwitchFaults.hibCurrentFault,
                    PowerSwitchFaults.hudlCurrentFault,
                    PowerSwitchFaults.gubCurrentFault);

    log::LOGGER.log(log::Logger::LogLevel::DEBUG,
                    "Switch 0 Channel 2 Current: %d microAmps\r\nSwitch 1 Channel 2 Current: %d microAmps\r\nSwitch 2 "
                    "Channel 2 Current: %d microAmps\r\n",
                    PowerSwitchState.hibCurrent,
                    PowerSwitchState.hudlCurrent,
                    PowerSwitchState.gubCurrent);

    PowerSwitchState.switch0Temp = powerSwitches[0]->getTemperature();

    if (PowerSwitchState.switch0Temp == TPS2HB35BQ::TEMP_FAULT) {
        PowerSwitchFaults.switch0TempFault = 1;
        boardEN.batt                       = 0;
        boardEN.hib                        = 0;
        powerSwitches[0]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[0]->setPowerSwitchStates(boardEN.batt, boardEN.hib);
    }

    PowerSwitchState.switch1Temp = powerSwitches[1]->getTemperature();

    if (PowerSwitchState.switch1Temp == TPS2HB35BQ::TEMP_FAULT) {
        PowerSwitchFaults.switch1TempFault = 1;
        boardEN.tms                        = 0;
        boardEN.hudl                       = 0;
        powerSwitches[1]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[1]->setPowerSwitchStates(boardEN.tms, boardEN.hudl);
    }

    PowerSwitchState.switch2Temp = powerSwitches[2]->getTemperature();

    if (PowerSwitchState.switch2Temp == TPS2HB35BQ::TEMP_FAULT) {
        PowerSwitchFaults.switch2TempFault = 1;
        boardEN.gub                        = 0;
        boardEN.acc                        = 0;
        powerSwitches[2]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[2]->setPowerSwitchStates(boardEN.acc, boardEN.gub);
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO,
                    "Power Switch Temperature Fault Status\r\nSwitch 0: %d\r\nSwitch 1: %d\r\nSwitch 2: %d\r\n",
                    PowerSwitchFaults.switch0TempFault,
                    PowerSwitchFaults.switch1TempFault,
                    PowerSwitchFaults.switch2TempFault);

    log::LOGGER.log(
        log::Logger::LogLevel::DEBUG,
        "Switch 0 Temperature: %d milliCelsius\r\nSwitch 1 Temperature: %d milliCelsius\r\nSwitch 2 Temperature: "
        "%d milliCelsius\r\n",
        PowerSwitchState.switch0Temp,
        PowerSwitchState.switch1Temp,
        PowerSwitchState.switch2Temp);
#endif // DIAG_ENABLED

    battPackCurrent = acs71240.readCurrent();

    log::LOGGER.log(log::Logger::LogLevel::DEBUG, "Vicor Current: %d\r\n", battPackCurrent);
}
} // namespace LVSS
