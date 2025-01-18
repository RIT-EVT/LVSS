#ifndef _LVSS_
#define _LVSS_

#include <EVT/dev/LCD.hpp>
#include <EVT/io/CANopen.hpp>
#include <EVT/io/GPIO.hpp>
#include <EVT/io/SPI.hpp>
#include <EVT/utils/log.hpp>
#include <LVSS.hpp>
#include <cstdio>
#include <cstring>
#include <dev/ACS71240.hpp>
#include <dev/TPS2HB50BQ1.hpp>

namespace IO = EVT::core::IO;
namespace DEV = EVT::core::DEV;
namespace time = EVT::core::time;

namespace LVSS {

static constexpr uint8_t POWER_SWITCHES_SIZE = 3;

/**
 * This is an example of a class for a board
 */
class LVSS : public CANDevice {
public:
    static constexpr uint8_t NODE_ID = 42;
    static constexpr uint8_t TPDO_NODE_ID = 1;

    /** Union bit field to hold a bit representing which boards are on/off */
    typedef union {
        struct {
            uint8_t tms : 1;
            uint8_t hib : 1;
            uint8_t gub : 1;
            uint8_t hudl: 1;
            uint8_t acc : 1;
            uint8_t batt: 1;
        };
        uint16_t val;
    } u_t;

    /** FSM State declaration */
    enum class State {
        INITIALIZATION = 0, /* When LVSS is powered on */
        SOFT_START     = 1, /* Reads VCU signal and starting data liike switch current */
        POWER_UP       = 2, /* Turns on the specified boards */
        IDLE           = 3, /* Checks values for errors */
    };

    /**
     * Constructor for the LVSS class, takes a pointer to an array of power switches
     * @param powerSwitches an array of pointers to power switches
     */
    explicit LVSS(TPS2HB50BQ1* powerSwitches[POWER_SWITCHES_SIZE]);

    CO_OBJ_T* getObjectDictionary() override;

    uint8_t getNumElements() override;

    uint8_t getNodeID() override;

    /**
     * Reads a CANopen message from VCU and assigns it to a variable
     */
    void setBoardEnable();
    uint8_t getBoardEnable();

    /** Returns high value current */
    uint16_t getHVCurrent();

    /** Returns any errors found */
    uint16_t getErrorStatus();

    /** Returns the current of the power switches */
    uint16_t getSwitchCurrent();

    /**
     * Handle running the core logic of the LVSS
     */
    void process();

private:
    // false = OFF, true = ON?

    TPS2HB50BQ1* powerSwitches[POWER_SWITCHES_SIZE]{};// a struct for each power switch (of which there are 3)

    /**
     * The current state of the LVSS
     */
    State state = State::INITIALIZATION;

    /**
     * Boolean flag which represents that a state has just changed
     *
     * Useful for determining when operations that only take place once per
     * state change should take place.
     */
    bool isNewState = false;

    /**
     * Handle VICOR Initialization state
     * Waits for 110ms while the VICOR initializes.
     *
     * State: State::INITIALIZATION
     */
    void initState();

    /**
     * Handle VICOR soft start state
     * Reads in CANopen messages from the VCU of which boards to turn on/ff.
     *
     * State: State::SOFT_START
     */
    void softStartState();

    /**
     * Handle VICOR when the LVSS sends out power to the boards
     * Determines which boards get turned on/off first based on priority.
     *
     * State: State::POWER_UP
     */
    void powerUpState();

    /**
     * Handle errors
     *
     * State: State::IDLE
     */
    void idleState();

    u_t boardEN;

    /** Holds signal from VCU */
    uint16_t VCUBoardSig;
    uint16_t highValCurrent = 1;

    /** Holds power switch current */
    uint16_t swCurrent      = 2;

    /**
     * Have to know the size of the object dictionary for initialization
     * process.
     */
    static constexpr uint8_t OBJECT_DICTIONARY_SIZE = 40;

    /**
     * The object dictionary itself. Will be populated by this object during
     * construction.
     *
     * The plus one is for the special "end of dictionary" marker.
     */
    CO_OBJ_T objectDictionary[OBJECT_DICTIONARY_SIZE + 1] = {
        MANDATORY_IDENTIFICATION_ENTRIES_1000_1014,
        HEARTBEAT_PRODUCER_1017(2000),
        IDENTITY_OBJECT_1018,
        SDO_CONFIGURATION_1200,

        RECEIVE_PDO_SETTINGS_OBJECT_140X(0x00, 0x00, TPDO_NODE_ID, RECEIVE_PDO_TRIGGER_ASYNC),
        RECEIVE_PDO_MAPPING_START_KEY_16XX(0x00, 0x1),
        RECEIVE_PDO_MAPPING_ENTRY_16XX(0x00, 0x01, PDO_MAPPING_UNSIGNED16),

        TRANSMIT_PDO_SETTINGS_OBJECT_18XX(0x00, TRANSMIT_PDO_TRIGGER_TIMER, TRANSMIT_PDO_INHIBIT_TIME_DISABLE, 2000),
        TRANSMIT_PDO_MAPPING_START_KEY_1AXX(0x00, 0x02),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x00, 0x01, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x00, 0x02, PDO_MAPPING_UNSIGNED16),

        // User defined data, this will be where we put elements that can be
        // accessed via SDO and depending on configuration PDO
        DATA_LINK_START_KEY_21XX(0x00, 0x03),
        /** Receive data */
        DATA_LINK_21XX(0x00, 0x01, CO_TUNSIGNED16, &VCUBoardSig),

        /** Transfer data */
        DATA_LINK_21XX(0x00, 0x02, CO_TUNSIGNED16, &highValCurrent),
        DATA_LINK_21XX(0x00, 0x03, CO_TUNSIGNED16, &swCurrent),


        // End of dictionary marker
        CO_OBJ_DICT_ENDMARK,
    };
};

}// namespace LVSS
#endif