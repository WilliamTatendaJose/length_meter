# Pipe Length Measurement Project

This ESP32 project measures the length of a pipe using an encoder and displays the information on an LCD screen. The project includes features for setting target lengths, configuring encoder parameters, and calibrating the system.

## Components

- ESP32 Board
- Rotary Encoder
- LCD Display (I2C)
- Buttons (Menu, Increment, Decrement, Save, Back, Alt)
- Buzzer
- EEPROM (emulated in flash memory)

## Pin Configuration

- **Encoder Pins:**
  - ENCODER_A: 34
  - ENCODER_B: 35

- **Button Pins:**
  - BTN_MENU: 32
  - BTN_INC: 33
  - BTN_DEC: 25
  - BTN_SAVE: 26
  - BTN_BACK: 27
  - BTN_ALT: 14

- **Buzzer Pin:**
  - BUZZER: 12

## Features

- **Main Menu:**
  - Displays the current length and target length.

- **Set Target:**
  - Allows setting a target length.

- **Configure Encoder:**
  - Allows configuring pulses per revolution (PPR) and wheel circumference.

- **Calibrate:**
  - Allows calibrating the encoder for accurate measurements.

- **Save Settings:**
  - Saves the current settings to EEPROM (emulated in flash memory).

## Usage

1. **Setup:**
   - Connect the components as per the pin configuration.
   - Upload the `pipe_length.ino` code to the ESP32 board.

2. **Operation:**
   - Use the Menu button to navigate through different menus.
   - Use the Increment and Decrement buttons to adjust values.
   - Use the Save button to save the settings.
   - Use the Back button to return to the previous menu.
   - Use the Alt button to switch between configuration options or calibration steps.

3. **Calibration:**
   - Follow the on-screen instructions to calibrate the encoder.

## Optimizations Made

1. **FreeRTOS Tasks:**
   - Utilized FreeRTOS tasks to handle encoder processing and HMI updates concurrently, improving responsiveness and efficiency.

2. **EEPROM Emulation:**
   - Used a portion of the ESP32's flash memory to emulate EEPROM, allowing persistent storage of configuration settings.

3. **Interrupt Service Routines (ISRs):**
   - Implemented ISRs for handling button presses and encoder changes, ensuring quick and efficient response to user inputs.

4. **Menu Navigation:**
   - Improved menu navigation logic to ensure smooth transitions between different states and accurate updates on the LCD screen.

5. **Calibration Process:**
   - Enhanced the calibration process to allow easy and accurate calibration using a known distance, ensuring precise measurements.

## License

This project is licensed under the MIT License.
