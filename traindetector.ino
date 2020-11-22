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

#define second 1000
#define hour 60 * 60 * 1000
#define ledPin 1
#define sensorPin 2
#define readOnlySwitch 0
#define threadDelay 50                                  // Main loop delay
#define blinkInterval 60                                // Led blink interval in seconds
#define eventsDelay (second / threadDelay) * 60         // Delay between events, 10 seconds
#define counterIncreaseRatio (second / threadDelay) * 1 // How many ticks are needed to increase counter by one second

#define secondsCounterAddress 0   // Seconds counter address
#define eventIndexAddress 4       // Address of event index

long secondsCounter = 0;
int timer = 0;

int memorySize = 0;             // Size of the board EEPROM
int eventAddress = 8;           // Initial addres of the first event
int eventAddressStep = 4;       // Event address step

boolean isReadOnly = false;

boolean hasEvent = false;
int eventTime = 0;

void setup()
{
  pinMode(readOnlySwitch, INPUT);
  pinMode(sensorPin, INPUT);
  pinMode(ledPin, OUTPUT);

  if (clearMemory) {
    eraseEEPROM();
  }

  EEPROM.begin();
  memorySize = EEPROM.length();

  initializeEventAddress();
  initialzieSecondsCounter();

  if (serialEnabled) {
    DigiKeyboard.delay(1000);
    DigiKeyboard.sendKeyStroke(0);
  }
  
  if (digitalRead(readOnlySwitch) == HIGH) {
    isReadOnly = true;
    if (serialEnabled) {
      printStatus();
    }
  }
}

void initializeEventAddress() {
  eventAddress = readLong(eventIndexAddress);
  if (eventAddress <= 0) {
    writeLong(eventIndexAddress, eventAddress);
    eventAddress = eventIndexAddress;
  }
}

void initialzieSecondsCounter() {
  secondsCounter = readLong(secondsCounterAddress);
  if (secondsCounter <= 0) {
    writeLong(secondsCounterAddress, 0);
    secondsCounter = 0;
  }
}

void printStatus() {
  DigiKeyboard.println("TRAIN DETECTOR");
  DigiKeyboard.print("Memory size: ");
  DigiKeyboard.println(memorySize);
  DigiKeyboard.print("Actual timestamp: ");
  DigiKeyboard.println(secondsCounter);
  DigiKeyboard.print("Actual event address: ");
  DigiKeyboard.println(eventAddress);
  printEEPROM();
}

void loop()
{
  if (isReadOnly) {
    if (serialEnabled) {
      DigiKeyboard.println("READ ONLY MODE");
    }
    digitalWrite(ledPin, HIGH);
    delay(hour);
    return;
  }

  if (isMemoryFull()) {
    if (serialEnabled) {
      DigiKeyboard.println("MEMORY IS FULL!");
    }
    digitalWrite(ledPin, HIGH);
    delay(hour);
    return;
  }


  handleStatusLed();
  handleSensor();
  handleTimer();

  delay(threadDelay);
}

boolean isMemoryFull() {
  return eventAddress > memorySize;
}

void handleSensor() {
  if (!hasEvent && digitalRead(sensorPin) == LOW) {
    hasEvent = true;
    eventTime = 0;

    // Save in the next free memory cell event time
    eventAddress += eventAddressStep;
    writeLong(eventAddress, secondsCounter);

    // Increase event address index
    writeLong(eventIndexAddress, eventAddress);

    // Blink on new event
    digitalWrite(ledPin, HIGH);

    if (serialEnabled) {
      DigiKeyboard.print("Event occurred at: ");
      DigiKeyboard.print(secondsCounter);
    }
  }

  if (hasEvent) {
    eventTime++;
    if (eventTime > eventsDelay) {
      hasEvent = false;
    }
  }
}

void handleTimer() {

  timer++;

  if (timer >= counterIncreaseRatio) {
    timer = 0;
    secondsCounter++;

    if (serialEnabled) {
      DigiKeyboard.print("Timestamp: ");
      DigiKeyboard.println(secondsCounter);
    }

    writeLong(secondsCounterAddress, secondsCounter);
  }
}

void handleStatusLed() {
  digitalWrite(ledPin, LOW);

  if (secondsCounter % blinkInterval == 0) {
    digitalWrite(ledPin, HIGH);
  }
}

void eraseEEPROM() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
}

void printEEPROM() {
  Serial.println("Memory dump:");
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
