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

LVSS::LVSS(TPS2HB50BQ1* powerSwitches[POWER_SWITCHES_SIZE]) {
    for (int i = 0; i < POWER_SWITCHES_SIZE; i++) {
        this->powerSwitches[i] = powerSwitches[i];
    }
}

// temporarily commented out to make compiler happy
//LVSS::LVSS(TPS2HB50BQ1 powerSwitchArr[POWER_SWITCHES_SIZE]) : PowerSwitches(powerSwitchArr), currentSensor(currentSensor) {
//}
//LVSS::LVSS(TPS2HB50BQ1* powerSwitches) : PowerSwitches(powerSwitches) {}
//

void LVSS::setBoardEnable() {
    this->boardEN.val = VCUBoardSig;
}

uint8_t LVSS::getBoardEnable() {
    return boardEN.val;
}

/** LVSS Vicor DCM4623 State Machine */
void LVSS::process() {
    switch (state) {
    case State::INITIALIZATION:
        initState();

    case State::SOFT_START:
        softStartState();

    case State::POWER_UP:
        powerUpState();

    case State::IDLE:
        idleState();
    }
}

void LVSS::initState() {
    /** Entry housekeeping */
    /* Checks if the FSM has entered a new state */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering initialization state");
    }

    /** State business */
    time::wait(102); // Wait for 102ms as per the VICOR datasheet

    /**
     * Exit housekeeping
     * If 110ms has passed then go into the next state
     */
    if (time::millis() >= 110) {
        state = State::SOFT_START;
        isNewState = true;
    }
    else {
        log::LOGGER.log(log::Logger::LogLevel::ERROR, "Initialization state error");
    }
}

void LVSS::softStartState() {
    /** Entry housekeeping */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering soft start state");
    }

    /** State business */
    this->boardEN.val = VCUBoardSig; // Assigns the VCU signal to the union bit field
    // Read HV Current
    // Read Error Status
    // Read Switch Current

    log::LOGGER.log(log::Logger::LogLevel::INFO, "%d", boardEN.val);

    /**
     * Exit housekeeping
     * If the VCU sends a signal to turn on all the boards go to the power up state
     */
    if (boardEN.val == 0x3F) {
        state = State::POWER_UP;
        isNewState = true;
    }
}

void LVSS::powerUpState() {
    /** Entry housekeeping */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering power up state");
    }

    /**
     * State business
     * 1. Assign which boards will be turned on
     * 2. Write to the SDO server
     */

    /** Exit housekeeping */
}

void LVSS::idleState() {
    /** Entry housekeeping */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering idle state");
    }

    /**
     * State business
     * 1. Checks important data
     * 2. If a piece of data is not what is expected or coming in late give a corresponding error
     */
    if(highValCurrent >= 9000) {
        err = LVSS_ERR::LVSS_ERR_HV_CURRENT;
    }
    else {
        err = LVSS_ERR::LVSS_ERR_NONE;
    }

    /** Exit housekeeping */
}
}// namespace LVSS