/**
 * Train detector
 * 
 * Project uses vibration sensor to detect if train is passing. 
 * Box with board, battery and sensor is placed on the rail with magnets.
 * 
 * Events will be saved in board internal EEPROM memory, unitil is full.
 * 
 * Pinouts:
 * 1 - Build in led used to notify state (blinks with 60s interval). 
 *     - When memory is full or board is in readonly mode, the led is turned permamently on. 
 * 2 - Sensor pin (Grove SW-420).
 * 3 - Readonly switch (when is in HIGHT state, then data is not collected).
 * 
 */
#include <EEPROM.h>

#define serialEnabled true
#define serialPort 9600
#define second 1000
#define hour 60 * 60 * 1000
#define ledPin LED_BUILTIN // TODO: Check if is working with "Digispark kickstarter"
#define sensorPin 2
#define readOnlySwitch 3
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

  //clearMemory();

  memorySize = EEPROM.length();

  eventAddress = readLong(eventIndexAddress);
  if (eventAddress <= 0) {
    writeLong(eventIndexAddress, eventAddress);
    eventAddress = eventIndexAddress;
  }

  secondsCounter = readLong(secondsCounterAddress);
  if (secondsCounter <= 0) {
    secondsCounter = 0;
  }

  if (serialEnabled) {
    Serial.begin(serialPort);
  }
  
  if (digitalRead(readOnlySwitch) == HIGH) {
    isReadOnly = true;
    
    if (serialEnabled) {
      Serial.println("--------------");
      Serial.println("Train detector");
      Serial.println("--------------");
      Serial.print("Memory size: ");
      Serial.println(memorySize);
      Serial.print("Actual timestamp: ");
      Serial.println(secondsCounter);
      Serial.print("Actual event address: ");
      Serial.println(eventAddress);
      printMemoryStatus();
    }
  }
}

void loop()
{
  if (isReadOnly) {
    if (serialEnabled) {
      Serial.println("READ ONLY MODE");
      digitalWrite(ledPin, HIGH);
    }
    delay(hour);
    return;
  }

  if (isMemoryFull()) {
    if (serialEnabled) {
      Serial.println("MEMORY IS FULL!");
      digitalWrite(ledPin, HIGH);
    }
    delay(hour);
    return;
  }

  
  handleSensor();
  handleTimer();
  handleStatusLed();
 
  delay(threadDelay);
}

boolean isMemoryFull() {
    return eventAddress > memorySize;
}

void handleSensor() {
  if (!hasEvent && digitalRead(sensorPin) == HIGH) {
      hasEvent = true;
      eventTime = 0;
    
      // Save in the next free memory cell event time
      eventAddress += eventAddressStep;
      writeLong(eventAddress, secondsCounter);

      // Increase event address index
      writeLong(eventIndexAddress, eventAddress);
      
      if (serialEnabled) {
        Serial.print("Event occurred at: ");
        Serial.print(secondsCounter);
        
        Serial.print(", written to: ");
        Serial.println(eventAddress);
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

  if (timer >= counterIncreaseRatio){
    timer = 0;
    secondsCounter++;
    
    if (serialEnabled) {
      Serial.print("Timestamp: ");
      Serial.println(secondsCounter);
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

void clearMemory() {
   for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
}

void printMemoryStatus() {
  Serial.println("Memory dump:");
  int zeroCount = 0;
  for (long i = 0; i < EEPROM.length(); i += 4) {
    long value = readLong(i);
    Serial.print("[");
    Serial.print(i);
    Serial.print("] = ");
    Serial.println(value);

    if (value == 0) {
      zeroCount++;
    } else {
      zeroCount--;
    }
    
    if (zeroCount > 4 /* Long size */) {
      Serial.println("...");
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
