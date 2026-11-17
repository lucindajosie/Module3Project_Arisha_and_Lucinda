# Flip n Feed
Arisha and Lucinda's Module 3 Project

🌭 A two-player cooperative pinball game to land as many hot dogs as possible in the man's mouth 🌭

Using the starter from this module, our goal was to flip it into a game neither player can win alone. Each player controls one plate, and early/late flips pass the hot dog to your partner instead of going to waste. Players figure out how to maximize feeding the man by working together. 

Arisha's Blog Post: 
Lucinda's Blog Post: 

## How it works
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
* Download the project folder and open `feed_the_man/feed_the_man.ino` in Arduino IDE
* Set up TFT_eSPI for the T-Display: in `User_Setup_Select.h`, use `Setup25_TTGO_T_Display.h` instead of the default `User_Setup.h`
* Set `TEAM_ID` to any number (both boards must match) and `PLAYER_ID` to 0
* Select ESP32 Dev Module as the board, select your port, and upload to the first ESP32
* Change `PLAYER_ID` to 1 and upload to the second ESP32, reselecting the port

## Gameplay
* Press either button on your board to flip your plate to launch hot dogs into the man's mouth.
* After dropping three hot dogs, the game is over. Press any button to play again.
* Tip: Experiment with launching from different parts of the plate to see how that affects the hot dog's trajectory.

## Acknowledgments
* Starter code provided by Professor Tseng, bccoms3930 module 3
* AI Acknowledgment: Claude (Anthropic), Claude Opus 5.5

We used Claude to help debug some code and help implementing the physics for the game. We tested, edited, and designed everything as well as made every game rule decisions. More details are in the blog post.
