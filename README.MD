# Train detector

**In progress**

Project uses accelerometer (LIS3DH) to detect if train is passing. 
Box with board, battery and sensor is placed on the rail with magnets.

Events will be saved in board internal EEPROM memory, unitil is full.
 
### Pinouts
* 5 - External led used to notify state (blinks with 60s interval).
    * When board is in readonly mode, led blikns in 250ms interval.
    * When memory is full, led blikns in 1000ms interval.
