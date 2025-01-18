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

LVSS::LVSS(TPS2HB50BQ1* powerSwitches[POWER_SWITCHES_SIZE]) : boardEN({0}), VCUBoardSig(0), highValCurrent(0), swCurrent(0) {
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
    if (isNewState) { // Checks if the FSM has entered a new state
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering initialization state");
    }

    /** State business */
    time::wait(102); // Wait for 102ms as per the VICOR datasheet

    /** Exit housekeeping */
    if (time::millis() >= 110) { // If 110ms has passed then go into the next state
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

    log::LOGGER.log(log::Logger::LogLevel::INFO, "boardEN.val = %d", boardEN.val);

    /** Exit housekeeping */
    if (boardEN.val == 0x3F) { // If the VCU sends a signal to turn on all the boards go to the power up state
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
    bool powerUpFinished = false;

    /** State business */
    // Assign which boards will be turned on

    /** Exit housekeeping */
    if (powerUpFinished) {
        state = State::IDLE;
        isNewState = true;
    }
}

void LVSS::idleState() {
    /** Entry housekeeping */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering idle state");
    }

    /** State business */
    if(highValCurrent >= 9000) {
        log::LOGGER.log(log::Logger::LogLevel::ERROR, "High Value Current Error\r\n");
    }
    else if(swCurrent >= 9000) {
        log::LOGGER.log(log::Logger::LogLevel::ERROR, "Switch Current Error\r\n");
    }

    /** Exit housekeeping */
}
}// namespace LVSS