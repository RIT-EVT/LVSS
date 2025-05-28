# REV3-LVSS

## Introduction
The Low Voltage Sub-System (LVSS) is a board designed to receive CANopen messages from the Vehicle Control Unit (VCU), indicating which boards on the bike needs to be turned on or off. The LVSS will include an STM32F446RE that works with a DCM4623TD2K13E0T70 Vicor DCM Power Module to take the battery pack voltage and step it down to multiple 12-volt signals. Utilizing the TPS2HB35BQ high-side power switches, a state machine will be able to send out the 12-volt signals to each individual board. The power switches can sense the current, temperature, and fault status and use these values to ensure the health of the LVSS.
LVSS Node ID = 1