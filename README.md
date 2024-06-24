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
 - Crashes with ESP32 Core 3.0. I can't work out why. I use the previous version 2.0.17.

Report any issues and I will try to help.

How to use...

1. Use the menu to select a tipping source (api.squiggle.com.au/?=source)
2. Once you have chosen your tips and posted them, touch set tips on the menu screen. This will keep your tips even if the API source changes them.
3. After setting the tips, you can change the source, but only for observation. When watching games, your saved tips will count.
4. When the round finishes, change the round in the menu and touch the rnd set button. This will set the new current round.
5. At any time, you can check the results from any year, any round (that the API supports) using the menu.
6. On the main screen, if the LIVE notification is red the HTTP call is not connecting.
7. On the main screen, if the TIPS notification is red, that means the tips are not saved and are coming from the API.
8. Touching the Ladder from the main screen will give you the current ladder.
9. Touching Sources from the menu screen, or tips from the main screen will give you a list of the sources that you can select for your tipping.
10. Set your WiFi credentials in the code, including home, your phone and up to four other options (User 1-4). Changing these in the menu requires a reboot.
11. Set your timezone according to time zone rules in the code. Also add your relative time difference from Sydney in minutes in the line underneath. (eg Adelaide is -30)
12. If you want to change the theme colours, do so in the code. (at the beginning of the sketch, then also at the beginning of menuScreen() and getLadder()
13. You can add your own USERID to the API calls, by changing the ds@ds.com email to your own.
