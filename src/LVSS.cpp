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

    /* Current Checking */
    PowerSwitchState.battCurrent = powerSwitches[0]->getCurrentAndFault(1);
    PowerSwitchState.tmsCurrent  = powerSwitches[1]->getCurrentAndFault(1);
    PowerSwitchState.accCurrent  = powerSwitches[2]->getCurrentAndFault(1);

    if (PowerSwitchState.battCurrent == INTMAX_MIN) {
        PowerSwitchFaults.battCurrentFault = 1;
        powerSwitches[0]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[0]->setPowerSwitchStates(false
            , false);
    }

    if (PowerSwitchState.tmsCurrent == INTMAX_MIN) {
        PowerSwitchFaults.tmsCurrentFault = 1;
        powerSwitches[1]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[1]->setPowerSwitchStates(false
            , false);
    }

    if (PowerSwitchState.accCurrent == INTMAX_MIN) {
        PowerSwitchFaults.accCurrentFault = 1;
        powerSwitches[2]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[2]->setPowerSwitchStates(false
            , false);
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO,
        "Power Switch Channel 1 Current Fault Status\r\nBattery: %d\r\nTMS: %d\r\nAcc: %d\r\n",
        PowerSwitchFaults.battCurrentFault,
        PowerSwitchFaults.tmsCurrentFault,
        PowerSwitchFaults.accCurrentFault);

    log::LOGGER.log(log::Logger::LogLevel::DEBUG,
    "Switch 0 Channel 1 Current: %d\r\nSwitch 1 Channel 1 Current: %d\r\nSwitch 2 Channel 1 Current: %d\r\n",
    PowerSwitchState.battCurrent,
    PowerSwitchState.tmsCurrent,
    PowerSwitchState.accCurrent);

    time::wait(2); // Power switches require the ADC to wait a min of 165 micro seconds before sampling SNS pin again

    PowerSwitchState.hibCurrent  = powerSwitches[0]->getCurrentAndFault(2);
    PowerSwitchState.hudlCurrent = powerSwitches[1]->getCurrentAndFault(2);
    PowerSwitchState.gubCurrent  = powerSwitches[2]->getCurrentAndFault(2);

    if (PowerSwitchState.hibCurrent == INTMAX_MIN) {
        PowerSwitchFaults.hibCurrentFault = 1;
        powerSwitches[0]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[0]->setPowerSwitchStates(false
            , false);
    }

    if (PowerSwitchState.hudlCurrent == INTMAX_MIN) {
        PowerSwitchFaults.hudlCurrentFault = 1;
        powerSwitches[1]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[1]->setPowerSwitchStates(false
            , false);
    }

    if (PowerSwitchState.gubCurrent == INTMAX_MIN) {
        PowerSwitchFaults.gubCurrentFault = 1;
        powerSwitches[2]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[2]->setPowerSwitchStates(false
            , false);
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO,
        "Power Switch Channel 2 Current Fault Status\r\nHIB: %d\r\nHUDL: %d\r\nGUB: %d\r\n", PowerSwitchFaults.hibCurrentFault,
        PowerSwitchFaults.hudlCurrentFault,
        PowerSwitchFaults.gubCurrentFault);

    log::LOGGER.log(log::Logger::LogLevel::DEBUG,
    "Switch 0 Channel 2 Current: %d\r\nSwitch 1 Channel 2 Current: %d\r\nSwitch 2 Channel 2 Current: %d\r\n",
    PowerSwitchState.hibCurrent,
    PowerSwitchState.hudlCurrent,
    PowerSwitchState.gubCurrent);

    PowerSwitchState.switch0Temp = powerSwitches[0]->getTemperature();
    PowerSwitchState.switch1Temp = powerSwitches[1]->getTemperature();
    PowerSwitchState.switch2Temp = powerSwitches[2]->getTemperature();

    if (PowerSwitchState.switch0Temp == INTMAX_MIN) {
        PowerSwitchFaults.switch0TempFault = 1;
        powerSwitches[0]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[0]->setPowerSwitchStates(false
            , false);
    }

    if (PowerSwitchState.switch1Temp == INTMAX_MIN) {
        PowerSwitchFaults.switch1TempFault = 1;
        powerSwitches[1]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[1]->setPowerSwitchStates(false
            , false);
    }

    if (PowerSwitchState.switch2Temp == INTMAX_MIN) {
        PowerSwitchFaults.switch2TempFault = 1;
        powerSwitches[2]->setLatch(TPS2HB35BQ::LATCHED);
        powerSwitches[2]->setPowerSwitchStates(false
            , false);
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO,
    "Power Switch Temperature Fault Status\r\nSwitch 0: %d\r\nSwitch 1: %d\r\nSwitch 2: %d\r\n",
    PowerSwitchFaults.switch0TempFault,
    PowerSwitchFaults.switch1TempFault,
    PowerSwitchFaults.switch2TempFault);

    log::LOGGER.log(log::Logger::LogLevel::DEBUG,
                    "Switch 0 Temperature: %d\r\nSwitch 1 Temperature: %d\r\nSwitch 2 Temperature: %d\r\n",
                    PowerSwitchState.switch0Temp,
                    PowerSwitchState.switch1Temp,
                    PowerSwitchState.switch2Temp);

    battPackCurrent = acs71240.readCurrent();
    log::LOGGER.log(log::Logger::LogLevel::DEBUG, "Vicor Current: %d\r\n", battPackCurrent);
}
} // namespace LVSS
