/**
   Train detector

   Project uses vibration sensor to detect if train is passing.
   Box with board, battery and sensor is placed on the rail with magnets.

   Events will be saved in board internal EEPROM memory, until is full.

   Pinouts:
   1 - Build in led used to notify state (blinks with 60s interval).
       - When memory is full or board is in readonly mode, the led is turned permamently on.
   2 - Sensor pin (Grove SW-420).
   0 - Readonly switch (when is in HIGHT state, then data is not collected).

*/
#define kbd_en_us
#include <EEPROM.h>
#include <DigiKeyboard.h>

#define clearMemory false
#define serialEnabled false
#define usesFakeSensor false

#define second 1000
#define hour 60 * 60 * 1000
#define ledPin 1
#define sensorPin 2
#define readOnlySwitch 0
#define eventThreadDelay 20     // Event loop delay
#define blinkInterval 60        // Led blink interval in seconds
#define eventsDelay 30          // Delay between events, 60 seconds
#define counterIncreaseRatio 1  // How many ticks are needed to increase counter by one second

#define secondsCounterAddress 0   // Seconds counter address
#define eventIndexAddress 4       // Address of event index

unsigned long secondsCounter = 0;
unsigned long timer = 0;
unsigned long previousMillis = 0;
unsigned long previousEventMillis = 0;

int memorySize = 0;             // Size of the board EEPROM
int eventAddress = 8;           // Initial addres of the first event
int eventAddressStep = 4;       // Event address step

boolean isReadOnly = false;

boolean hasEvent = false;
int eventTime = 0;

void setup() {
  pinMode(readOnlySwitch, INPUT);
  pinMode(sensorPin, INPUT);
  pinMode(ledPin, OUTPUT);

  #if clearMemory
    eraseEEPROM();
    #endif

  EEPROM.begin();
  memorySize = EEPROM.length();

  initializeEventAddress();
  initializeSecondsCounter();

  #if serialEnabled
    DigiKeyboard.delay(1000);
    DigiKeyboard.sendKeyStroke(0);
    #endif
  
  if (digitalRead(readOnlySwitch) == HIGH) {
    isReadOnly = true;
    if (serialEnabled) {
      printStatus();
    }
  }

  // Turn on status led at start
  digitalWrite(ledPin, HIGH);
}

void initializeEventAddress() {
  eventAddress = readLong(eventIndexAddress);
  if (eventAddress <= 0) {
    writeLong(eventIndexAddress, eventAddress);
    eventAddress = eventIndexAddress;
  }
}

void initializeSecondsCounter() {
  secondsCounter = readLong(secondsCounterAddress);
  if (secondsCounter <= 0) {
    writeLong(secondsCounterAddress, 0);
    secondsCounter = 0;
  }
}

void printStatus() {
  #if serialEnabled
    DigiKeyboard.println("TRAIN DETECTOR");
    DigiKeyboard.print("Memory size: ");
    DigiKeyboard.println(memorySize);
    DigiKeyboard.print("Actual timestamp: ");
    DigiKeyboard.println(secondsCounter);
    DigiKeyboard.print("Actual event address: ");
    DigiKeyboard.println(eventAddress);
    printEEPROM();
    #endif
}

void loop() {
  if (isReadOnly) {
    #if serialEnabled
      DigiKeyboard.println("READ ONLY MODE");
      #endif
    digitalWrite(ledPin, HIGH);
    delay(hour);
    return;
  }

  if (isMemoryFull()) {
    #if serialEnabled
      DigiKeyboard.println("MEMORY IS FULL!");
      #endif
    digitalWrite(ledPin, HIGH);
    delay(hour);
    return;
  }

  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= second) {
    previousMillis = millis();
    increaseSeconds();
    handleStatusLed();
    
    if (hasEvent) {
      eventTime++;
      if (eventTime > eventsDelay) {
        hasEvent = false;
      }
    }
  }
  
  if (currentMillis - previousEventMillis >= eventThreadDelay) {
    previousEventMillis = millis();
    handleSensor();
  }

  delay(1);
}

boolean isMemoryFull() {
  return eventAddress > memorySize;
}

void handleSensor() {
  if (!hasEvent && digitalRead(sensorPin) == (usesFakeSensor ? HIGH : LOW)) {
    hasEvent = true;
    eventTime = 0;

    // Save in the next free memory cell event time
    eventAddress += eventAddressStep;
    writeLong(eventAddress, secondsCounter);

    // Increase event address index
    writeLong(eventIndexAddress, eventAddress);

    // Blink on new event
    digitalWrite(ledPin, HIGH);

    #if serialEnabled
      DigiKeyboard.print("Event occurred at: ");
      DigiKeyboard.print(secondsCounter);
      #endif
  }
}

void increaseSeconds() {
  secondsCounter++;

  #if serialEnabled
    DigiKeyboard.print("Timestamp: ");
    DigiKeyboard.println(secondsCounter);
    #endif

  writeLong(secondsCounterAddress, secondsCounter);
}

void handleStatusLed() {
  digitalWrite(ledPin, LOW);

  if (secondsCounter % blinkInterval == 0) {
    digitalWrite(ledPin, HIGH);
  }
}

void eraseEEPROM() {
  #if clearMemory
    for (int i = 0; i < EEPROM.length(); i++) {
      EEPROM.write(i, 0);
    }
    #endif
}

void printEEPROM() {
  #if serialEnabled
    int zeroCount = 0;
    for (long i = 0; i < EEPROM.length(); i += 4) {
      long value = readLong(i);
      DigiKeyboard.print("[");
      DigiKeyboard.print(i);
      DigiKeyboard.print("] = ");
      DigiKeyboard.println(value);
  
      if (value == 0 || value == -1) {
        zeroCount++;
      } else {
        zeroCount--;
      }
  
      if (zeroCount > 4 /* Long size */) {
        DigiKeyboard.println("...");
        break;
      }
    }
    #endif
}

void writeLong(long address, long number) {
  EEPROM.write(address, (number >> 24) & 0xFF);
  EEPROM.write(address + 1, (number >> 16) & 0xFF);
  EEPROM.write(address + 2, (number >> 8) & 0xFF);
  EEPROM.write(address + 3, number & 0xFF);
}

long readLong(long address) {
  return ((long)EEPROM.read(address) << 24) +
         ((long)EEPROM.read(address + 1) << 16) +
         ((long)EEPROM.read(address + 2) << 8) +
         (long)EEPROM.read(address + 3);
}
