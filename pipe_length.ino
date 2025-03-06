#include <Wire.h>
#include "LiquidCrystal_I2C.h"
#include <EEPROM.h>

// LCD Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);

// EEPROM Memory Addresses
#define EEPROM_PPR_ADDR 0
#define EEPROM_CIRC_ADDR 4
#define EEPROM_TARGET_ADDR 8

// Encoder Pins
#define ENCODER_A 34
#define ENCODER_B 35

// Button Pins
#define BTN_MENU 32
#define BTN_INC 33
#define BTN_DEC 25
#define BTN_SAVE 26
#define BTN_BACK 27
#define BTN_ALT 14

// Buzzer Pin
#define BUZZER 12

// Encoder Variables
volatile long encoderCount = 0;
float pulsesPerRev;
float wheelCircumference;
float targetDistance;
bool targetReached = false;
float lastDisplayedDistance = -1;  // Stores last displayed distance

// Menu States
enum MenuState { MAIN,
                 SET_TARGET,
                 CONFIG_ENCODER,
                 CALIBRATE,
                 SAVE_SUCCESS };
volatile MenuState menuState = MAIN;
volatile MenuState previousMenuState = MAIN;  // Stores the previous menu state
volatile bool menuChanged = true;             // Flag for menu changes

// Configuration Options
enum ConfigOption { CONFIG_PPR,
                    CONFIG_CIRC };
volatile ConfigOption configOption = CONFIG_PPR;  // Tracks which config option is being edited

// Calibration Variables
enum CalibrationStep { CALIBRATE_PPR,
                       CALIBRATE_CIRC };
volatile CalibrationStep calibrationStep = CALIBRATE_PPR;
volatile long calibrationStartCount = 0;   // Encoder count at the start of calibration
volatile float calibrationDistance = 1.0;  // Known distance for circumference calibration (in meters)

// Save Success Message Timer
unsigned long saveMessageStartTime = 0;
const unsigned long saveMessageDuration = 2000;  // Display message for 2 seconds

// Function prototypes
void IRAM_ATTR readEncoder();
void IRAM_ATTR handleMenuButton();
void IRAM_ATTR handleIncButton();
void IRAM_ATTR handleDecButton();
void IRAM_ATTR handleSaveButton();
void IRAM_ATTR handleBackButton();
void IRAM_ATTR handleAltButton();

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  EEPROM.begin(12);

  // Load stored values from EEPROM
  EEPROM.get(EEPROM_PPR_ADDR, pulsesPerRev);
  EEPROM.get(EEPROM_CIRC_ADDR, wheelCircumference);
  EEPROM.get(EEPROM_TARGET_ADDR, targetDistance);

  if (isnan(pulsesPerRev) || pulsesPerRev < 100) pulsesPerRev = 1000;
  if (isnan(wheelCircumference) || wheelCircumference < 0.01) wheelCircumference = 300;
  if (isnan(targetDistance) || targetDistance < 0.1) targetDistance = 100.0;

  // Encoder Setup
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), readEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), readEncoder, CHANGE);

  // Button Setup with Interrupts
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_INC, INPUT_PULLUP);
  pinMode(BTN_DEC, INPUT_PULLUP);
  pinMode(BTN_SAVE, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_ALT, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BTN_MENU), handleMenuButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_INC), handleIncButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_DEC), handleDecButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_SAVE), handleSaveButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_BACK), handleBackButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_ALT), handleAltButton, FALLING);
  // Buzzer Setup
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // Create Tasks
  xTaskCreatePinnedToCore(encoderTask, "EncoderTask", 1000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(hmiTask, "HMITask", 2000, NULL, 1, NULL, 1);

  updateDisplay();
}

void loop() {
  vTaskDelay(portMAX_DELAY);  // Loop not used since tasks handle everything
}

// Encoder Interrupt Service Routine
void IRAM_ATTR readEncoder() {
  int MSB = digitalRead(ENCODER_A);
  int LSB = digitalRead(ENCODER_B);
  int encoded = (MSB << 1) | LSB;
  static int lastEncoded = 0;
  int sum = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    encoderCount++;  // Clockwise
  }
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    encoderCount--;  // Counter-clockwise
  }

  lastEncoded = encoded;
}

// Menu Button Interrupt Service Routine
void IRAM_ATTR handleMenuButton() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 200) {
    // Cycle through menu states
    menuState = (MenuState)((menuState + 1) % 4);
    if (menuState == SAVE_SUCCESS) {
      menuState = (MenuState)((menuState + 1) % 4);
    }
    menuChanged = true;
  }
  lastInterruptTime = interruptTime;
}


void IRAM_ATTR handleAltButton() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 200) {
    if (menuState == CONFIG_ENCODER) {
      // Cycle through configuration options
      configOption = (ConfigOption)((configOption + 1) % 2);
      menuChanged = true;
    } else if (menuState == CALIBRATE) {
      // Cycle through calibration steps
      calibrationStep = (CalibrationStep)((calibrationStep + 1) % 2);
      menuChanged = true;
    }
  }
  lastInterruptTime = interruptTime;
}

// Increment Button Interrupt Service Routine
void IRAM_ATTR handleIncButton() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 200) {
    if (menuState == SET_TARGET) {
      targetDistance += 0.1;
    } else if (menuState == CONFIG_ENCODER) {
      if (configOption == CONFIG_PPR) {
        pulsesPerRev += 50;
      } else {
        wheelCircumference += 0.01;
      }
    } else if (menuState == CALIBRATE) {
      if (calibrationStep == CALIBRATE_PPR) {
        // Start PPR calibration
        calibrationStartCount = encoderCount;
      } else if (calibrationStep == CALIBRATE_CIRC) {
        // Start circumference calibration
        calibrationStartCount = encoderCount;
      }
    }
  }
  lastInterruptTime = interruptTime;
}

// Decrement Button Interrupt Service Routine
void IRAM_ATTR handleDecButton() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 200) {
    if (menuState == SET_TARGET) {
      targetDistance -= 0.1;
      if (targetDistance < 0) targetDistance = 0;
    } else if (menuState == CONFIG_ENCODER) {
      if (configOption == CONFIG_PPR) {
        pulsesPerRev -= 100;
        if (pulsesPerRev < 100) pulsesPerRev = 100;
      } else {
        wheelCircumference -= 0.01;
        if (wheelCircumference < 0.01) wheelCircumference = 0.01;
      }
    } else if (menuState == CALIBRATE) {
      if (calibrationStep == CALIBRATE_PPR) {
        // Reset PPR calibration
        pulsesPerRev = 1000;  // Default value
      } else if (calibrationStep == CALIBRATE_CIRC) {
        // Reset circumference calibration
        wheelCircumference = 0.314;  // Default value
      }
    }
  }
  lastInterruptTime = interruptTime;
}

// Save Button Interrupt Service Routine
void IRAM_ATTR handleSaveButton() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 200) {
    if (menuState == CALIBRATE) {
      // Calculate pulses per revolution based on the distance traveled
      float distanceTraveled = (encoderCount - calibrationStartCount) * wheelCircumference;
      pulsesPerRev = calibrationDistance / distanceTraveled;
    }
    saveSettings();
    //previousMenuState = menuState; // Store the current state
    menuState = SAVE_SUCCESS;         // Show save success message
    saveMessageStartTime = millis();  // Start the message timer
    menuState = MAIN;
    menuChanged = true;
  }
  lastInterruptTime = interruptTime;
}

// Back Button Interrupt Service Routine
void IRAM_ATTR handleBackButton() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 200) {
    menuState = (MenuState)((menuState - 1) % 4);
    menuChanged = true;
  }
  lastInterruptTime = interruptTime;
}


// Background task for encoder processing on Core 0
void encoderTask(void *parameter) {
  while (true) {
    float distanceTraveled = (encoderCount / pulsesPerRev) * wheelCircumference;

    if (distanceTraveled >= targetDistance && !targetReached) {
      digitalWrite(BUZZER, HIGH);
      vTaskDelay(500 / portTICK_PERIOD_MS);
      digitalWrite(BUZZER, LOW);
      targetReached = true;
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);  // Avoid CPU overload
  }
}

// HMI Task on Core 1
void hmiTask(void *parameter) {
  while (true) {
    updateDisplay();
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Update every 100ms
  }
}

// Update LCD only when necessary
void updateDisplay() {
  float distanceTraveled = (encoderCount / pulsesPerRev) * wheelCircumference;

  if (menuChanged) {
    lcd.clear();
    menuChanged = false;
  }

  if (menuState == MAIN) {

    lcd.setCursor(0, 0);
    lcd.print("Length: ");
    lcd.print(distanceTraveled, 2);
    lcd.print("m ");

    lcd.setCursor(0, 1);
    lcd.print("Target: ");
    lcd.print(targetDistance, 2);
    lcd.print("m ");


  } else if (menuState == SET_TARGET) {
    lcd.setCursor(0, 0);
    lcd.print("Set Target:");
    lcd.setCursor(0, 1);
    lcd.print(targetDistance, 2);
    lcd.print("m");
  } else if (menuState == CONFIG_ENCODER) {
    lcd.setCursor(0, 0);
    lcd.print(configOption == CONFIG_PPR ? "PPR: " : "Circum: ");
    lcd.print(configOption == CONFIG_PPR ? pulsesPerRev : wheelCircumference, 2);
    lcd.setCursor(0, 1);
    lcd.print(configOption == CONFIG_PPR ? "Circum: " : "PPR: ");
    lcd.print(configOption == CONFIG_PPR ? wheelCircumference : pulsesPerRev, 2);
  } else if (menuState == CALIBRATE) {
    lcd.setCursor(0, 0);
    lcd.print("Calibrate:");
    lcd.setCursor(0, 1);
    lcd.print(distanceTraveled, 2);
    lcd.print("m");
  } else if (menuState == SAVE_SUCCESS) {
    lcd.setCursor(0, 0);
    lcd.print("Save Successful!");
    lcd.setCursor(0, 1);
    lcd.print("Returning...");

    // Revert to the previous state after the message duration
    if (millis() - saveMessageStartTime >= saveMessageDuration) {
      menuState = MAIN;
      menuChanged = true;
    }
  }
}

void saveSettings() {
  EEPROM.put(EEPROM_PPR_ADDR, pulsesPerRev);
  EEPROM.put(EEPROM_CIRC_ADDR, wheelCircumference);
  EEPROM.put(EEPROM_TARGET_ADDR, targetDistance);
  EEPROM.commit();
}
