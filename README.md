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

Report any issues and I will try to help

Footy+API v1.02
~~~~~~~~~~~~~~~
  - Added font/background colour options toward the beginning.
  - Removed much of the code from loop() and setup() and put them in functions
  - Changed the cutoff time for accessing tips from the API to 8pm Wednesday
  - Added a small notificaion bottom right if the tips are coming from the API

Footy_API v1.01
~~~~~~~~~~~~~~
Added a way to save the footy tips after Wednesday so that if the API changes the tips, my registered tips are still used.
  - required LittleFS
  - required time and ntp functions
  - added a startup screen
  - tidied up the serial port output

Footy_API v1.00
~~~~~~~~~~~~~~
The first version to retrieve live tips and compare them with the actual scores, giving a tally.
  - changed the variables to char[] from const char* so that they didn't get lost when the JSON doc got destroyed.
