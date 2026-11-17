# Flip n Feed
Arisha and Lucinda's Module 3 Project
🌭 A two-player cooperative pinball game to land as many hot dogs as possible in the man's mouth 🌭

Using the starter from this module, our goal was to flip it into a game neither player can win alone. Each player controls one plate, and early/late flips pass the hot dog to your partner instead of going to waste. Players figure out how to maximize feeding the man by working together. 

Arisha's Blog Post: 
Lucinda's Blog Post: 

##How it works

Two boards talk to each other wirelessly over ESP-NOW.

* Player 0 (host) runs the game and controls the left plate. 
* Player 1 controls the right plate and shows the host's game on its screen. 

Both boards run the same sketch. Set (PLAYER_ID)  to choose each board's role.

## Components
* 2 x ESP32
* 2 x usb-c cable
* Arduino IDE
* ESP32 board package (esp32 by Espressif) - includes WiFi and ESP-NOW libraries
* TFT_eSPI library

## Setup
* Download the project folders with the green code button
* Configure TFT_eSPI for the T-Display: in the library's `User_Setup_Select.h`, comment out `#include <User_Setup.h>` and uncomment `#include <User_Setups/Setup25_TTGO_T_Display.h>`
* Open 'flip_n_feed.ino' in Arduino IDE:
```
open flip_n_feed.ino
```
* Set PLAYER_ID variable to 0 for one user and 1 for the other user
* Set PLAYER_ID variable to 0 for one user and 1 for the other user
* Connect your ESP32 to your computer with a usb-c cable
* Select your board (Tools > Boards > esp32 > ESP32 Dev Module)
* Select your port (Tools > Port > _your_port_)
* Press Upload in the top left corner

## Gameplay
* Use your plate to launch hot dogs into the man's mouth
* After dropping three hot dogs, the game is over
* Experiment with launching from different parts of the plate to see how that affects the hot dog's trajectory
