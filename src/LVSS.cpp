#include <LVSS.hpp>

namespace LVSS {

uint8_t LVSS::getNodeID() {
    return NODE_ID;
}

uint8_t LVSS::getNumElements() {
    return OBJECT_DICTIONARY_SIZE + 1;
}

CO_OBJ_T* LVSS::getObjectDictionary() {
    return  &objectDictionary[0];
}

LVSS::LVSS(TPS2HB50BQ1* powerSwitches[POWER_SWITCHES_SIZE], IO::GPIO& vicorFT) : vicorFT(vicorFT), state(State::INITIALIZATION), boardEN({0}) {
    for (int i = 0; i < POWER_SWITCHES_SIZE; i++) {
        this->powerSwitches[i] = powerSwitches[i];
    }
}

void LVSS::setBoardEnable() {
    this->boardEN.val = VCUBoardSig;
}

/* LVSS Vicor DCM4623 State Machine */
void LVSS::process() {
    switch (state) {
    case State::INITIALIZATION:
        initState();
        break;

    case State::SOFT_START:
        softStartState();
        break;

    case State::POWER_UP:
        powerUpState();
        break;

    case State::IDLE:
        idleState();
        break;
    }
}

void LVSS::initState() {
    /* Entry */
    if (isNewState) { // Checks if the FSM has entered a new state
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering initialization state");
    }

    /* State business */
    time::wait(102); // Wait for 102ms as per the VICOR datasheet

    /* Exit */
    if (time::millis() >= 110) { // If 110ms has passed then go into the next state
        state = State::SOFT_START;
        isNewState = true;
    }
    else {
        log::LOGGER.log(log::Logger::LogLevel::ERROR, "Initialization state error");
    }
}

void LVSS::softStartState() {
    /* Entry */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering soft start state");
    }

    /* State business */
    this->boardEN.val = VCUBoardSig; // Assigns the VCU signal to the union bit field

    log::LOGGER.log(log::Logger::LogLevel::INFO, "board enable: %d", boardEN.val);

    /* Exit */
    if (vicorFT.readPin() == VICOR_FAULT_ACTIVE_STATE) { // Check for vicor fault
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Vicor Fault Status: %d\r\n", vicorFT.readPin());
    }
    else if (boardEN.val == 63) { // If all the boards have been set to turn on go to the next state
        state = State::POWER_UP;
        isNewState = true;
    }
}

void LVSS::powerUpState() {
    /** Entry */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering power up state");
    }

    log::LOGGER.log(log::Logger::LogLevel::INFO, "current: %d\r\ntemperature: %d\r\nfault status: %d\r\n", switchCurrent, switchTemperature, switchFaultstatus);

    /** State business */
    powerSwitches[0]->setPowerSwitchStates(boardEN.batt,boardEN.hib); // Turn on battery and HIB
    powerSwitches[1]->setPowerSwitchStates(boardEN.tms,boardEN.hudl); // Turn on TMS and HUDL
    powerSwitches[2]->setPowerSwitchStates(boardEN.acc,boardEN.gub);  // Turn on Acc and GUB

    /* Exit */
    if (vicorFT.readPin() == VICOR_FAULT_ACTIVE_STATE) { // Check for vicor fault
        log::LOGGER.log(log::Logger::LogLevel::ERROR, "Vicor Fault Status: %d\r\n", vicorFT.readPin());
    }
    else {
        state = State::IDLE;
        isNewState = true;
    }
}

void LVSS::idleState() {
    /** Entry */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering idle state");
    }

    /** State business */
    for (int i=0; i<POWER_SWITCHES_SIZE; i++) {
        if(powerSwitches[i]->getCurrent() >= 9000){ // Check Switch current
            log::LOGGER.log(log::Logger::LogLevel::ERROR,"Switch %d current error: %d\r\n", i, powerSwitches[i]->getCurrent());
        }

        if(powerSwitches[i]->getTemp() >= 135){ // Check Switch temperature
            log::LOGGER.log(log::Logger::LogLevel::ERROR,"Switch %d temperature Error: %d\r\n", i, powerSwitches[i]->getTemp());
        }

        if(powerSwitches[i]->getFaultStatus() >= 9000){ // Check Switch fault status
            log::LOGGER.log(log::Logger::LogLevel::ERROR,"Switch %d fault error: %d\r\n", i, powerSwitches[i]->getFaultStatus());
        }
    }

    /** Exit */
}

}// namespace LVSS