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

LVSS::LVSS(TPS2HB35BQ* powerSwitches[POWER_SWITCHES_SIZE], IO::GPIO& vicorFT, ACS71240 acs71240) : vicorFT(vicorFT), boardEN({0}), adc0(adc0), acs71240(acs71240), state(State::INITIALIZATION) {
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

    case State::IDLE:
        idleState();
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

    if (time::millis() >= 101 && VICOR_FAULT_ACTIVE_STATE == IO::GPIO::State::LOW) {
        state = State::IDLE;
        isNewState = true;
    }
}

void LVSS::idleState() {
    /* Check if it is a new state */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering idle state");
    }
    /* Assigns the VCU signal to the union bit field */
    this->boardEN.val = VCUBoardSig;

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Battery: %d\r\nHIB: %d\r\nTMS: %d\r\nHUDL: %d\r\nACC: %d\r\nGUB: %d\r\n", boardEN.batt, boardEN.hib, boardEN.tms, boardEN.hudl, boardEN.acc, boardEN.gub);

    /* Turn on boards */
    powerSwitches[0]->setPowerSwitchStates(boardEN.batt, boardEN.hib);// Turn on Battery and HIB
    powerSwitches[1]->setPowerSwitchStates(boardEN.tms, boardEN.hudl);// Turn on TMS and HUDL
    powerSwitches[2]->setPowerSwitchStates(boardEN.acc, boardEN.gub); // Turn on Acc and GUB

    /* Current Checking */
    PowerSwitchState.battCurrent = powerSwitches[0]->getCurrent(1);
    PowerSwitchState.tmsCurrent = powerSwitches[1]->getCurrent(1);
    PowerSwitchState.accCurrent = powerSwitches[2]->getCurrent(1);

    if (PowerSwitchState.battCurrent == -1 || PowerSwitchState.tmsCurrent == -1 || PowerSwitchState.accCurrent == -1) {
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Power Switch Error\r\n");
        switchFaultstatus = 1;
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Switch 0 Channel 1: %d\r\nSwitch 1 Channel 1: %d\r\nSwitch 2 Channel 1: %d\r\n", PowerSwitchState.battCurrent, PowerSwitchState.tmsCurrent, PowerSwitchState.accCurrent);

    time::wait(2);// Power switches require the ADC to wait a min of 165 micro seconds before sampling SNS pin again

    PowerSwitchState.hibCurrent = powerSwitches[0]->getCurrent(2);
    PowerSwitchState.hudlCurrent = powerSwitches[1]->getCurrent(2);
    PowerSwitchState.gubCurrent = powerSwitches[2]->getCurrent(2);

    if (PowerSwitchState.hibCurrent == -1 || PowerSwitchState.hudlCurrent == -1 || PowerSwitchState.gubCurrent == -1) {
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Power Switch Error\r\n");
        switchFaultstatus = 1;
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Switch 0 Channel 2: %d\r\nSwitch 1 Channel 2: %d\r\nSwitch 2 Channel 2: %d\r\n", PowerSwitchState.hibCurrent, PowerSwitchState.hudlCurrent, PowerSwitchState.gubCurrent);

    PowerSwitchState.switch0Temp = powerSwitches[0]->getTempandFault();
    PowerSwitchState.switch1Temp = powerSwitches[1]->getTempandFault();
    PowerSwitchState.switch2Temp = powerSwitches[2]->getTempandFault();

    if (PowerSwitchState.switch0Temp == -1 || PowerSwitchState.switch1Temp == -1 || PowerSwitchState.switch2Temp == -1) {
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Power Switch Error\r\n");
        switchFaultstatus = 1;
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Switch 0 Temperature: %d\r\nSwitch 1 Temperature: %d\r\nSwitch 2 Temperature: %d\r\n", PowerSwitchState.switch0Temp, PowerSwitchState.switch1Temp, PowerSwitchState.switch2Temp);

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Vicor Temperature: %d\r\n", acs71240.readCurrent());
}
}// namespace LVSS
