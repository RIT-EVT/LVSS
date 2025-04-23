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

LVSS::LVSS(TPS2HB35BQ* powerSwitches[POWER_SWITCHES_SIZE], IO::GPIO& vicorFT) : vicorFT(vicorFT), boardEN({0}), state(State::INITIALIZATION) {
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
    /* Check if it is a new state */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering initialization state");
    }

    /* Wait for 102ms as per the VICOR datasheet */
    // time::wait(102);

    /* If 110ms has passed then go into the next state */
    if (time::millis() >= 102) {
        state = State::SOFT_START;
        isNewState = true;
    } else {
        log::LOGGER.log(log::Logger::LogLevel::ERROR, "Initialization state error");
    }
}

void LVSS::softStartState() {
    /* Check if it is a new state */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering soft start state");
    }

    /* Assigns the VCU signal to the union bit field */
    this->boardEN.val = VCUBoardSig;

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Battery: %d\r\nHIB: %d\r\nTMS: %d\r\nHUDL: %d\r\nACC: %d\r\nGUB: %d\r\n", boardEN.batt, boardEN.hib, boardEN.tms, boardEN.hudl, boardEN.acc, boardEN.gub);

    /* Check for vicor fault */
    if (vicorFT.readPin() == VICOR_FAULT_ACTIVE_STATE) {
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Vicor Fault Status: %d\r\n", vicorFT.readPin());
    } else if (boardEN.val == 63) {// If all the boards have been set to turn on go to the next state
        state = State::POWER_UP;
        isNewState = true;
    }
}

void LVSS::powerUpState() {
    /* Check if it is a new state */
    if (isNewState) {
        isNewState = false;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering power up state");
    }

    /* Turn on boards */
    powerSwitches[0]->setPowerSwitchStates(boardEN.batt, boardEN.hib);// Turn on Battery and HIB
    powerSwitches[1]->setPowerSwitchStates(boardEN.tms, boardEN.hudl);// Turn on TMS and HUDL
    powerSwitches[2]->setPowerSwitchStates(boardEN.acc, boardEN.gub); // Turn on Acc and GUB

    log::LOGGER.log(log::Logger::LogLevel::ERROR, "Vicor Fault Status: %d\r\n", vicorFT.readPin());

    /* Check for vicor fault */
    if (vicorFT.readPin() == VICOR_FAULT_ACTIVE_STATE) {
        log::LOGGER.log(log::Logger::LogLevel::ERROR, "Vicor Fault Status: %d\r\n", vicorFT.readPin());
    } else {
        state = State::IDLE;
        isNewState = true;
    }
}

void LVSS::idleState() {
    /* Check if it is a new state */
    if (isNewState) {
        isNewState = false;
        this->err[0] = PowerSwitchStatus::Safe;
        this->err[1] = PowerSwitchStatus::Safe;
        this->err[2] = PowerSwitchStatus::Safe;
        log::LOGGER.log(log::Logger::LogLevel::INFO, "Entering idle state");
    }

    /* Fault Checking */
    for (int i = 0; i < POWER_SWITCHES_SIZE; i++) {

        switchCH1Current = powerSwitches[i]->getChannel1Current();
        switchTemperature = powerSwitches[i]->getTempandFault();
        switchFaultstatus = powerSwitches[i]->getTempandFault();

        // if (powerSwitches[i]->getCurrent() >= CurrentLim) {// Check Switch current in milliamps
        //     log::LOGGER.log(log::Logger::LogLevel::ERROR, "Switch %d current error: %d\r\n", i, switchCurrent);
        //     this->err[i] = PowerSwitchStatus::OverCurrent;
        // }
        //
        // if (powerSwitches[i]->getTempandFault() >= 5000000000) {// Check Switch temperature in Celsius to determine if there is a fault
        //     log::LOGGER.log(log::Logger::LogLevel::ERROR, "Switch %d Fault: %d\r\n", i, switchTemperature);
        //     this->err[i] = PowerSwitchStatus::Fault;
        // }
        //
        // if (powerSwitches[i]->getTempandFault() >= 135) {// Check Switch temperature in Celsius
        //     log::LOGGER.log(log::Logger::LogLevel::ERROR, "Switch %d temperature Error: %d\r\n", i, switchFaultstatus);
        //     this->err[i] = PowerSwitchStatus::Temperature;
        // }
    }

    /* If no faults occur turn on the boards */
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

    /* Turn on boards */
    powerSwitches[0]->setPowerSwitchStates(boardEN.batt, boardEN.hib);
    powerSwitches[1]->setPowerSwitchStates(boardEN.tms, boardEN.hudl);
    powerSwitches[2]->setPowerSwitchStates(boardEN.acc, boardEN.gub);

    /** If a switch has an error go into the fault state */
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

    /* Check cause of the fault */
    for (int i = 0; i < POWER_SWITCHES_SIZE; i++) {
        if (this->err[i] == PowerSwitchStatus::OverCurrent) {
            log::LOGGER.log(log::Logger::LogLevel::ERROR, "Fault: Current Limit");
        }

        if (this->err[i] == PowerSwitchStatus::Temperature) {
            log::LOGGER.log(log::Logger::LogLevel::ERROR, "Fault: Temperature");
        }

        if (this->err[i] == PowerSwitchStatus::Fault) {
            log::LOGGER.log(log::Logger::LogLevel::ERROR, "Fault: Fault Pin High");
        }
    }

    /* Turn off corresponding switch */
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

    /* Wait for all switches to no longer have a fault */
    for (int i = 0; i < POWER_SWITCHES_SIZE; i++) {
        if (this->err[i] == PowerSwitchStatus::Safe) {
            state = State::POWER_UP;
            isNewState = true;
        }
    }
}

void LVSS::running() {

    /* Assigns the VCU signal to the union bit field */
    this->boardEN.val = VCUBoardSig;

    // log::LOGGER.log(log::Logger::LogLevel::INFO, "Battery: %d\r\nHIB: %d\r\nTMS: %d\r\nHUDL: %d\r\nACC: %d\r\nGUB: %d\r\n", boardEN.batt, boardEN.hib, boardEN.tms, boardEN.hudl, boardEN.acc, boardEN.gub);

    /* Turn on boards */
    powerSwitches[0]->setPowerSwitchStates(boardEN.batt, boardEN.hib);// Turn on Battery and HIB
    powerSwitches[1]->setPowerSwitchStates(boardEN.tms, boardEN.hudl);// Turn on TMS and HUDL
    powerSwitches[2]->setPowerSwitchStates(boardEN.acc, boardEN.gub); // Turn on Acc and GUB

    /* Current Checking */
    PowerSwitchState.battCurrent = powerSwitches[0]->getChannel1Current();
    PowerSwitchState.tmsCurrent = powerSwitches[1]->getChannel1Current();
    PowerSwitchState.accCurrent = powerSwitches[2]->getChannel1Current();

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Switch 0 Channel 1: %d\r\nSwitch 1 Channel 1: %d\r\nSwitch 2 Channel 1: %d\r\n", PowerSwitchState.battCurrent, PowerSwitchState.tmsCurrent, PowerSwitchState.accCurrent);

    time::wait(2);// Power switches require the ADC to wait a min of 165 micro seconds before sampling SNS pin again

    PowerSwitchState.hibCurrent = powerSwitches[0]->getChannel2Current();
    PowerSwitchState.hudlCurrent = powerSwitches[1]->getChannel2Current();
    PowerSwitchState.gubCurrent = powerSwitches[2]->getChannel2Current();

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Switch 0 Channel 2: %d\r\nSwitch 1 Channel 2: %d\r\nSwitch 2 Channel 2: %d\r\n", PowerSwitchState.hibCurrent, PowerSwitchState.hudlCurrent, PowerSwitchState.gubCurrent);

    PowerSwitchState.switch0Temp = powerSwitches[0]->getTempandFault();
    PowerSwitchState.switch1Temp = powerSwitches[1]->getTempandFault();
    PowerSwitchState.switch2Temp = powerSwitches[2]->getTempandFault();

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Switch 0 Temperature: %d\r\nSwitch 1 Temperature: %d\r\nSwitch 2 Temperature: %d\r\n", PowerSwitchState.switch0Temp, PowerSwitchState.switch1Temp, PowerSwitchState.switch2Temp);
}

}// namespace LVSS