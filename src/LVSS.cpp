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

LVSS::LVSS(TPS2HB50BQ1* powerSwitches[POWER_SWITCHES_SIZE], IO::GPIO& vicorFT) : vicorFT(vicorFT), boardEN({0}), state(State::INITIALIZATION) {
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

    case State::SOFT_START:
        softStartState();
        break;

    case State::POWER_UP:
        powerUpState();
        break;

    case State::IDLE:
        idleState();
        break;

    case State::FAULT:
        faultState();
        break;
    }
}

void LVSS::initState() {
    /* Entry */
    if (isNewState) {// Checks if the FSM has entered a new state
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering initialization state");
    }

    /* State business */
    time::wait(102);// Wait for 102ms as per the VICOR datasheet

    /* Exit */
    if (time::millis() >= 110) {// If 110ms has passed then go into the next state
        state = State::SOFT_START;
        isNewState = true;
    } else {
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
    this->boardEN.val = VCUBoardSig;// Assigns the VCU signal to the union bit field

    log::LOGGER.log(log::Logger::LogLevel::INFO, "board enable: %d", boardEN.val);

    /* Exit */
    if (vicorFT.readPin() == VICOR_FAULT_ACTIVE_STATE) {// Check for vicor fault
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Vicor Fault Status: %d\r\n", vicorFT.readPin());
    } else if (boardEN.val == 63) {// If all the boards have been set to turn on go to the next state
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
    powerSwitches[0]->setPowerSwitchStates(boardEN.batt, boardEN.hib);// Turn on battery and HIB
    powerSwitches[1]->setPowerSwitchStates(boardEN.tms, boardEN.hudl);// Turn on TMS and HUDL
    powerSwitches[2]->setPowerSwitchStates(boardEN.acc, boardEN.gub); // Turn on Acc and GUB

    /* Exit */
    if (vicorFT.readPin() == VICOR_FAULT_ACTIVE_STATE) {// Check for vicor fault
        log::LOGGER.log(log::Logger::LogLevel::ERROR, "Vicor Fault Status: %d\r\n", vicorFT.readPin());
    } else {
        state = State::IDLE;
        isNewState = true;
    }
}

void LVSS::idleState() {
    /** Entry */
    if (isNewState) {
        isNewState = false;
        this->err[0] = PowerSwitchStatus::Safe;
        this->err[1] = PowerSwitchStatus::Safe;
        this->err[2] = PowerSwitchStatus::Safe;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering idle state");
    }

    /** State business */
    for (int i = 0; i < POWER_SWITCHES_SIZE; i++) {
        if (powerSwitches[i]->getCurrent() >= CurrentLim) {// Check Switch current in milliamps
            log::LOGGER.log(log::Logger::LogLevel::ERROR, "Switch %d current error: %d\r\n", i, powerSwitches[i]->getCurrent());
            this->err[i] = PowerSwitchStatus::OverCurrent;
        }

        if (powerSwitches[i]->getTemp() >= TemperatureLim) {// Check Switch temperature in Celsius
            log::LOGGER.log(log::Logger::LogLevel::ERROR, "Switch %d temperature Error: %d\r\n", i, powerSwitches[i]->getTemp());
            this->err[i] = PowerSwitchStatus::Temperature;
        }

        if (powerSwitches[i]->getFaultStatus()) {// Check Switch fault status
            log::LOGGER.log(log::Logger::LogLevel::ERROR, "Switch %d fault error: %d\r\n", i, powerSwitches[i]->getFaultStatus());
            this->err[i] = PowerSwitchStatus::Fault;
        }
    }

    if (this->err[0] == PowerSwitchStatus::Safe) {
        this->boardEN.batt = 1;
        this->boardEN.hib = 1;
    }

    if (this->err[1] == PowerSwitchStatus::Safe) {
        this->boardEN.tms = 1;
        this->boardEN.hudl = 1;
    }

    if (this->err[2] == PowerSwitchStatus::Safe) {
        this->boardEN.gub = 1;
        this->boardEN.acc = 1;
    }

    powerSwitches[0]->setPowerSwitchStates(boardEN.batt, boardEN.hib);
    powerSwitches[1]->setPowerSwitchStates(boardEN.tms, boardEN.hudl);
    powerSwitches[2]->setPowerSwitchStates(boardEN.acc, boardEN.gub);

    /** Exit */
    for (int i = 0; i < POWER_SWITCHES_SIZE; i++) {
        if (this->err[i] != PowerSwitchStatus::Safe) {
            state = State::FAULT;
            isNewState = true;
        }
    }
}

void LVSS::faultState() {
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering fault state\r\nPowering down all boards");
    }

    for (int i = 0; i < POWER_SWITCHES_SIZE; i++) {
        if (this->err[i] == PowerSwitchStatus::OverCurrent) {
            log::LOGGER.log(log::Logger::LogLevel::ERROR, "Fault: Overcurrent");
        }

        if (this->err[i] == PowerSwitchStatus::Temperature) {
            log::LOGGER.log(log::Logger::LogLevel::ERROR, "Fault: Temperature");
        }

        if (this->err[i] == PowerSwitchStatus::Fault) {
            log::LOGGER.log(log::Logger::LogLevel::ERROR, "Fault: Fault Pin High");
        }
    }

    if (this->err[0] != PowerSwitchStatus::Safe) {
        this->boardEN.batt = 0;
        this->boardEN.hib = 0;
    }

    if (this->err[1] != PowerSwitchStatus::Safe) {
        this->boardEN.tms = 0;
        this->boardEN.hudl = 0;
    }

    if (this->err[2] != PowerSwitchStatus::Safe) {
        this->boardEN.gub = 0;
        this->boardEN.acc = 0;
    }

    /** Exit */
    if (this->err[0] == PowerSwitchStatus::Safe && this->err[1] == PowerSwitchStatus::Safe && this->err[2] == PowerSwitchStatus::Safe) {
            state = State::POWER_UP;
            isNewState = true;
        }
}

}// namespace LVSS