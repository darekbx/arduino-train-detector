/**
   Train detector

   Project uses vibration sensor to detect if train is passing.
   Box with board, battery and sensor is placed on the rail with magnets.

   Events will be saved in board internal EEPROM memory, until is full.

   Pinouts:
   5 - External led used to notify state (blinks with 60s interval).
       - When board is in readonly mode, led blikns in 250ms interval.
       - When memory is full, led blikns in 1000ms interval.

   Sensor uses LIS3DH accelerometer to detect passing train, accelerometer is connected by I2C.
*/
#include <SparkFunLIS3DH.h>
#include <Wire.h>
#include <EEPROM.h>

#define clearMemory false
#define emulateEEPROM false
#define serialEnabled false
#define isReadOnly false

#define ledPin 5

#define second 1000
#define hour 60 * 60 * 1000
#define eventThreadDelay 20     // Event loop delay
#define blinkInterval 60        // Led blink interval in seconds
#define eventsDelay 30          // Delay between events, 60 seconds
#define counterIncreaseRatio 1  // How many ticks are needed to increase counter by one second
#define startDelay 10 * second  // Wait 10 seconds before start collecting data

#define secondsCounterAddress 0   // Seconds counter address
#define eventIndexAddress 4       // Address of event index

unsigned long secondsCounter = 0;
unsigned long timer = 0;
unsigned long previousMillis = 0;
unsigned long previousEventMillis = 0;

int memorySize = 0;             // Size of the board EEPROM
int eventAddress = 8;           // Initial addres of the first event
int eventAddressStep = 4;       // Event address step

boolean hasEvent = false;
int eventTime = 0;

LIS3DH accelerometer(I2C_MODE, 0x18);
float movementScale = 0.1;

void setup() {
  
  pinMode(ledPin, OUTPUT);

  #if clearMemory
    eraseEEPROM();
    #endif

  EEPROM.begin();
  memorySize = EEPROM.length();

  initializeEventAddress();
  initializeSecondsCounter();

  #if serialEnabled
    Serial.begin(9600);
    while (!Serial) {
      ;
    }
    #endif
  
  accelerometer.settings.accelSampleRate = 50;
  accelerometer.settings.accelRange = 2;
  accelerometer.begin();
  
  #if isReadOnly
    printStatus();
    #endif

  // Turn on status led at start
  digitalWrite(ledPin, HIGH);
  delay(startDelay);
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
    Serial.println("TRAIN DETECTOR");
    Serial.print("Memory size: ");
    Serial.println(memorySize);
    Serial.print("Actual timestamp: ");
    Serial.println(secondsCounter);
    Serial.print("Actual event address: ");
    Serial.println(eventAddress);
    printEEPROM();
    #endif
}

void loop() {
  if (isReadOnly) {
    #if serialEnabled
      Serial.println("READ ONLY MODE");
      #endif
    digitalWrite(ledPin, HIGH);
    delay(250);
    digitalWrite(ledPin, LOW);
    delay(250);
    return;
  }

  if (isMemoryFull()) {
    #if serialEnabled
      Serial.println("MEMORY IS FULL!");
      #endif
    digitalWrite(ledPin, HIGH);
    delay(1000);
    digitalWrite(ledPin, LOW);
    delay(1000);
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
  float accelerometerX = accelerometer.readFloatAccelX();
  boolean hasMovement = 
    (accelerometerX < (-1 - movementScale)) || 
    (accelerometerX > (-1 + movementScale));

  if (!hasEvent && hasMovement) {
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
      Serial.print("Event occurred at: ");
      Serial.print(secondsCounter);
      #endif
  }
}

void increaseSeconds() {
  secondsCounter++;

  #if serialEnabled
    Serial.print("Timestamp: ");
    Serial.println(secondsCounter);
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
  if (emulateEEPROM) {
    Serial.println("eraseEEPROM");
    return;
  }
  #if clearMemory
    for (int i = 0; i < EEPROM.length(); i++) {
      EEPROM.write(i, 0);
    }
    #endif
}

void printEEPROM() {
  if (emulateEEPROM) {
    Serial.println("printEEPROM");
    return;
  }
  #if serialEnabled
    int zeroCount = 0;
    for (long i = 0; i < EEPROM.length(); i += 4) {
      long value = readLong(i);
      Serial.print("[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(value);
  
      if (value == 0 || value == -1) {
        zeroCount++;
      } else {
        zeroCount--;
      }
  
      if (zeroCount > 4 /* Long size */) {
        Serial.println("...");
        break;
      }
    }
    #endif
}

void writeLong(long address, long number) {
  if (emulateEEPROM) {
    #if serialEnabled
      Serial.print("Write [");
      Serial.print(address);
      Serial.print("] = ");
      Serial.println(number);
      #endif
    return;
  }
  EEPROM.write(address, (number >> 24) & 0xFF);
  EEPROM.write(address + 1, (number >> 16) & 0xFF);
  EEPROM.write(address + 2, (number >> 8) & 0xFF);
  EEPROM.write(address + 3, number & 0xFF);
}

long readLong(long address) {
  if (emulateEEPROM) {
    #if serialEnabled
      Serial.print("Read ");
      Serial.println(address);
      #endif
    return address;
  }
  return ((long)EEPROM.read(address) << 24) +
         ((long)EEPROM.read(address + 1) << 16) +
         ((long)EEPROM.read(address + 2) << 8) +
         (long)EEPROM.read(address + 3);
}
