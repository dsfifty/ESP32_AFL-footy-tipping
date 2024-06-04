![20240604_191033](https://github.com/dsfifty/ESP32_AFL-footy-tipping/assets/113217855/4acc1aed-35a7-437c-a881-e3e274a0f855)
![20240525_180023](https://github.com/dsfifty/AFL-footy-tipping/assets/113217855/df26ea26-1a8d-4079-a39f-9f8978f0faf6)
![20240530_173848](https://github.com/dsfifty/ESP32_AFL-footy-tipping/assets/113217855/d80f818d-321c-41c2-b45a-296764771463)
![1000019147](https://github.com/dsfifty/AFL-footy-tipping/assets/113217855/1454fe34-b86a-425d-a928-043ecc0a6159)
![1000019144](https://github.com/dsfifty/AFL-footy-tipping/assets/113217855/f72631a0-d818-4583-aeb6-28e90580bbfb)

# ESP32 AFL-footy-tipping
An esp32 project that works with a TFT screen to collect AFL footy tips and then update scores and your tip results. 

Written in the Arduino IDE.

Comments in the code might help understand what is happening.
Special thanks to the ArduinoJSON assistant v7

Accesses api.squiggle.com.au

This code uses the TFT_eSPI library. In this library you have to set up the User.Setup.h file in the library folder according to your own connections.

If you have your screen working properly using the TFT_eSPI library, then this code is good to go (nearly).

![20240526_180356](https://github.com/dsfifty/ESP32_AFL-footy-tipping/assets/113217855/4c5ebc90-3dc2-4c4b-8385-05cf9c4b775f)
![20240602_144637](https://github.com/dsfifty/ESP32_AFL-footy-tipping/assets/113217855/36b1263e-fdda-40ad-bc07-cd6fb136e9f7)

NOTES: 
 - All versions prior to v1.1 run on a 320x240 screen (ILI9341).
 - Versions v1.1 and after are on a 320x480 screen with touch (ILI9488).
 - Versions v1.1 and after require Free_Fonts.h (included) to be in the same directory as the program file.
 - Versions v1.12 and later require logo.h to be in the same directory as the program file.

Report any issues and I will try to help.

![pinout](https://github.com/dsfifty/ESP32_AFL-footy-tipping/assets/113217855/e2c9d308-8198-45d3-8c15-8f91531ebba1)

How to use...

1. Use the menu to select a tipping source (api.squiggle.com.au/?=source)
2. Before Wednesday at 8pm, you can change your tipping source (This is the time I set my tips)
3. After Wednesday at 8pm, you can change the source, but only for observation. When watching games, your saved tips will count.
4. When the round finishes, change the round in the menu and touch the set button. This will set the new current round.
5. At any time, you can check the results from any year, any round (that the API supports) using the menu.
6. Set your WiFi credentials in the code, including home, your phone and up to four other options (User 1-4)
7. Set your timezone according to time zone rules in the code. Also add your relative time difference from Sydney in minutes in the line underneath. (eg Adelaide is -30)
8. If you want to change the theme colours, do so in the code. (at the beginning of the sketch, then also at the beginning of menuScreen() and getLadder()
