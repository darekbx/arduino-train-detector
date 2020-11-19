# Train detector

**In progress**

Project uses vibration sensor to detect if train is passing. 
Box with board, battery and sensor is placed on the rail with magnets.

Events will be saved in board internal EEPROM memory, unitil is full.
 
### Pinouts
* PIN 1 - Build in led used to notify state (blinks with 60s interval).
  * When memory is full or board is in readonly mode, the led is turned permamently on. 
* PIN 2 - Sensor pin (Grove SW-420).
* PIN 3 - Readonly switch (when is in HIGHT state, then data is not collected).
