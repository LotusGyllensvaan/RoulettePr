# Roulette
Arduino Roulette Game
This Arduino project implements a simple roulette game with a graphical user interface on an OLED display and a simulated spinning wheel using NeoPixel LEDs. The game includes various betting options and a debug mode for testing.

Features
Graphical OLED Display: Utilizes the Adafruit SSD1306 library to render a menu-based interface on a 128x64 OLED screen.

Rotary Encoder Control: Allows users to navigate through menus using a rotary encoder with a built-in button.

NeoPixel LED Wheel: Simulates the spinning roulette wheel with a ring of NeoPixel LEDs (Adafruit NeoPixel library).

Betting Options: Users can place bets on different outcomes (Green, Odd, Even, White) with corresponding visual feedback.

Sound Effects: The project includes a buzzer (connected to pin 3) for playing sound effects during gameplay.

Hardware Requirements
Arduino board (tested on [specific board])
OLED display (Adafruit SSD1306)
Rotary encoder with button
NeoPixel LED ring (Adafruit NeoPixel, GRB, 800 KHz)
Buzzer (connected to pin 3)
Dependencies
Adafruit GFX Library
Adafruit SSD1306 Library
Adafruit NeoPixel Library
InterpolationLib (included in the project)
Arduino SPI Library
Arduino Wire Library
Pin Configuration
OLED:

SDA: A4
SCL: A5
RESET: -1
Rotary Encoder:

CLK: 7
DT: 8
SW: 9
NeoPixel LED Ring:

LED_PIN: 6
LED_COUNT: 24
Buzzer:

BUZZER_PIN: 3
Usage
Connect the hardware components following the specified pin configuration.
Install the required libraries in the Arduino IDE.
Upload the provided code to your Arduino board.
Power on the Arduino to start the roulette game.
Use the rotary encoder to navigate through the menu options and place bets.
Press the encoder button to make selections.
Enjoy the simulated roulette game with visual and sound effects.
Note
The code includes a debug function (debug()) that guarantees a win for testing purposes. Remove or modify this function for normal gameplay.
Feel free to customize and expand upon this project for your own Arduino gaming experience.