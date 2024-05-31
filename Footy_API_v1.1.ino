/*
Footy_API_1.1 (320x480)
~~~~~~~~~~~~~~~~~~~~~~~ 
  - adjusted for 320x480 display ILI9488 (fonts, Screen Layout)
  - added menu that will adjust the round, tipping source, year and WiFi source
  - a 'SET' button allows you to set the round as the current round. The current round is saved in LittleFS
  - the tipping source and WiFi settings are saved in LittleFS
  - added six WiFi settings  Home, Phone, User 1-4. These are configurable in the sketch
  - can now scroll through any round/year without upsetting the current round's tips
  - can see which tipping source will tip what. (see https://api.squiggle.com.au/?q=sources)

 Footy_API v1.04
~~~~~~~~~~~~~~~~
  - final update for the 320x240 screen (ILI9341)
  - added two colours for game times so that games days are grouped together.
  - added a line between games/scores to group game days together.
  - added OTA functionality.
  - got rid of delay() function, replacing with millis().

Footy_API v1.03
~~~~~~~~~~~~~~~
  - added a new score colour for game breaks.
  - added game start time if the game is not yet started, adjusted for time zone and 12 hour clock (have to select tz difference from Melbourne).
  - added option to change the tipping source. (options https://api.squiggle.com.au/?q=sources).
  - TO DO: allow for changing the round to check future and past rounds. I want to do this by touch screen. Create a settings menu.
  - tried to add touch screen functionality. Didn't work. Assume screen is not touchable, despite pins being available. New screen ordered.

Footy_API v1.02
~~~~~~~~~~~~~~~
  - Added font/background colour options toward the beginning.
  - Removed much of the code from loop() and setup() and put them in functions.
  - Changed the cutoff time for accessing tips from the API to 8pm Wednesday.
  - Added a small notification bottom right if the tips are coming from the API.

Footy_API v1.01
~~~~~~~~~~~~~~
Added a way to save the footy tips after Wednesday so that if the API changes the tips, my registered tips are still used.
  - required LittleFS.
  - required time and ntp libraries.
  - added a startup screen.
  - tidied up the serial port output.

Footy_API v1.00
~~~~~~~~~~~~~~
The first version to retrieve live tips and compare them with the actual scores, giving a tally.
  - changed the variables to char[] from const char* so that they didn't get lost when the JSON doc got destroyed.
*/

#include <ArduinoOTA.h>        //OTA library
#include <WiFi.h>              // Wifi library
#include <WiFiClientSecure.h>  // HTTP Client for ESP32 (comes with the ESP32 core download)
#include <ArduinoJson.h>       // Required for parsing the JSON coming from the API calls
#include <TFT_eSPI.h>          // Required library for TFT Dsplay. Pin settings found in User_Setup.h in the library directory
#include <SPI.h>               // Communicate with the TFT
#include "Free_Fonts.h"        // Free fonts library in same directory as sketch
#include <LittleFS.h>          // for saving tips data
#include <time.h>              // for using time functions
#include <sntp.h>              // for synchronising clock online

TFT_eSPI tft = TFT_eSPI();  // Activate TFT device
#define TFT_GREY 0x5AEB     // New colour

#define HOST "api.squiggle.com.au"  // Base URL of the API

char ssid0[] = "YOURSSID1";             // network SSID (Home)
char password0[] = "YOURPASSOWRD1";     // network key (Home)
char ssid1[] = "";                      // network SSID (Phone)
char password1[] = "";                  // network key (Phone)
char ssid2[] = "";                      // network SSID (User 1)
char password2[] = "";                  // network key (User 1)
char ssid3[] = "";                      // network SSID (User 2)
char password3[] = "";                  // network key (User 2)
char ssid4[] = "";                      // network SSID (User 3)
char password4[] = "";                  // network key (User 3)
char ssid5[] = "";                      // network SSID (User 4)
char password5[] = "";                  // network key (User 4)

/* COLOUR OPTIONS: 
*  TFT_BLACK TFT_NAVY TFT_DARKGREEN TFT_DARKCYAN TFT_MAROON TFT_PURPLE TFT_OLIVE
*  TFT_LIGHTGREY TFT_DARKGREY TFT_BLUE TFT_GREEN TFT_CYAN TFT_RED  
*  TFT_WHITE TFT_ORANGE TFT_GREENYELLOW TFT_MAGENTA TFT_YELLOW
*  TFT_PINK TFT_BROWN TFT_GOLD TFT_SILVER TFT_SKYBLUE TFT_VIOLET */
uint16_t titleColor = TFT_GREEN;        // colour of the title. Default: TFT_GREEN
uint16_t teamColor = TFT_YELLOW;        // colour of the teams. Default: TFT_YELLOW
uint16_t pickedColor = TFT_ORANGE;      // colour of the picked teams. Default: TFT_ORANGE
uint16_t tallyColor = TFT_GREEN;        // colour of the tally. Default: TFT_GREEN
uint16_t background = TFT_BLACK;        // colour of the background. Default: TFT_BLACK
uint16_t timeColor1 = TFT_LIGHTGREY;    // colour of the game time 1. Default: TFT_LIGHTGREY Make the same to deactivate
uint16_t timeColor2 = TFT_LIGHTGREY;    // colour of the game time 2. Default: TFT_DARKGREY Make the same to deactivate
uint16_t pickedScoreColor = TFT_GREEN;  // colour of the score - tipped. Default: TFT_GREEN
uint16_t wrongScoreColor = TFT_RED;     // colour of the score - not tipped. Default: TFT_RED
uint16_t liveScoreColor = TFT_CYAN;     // colour of the live score. Default: TFT_GREEN
uint16_t gamebreakColor = TFT_WHITE;    // colour of the live score during a break time. Default: TFT_WHITE
uint16_t notifColor = TFT_CYAN;         // colour of the API notification. Default: TFT_CYAN
uint16_t lineColor = TFT_SKYBLUE;       // colour of the lines that group the days. Default: TFT_BROWN Not Used: TFT_BLACK

// Layout Options
int titleX = 130;      // These are the parameters for the setting out of the display. Title x position
int titleY = 25;       // title y position
int line1Start = 60;   // location of the first line location under the title
int lineSpacing = 25;  // Line spacing
int vsLoc = 185;       // Horizontal location of the "vs."
int hteamLoc = 20;     // Horizontal location of the Home Team
int ateamLoc = 250;    // Horizontal location of the Away Team
int scoreLoc = 398;    // Horizontal location of the score
int timeLoc = 410;     // Horizontal location of the game time
int tallyX = 125;      // Horizontal location of the tips tally
int tallyY = 315;      // Vertical location of the tips tally
int warnX = 108;       // Horizontal location of the startup notification
int warnY = 100;       // Vertical location of the startup notification
int notifX = 460;      // Horizontal location of the API notification
int notifY = 310;      // Horizontal location of the API notification
int LnStart = 405;     // x Start position of lines marking the days
int LnEnd = 460;       // x End position of lines marking the days

// Time servers 1 and 2
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";

//Time Zone
char time_zone[] = "ACST-9:30ACDT,M10.1.0,M4.1.0/3";  // TimeZone rule for Adelaide including daylight adjustment rules
int tz = -30;                                         // Time zone in minutes relative to AEST (Melbourne)

// Request string parts
char requestGames[30] = "/?q=games;year=";  // building blocks of the API URL (less base)
char part2Games[] = ";round=";              // for getting games and scores
char requestTips[40] = "/?q=tips;year=";    // building blocks of the API URL (less base)
char part2Tips[] = ";round=";               // for getting tips
char part3Tips[] = ";source=";              // source is the tipping source. 1 = Squiggle

// Arrays for parsing data
char get_winner[9][30];     // global variable array for the winning team
int get_hscore[9];          // global variable array for the home score
int get_ascore[9];          // global variable array for the away score
int get_complete[9];        // global variable array for the percent complete of each game
char get_roundname[9][10];  // global variable array for the Round Name
char get_hteam[9][30];      // global variable array for the home team
char get_ateam[9][30];      // global variable array for the awat team
long get_id[9];             // global variable array for the game id. used for sorting games into chronological order
int orderG[9];              // global variable array for the reordering the games into chronological order
int orderT[9];              // global variable array for the reordering the games into chronological order
char get_tip[9][30];        // global variable array for the tipped teams
long get_tipId[9];          // global variable array for the game id. used for sorting tips into chronological order
char rawData[8000];         // for storing the tips JSON buffer
char get_gtime[9][20];      // global variable array used for retrieving the game start time
char gameTime[9][15];       // global variable array used for retrieving the game start time, adjusted for time zone and 12 hour clock
char gameDate[9][15];       // for future use - game day of the week

// Global variable list
int picked = 0;                    // counts the successful tips
int total = 0;                     // counts the games in each round
int played = 0;                    // counts the games played in each round
int gameLive = 0;                  // used to test if a game is live to shorten the loop delay
int delLive = 30000;               // delay between API calls if there is a live game (ms)
int delNoGames = 120000;           // delay between API calls if there is no live game (ms)
int del = 0;                       // for working out the delay in the main loop wihtout the delay() function
int weekDay = 10;                  // global variable to hold the day of the week when set to 10, it will wait until the time is retrieved
int hour = 0;                      // global variable to hold the current hour
int tipsLive = 1;                  // global variable 1 = tips are retrieved from API, 0= tips are retrieved from LittleFS
unsigned long previousMillis = 0;  // will store the last time the score was updated
int newdate = 0;                   // global variable for noting the game dates during display routine
int rnd = 0;                       // global variable for round
int currentRnd = 0;                // global variable for the current round
int year = 0;                      // global variable for the year
int currentYear = 0;               // global variable for the current year
int tipSource = 0;                 // global variable for the tipping source (https://api.squiggle.com.au/?q=sources for options)
int wifi = 0;                      // global variable for the wifisettings
char wifihead[8];                  // holds the menu headings for WiFi options
char ssid[20];                     //global variable for the ssid
char password[20];                 // global variable for the WiFi password

WiFiClientSecure client;  // starts the client for HTTPS requests

void setup() {

  Serial.begin(115200);  // for debugging

  if (!LittleFS.begin()) {  //LittleFS set up. Put true first time running on a new esp32 to format the LittleFS
    Serial.println("Error mounting LittleFS");
  }

  sntp_set_time_sync_notification_cb(timeavailable);  // set notification call-back function
  sntp_servermode_dhcp(1);                            // (optional)
  configTzTime(time_zone, ntpServer1, ntpServer2);    // these all need to be done before connecting to WiFi

  readWifi();
  if (wifi == 0) {
    strcpy(ssid, ssid0);
    strcpy(password, password0);
  }
  if (wifi == 1) {
    strcpy(ssid, ssid1);
    strcpy(password, password1);
  }
  if (wifi == 2) {
    strcpy(ssid, ssid2);
    strcpy(password, password2);
  }
  if (wifi == 3) {
    strcpy(ssid, ssid3);
    strcpy(password, password3);
  }
  if (wifi == 4) {
    strcpy(ssid, ssid4);
    strcpy(password, password4);
  }
  if (wifi == 5) {
    strcpy(ssid, ssid5);
    strcpy(password, password5);
  }
  Serial.println(ssid);
  Serial.println(password);
  WiFi.begin(ssid, password);  // Connect to the WiFI
  Serial.println("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {  // Wait for connection
    delay(50);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  ArduinoOTA.begin();  //Start OTA

  client.setInsecure();  //bypasses certificates

  tft.init();                                         // initialises the TFT
  tft.setRotation(3);                                 // sets the rotation to match the device
  uint16_t calData[5] = { 255, 3673, 278, 3500, 1 };  //inuts the calibration data for the particular screen from TFT_eSPI examples
  tft.setTouch(calData);
  startScreenTFT();  // sets out the loading messages in a rectangle while waiting for the time

  Serial.print("Getting time ");  // waits for the interupt function to get the time.
  while (weekDay == 10) {
    Serial.print(".");
    delay(100);
  }
  readSource();
  readRound();

  tft.fillScreen(background);


  if ((weekDay == 1) || (weekDay == 2) || ((weekDay == 3) && hour < 20)) {  // decides whether to use live tips or saved tips
    tipsLive = 1;                                                           // based on the days of the week and hour.
    Serial.println("Getting tips from API and writing to LittleFS");        // 0=Sunday. 24 hour clock
  } else {                                                                  // 1 = get tips from API
    tipsLive = 0;                                                           // 0 = get tips from LittleFS
    Serial.println("Getting tips by reading from LittleFS");
  }
  stringBuild();  // build the strings for the URLs for games, scores and tips using the year and rnd variable.

  tipsRequest();  // calls the function to get the tips

  sortT(get_tipId, total);  // sorts the matches into chronological order using the sortT() function

  gameRequest();  // calls the gameRequest() function

  timeZoneDiff();  // adjusts the game time for time zone and change to 12 hour clock

  sortG(get_id, total);  // sorts the matches into chronological order using the sortG() function

  printTFT();  // sends the data to the screen

  printSerial();  // sends data to the serial monitor
}

void gameRequest() {  // function to get the games and scores

  if (!client.connect(HOST, 443)) {  //opening connection to server
    Serial.println(F("Connection failed"));
    return;
  }

  yield();  // give the esp a breather

  client.print(F("GET "));     // Send HTTP request
  client.print(requestGames);  // This is the second half of a request (everything that comes after the base URL)
  client.println(F(" HTTP/1.1"));

  client.print(F("Host: "));  //Headers
  client.println(HOST);
  client.println(F("Cache-Control: no-cache"));
  client.println("User-Agent: Arduino Footy Project - ds@ds.com");  // Required by API for contact if anything goes wrong

  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }
  char status[32] = { 0 };  // Check HTTP status
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  char endOfHeaders[] = "\r\n\r\n";  // Skip HTTP headers
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  while (client.available() && client.peek() != '{') {  // This is needed to deal with random characters coming back before the body of the response.
    char c = 0;
    client.readBytes(&c, 1);
    //    Serial.print(c);  // uncomment to see the bad characters
    //    Serial.println("BAD");  // as above
  }

  int i = 0;                // for counting during parsing
  total = 0;                // counts the total games
  char game_roundname[10];  // variable for the round name
  char game_ateam[30];      // variable for the away team
  char game_hteam[30];      // variable for the home team
  char game_winner[30];     // variable for the game winner
  char game_gtime[25];      // variable for the game start (local time)


  JsonDocument filter;  // begin filtering the JSON data from the API. The following code was made with the ArduinoJSON Assistant (v7)

  JsonObject filter_games_0 = filter["games"].add<JsonObject>();
  filter_games_0["roundname"] = true;
  filter_games_0["id"] = true;
  filter_games_0["ateam"] = true;
  filter_games_0["date"] = true;
  filter_games_0["ascore"] = true;
  filter_games_0["hscore"] = true;
  filter_games_0["winner"] = true;
  filter_games_0["hteam"] = true;
  filter_games_0["complete"] = true;

  JsonDocument doc;  //deserialization begins

  DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(filter));

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  for (JsonObject game : doc[F("games")].as<JsonArray>()) {

    strcpy(game_roundname, game[F("roundname")]);  // Variable changed from const char* to char[] // "Round 7", "Round 7", "Round 7", "Round 7", "Round ...
    int game_hscore = game[F("hscore")];           // 85, 95, 118, 112, 113, 42, 81, 82, 42
    strcpy(game_ateam, game[F("ateam")]);          // Variable changed from const char* to char[] // "Collingwood", "Western Bulldogs", "Carlton", "West ...
    long game_id = game[F("id")];                  // 35755, 35760, 35759, 35761, 35756, 35762, 35758, 35757, 35754
    if (game[F("winner")]) {                       // deals with null when there's no winners
      strcpy(game_winner, game[F("winner")]);      // Variable changed from const char* to char[] // nullptr, "Fremantle", "Geelong", "Gold Coast", "Greater ...
    }
    int game_ascore = game[F("ascore")];      // 85, 71, 105, 75, 59, 118, 138, 72, 85
    int game_complete = game[F("complete")];  // 100, 100, 100, 100, 100, 100, 100, 100, 100
    strcpy(game_hteam, game[F("hteam")]);     // Variable changed from const char* to char[] // "Essendon", "Fremantle", "Geelong", "Gold Coast", "Greater ...
    strcpy(game_gtime, game[F("date")]);      // Variable changed from const char* to char[] // "2024-05-09 19:30:00", "2024-05-10 19:10:00", ...


    // Importing temp variables from JSON doc into global arrays to be used outside the function.
    if (!(strcmp(game_winner, "NULL") == 0)) {
      strcpy(get_winner[i], game_winner);
      if (!(strstr(get_winner[i], "Greater Western") == NULL)) {
        strcpy(get_winner[i], "GWS Giants");
      }
    }
    get_hscore[i] = game_hscore;
    get_ascore[i] = game_ascore;
    get_complete[i] = game_complete;
    strcpy(get_roundname[i], game_roundname);
    strcpy(get_hteam[i], game_hteam);
    if (!(strstr(get_hteam[i], "Greater Western") == NULL)) {
      strcpy(get_hteam[i], "GWS Giants");
    }
    strcpy(get_ateam[i], game_ateam);
    if (!(strstr(get_ateam[i], "Greater Western") == NULL)) {
      strcpy(get_ateam[i], "GWS Giants");
    }
    strcpy(get_gtime[i], game_gtime);  // get the game time variable string from "date"

    get_id[i] = game_id;
    orderG[i] = i;

    total = total + 1;

    i++;
  }
  client.stop();  // fixes bug where client.print fails every second time.
}

void tipsRequest() {  // function to get the current tips

  Serial.print("rnd = ");
  Serial.println(rnd);
  Serial.print("currentRnd = ");
  Serial.println(currentRnd);
  Serial.print("year = ");
  Serial.println(year);
  Serial.print("currentYear = ");
  Serial.println(currentYear);

  if ((tipsLive == 1) || (!(rnd == currentRnd)) || (!(year == currentYear))) {  // start of the HTTP get to retrieve tips from the API
    if (!client.connect(HOST, 443)) {                                           //opening connection to server
      Serial.println(F("Connection failed"));
      return;
    }

    yield();  // give the esp a breather

    client.print(F("GET "));    // Send HTTP request
    client.print(requestTips);  // This is the second half of a request (everything that comes after the base URL)
    client.println(F(" HTTP/1.1"));

    client.print(F("Host: "));  //Headers
    client.println(HOST);
    client.println(F("Cache-Control: no-cache"));
    client.println("User-Agent: Arduino Footy Project - ds@ds.com");  // Required by API for contact if anything goes wrong

    if (client.println() == 0) {
      Serial.println(F("Failed to send request"));
      return;
    }
    char status[32] = { 0 };  // Check HTTP status
    client.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
      Serial.print(F("Unexpected response: "));
      Serial.println(status);
      return;
    }

    char endOfHeaders[] = "\r\n\r\n";  // Skip HTTP headers
    if (!client.find(endOfHeaders)) {
      Serial.println(F("Invalid response"));
      return;
    }

    while (client.available() && client.peek() != '{') {  // This is needed to deal with random characters coming back before the body of the response.
      char c = 0;
      client.readBytes(&c, 1);
      //    Serial.print(c);  // uncomment to see the bad characters
      //    Serial.println("BAD");  // as above
    }
    int j = 0;
    while (client.available()) {  // reads the API call client into the global array rawData[]
      char c = 0;
      client.readBytes(&c, 1);
      rawData[j] = c;
      j++;
    }
    rawData[j] = 0;  // adds the required null character to the end of the string rawData[]

    if ((rnd == currentRnd) && (year == currentYear)) {
      File file = LittleFS.open("/tip.txt", FILE_WRITE);  // opens file in LittleFS to write the API call data in the file /tip.txt
      if (!file) {
        Serial.println("- failed to open file for writing");
        return;
      }
      if (file.print(rawData)) {
        Serial.println("LittleFS - file written");
      } else {
        Serial.println("- write failed");
      }
      file.close();
    }
  } else {
    File file = LittleFS.open("/tip.txt", FILE_READ);  // routine to retrieve tips from the LittleFS file /tip.txt into rawData[]
    if (!file) {
      Serial.println(" - failed to open file for reading");
    }
    int j = 0;
    while (file.available()) {
      rawData[j] = (file.read());  //reads the file one character at a time and feeds it into the char array rawData[] as j increments
      j++;
    }
    file.close();
    Serial.println("LittleFS - file read");
  }

  int i = 0;         // count for parsing
  char tip_tip[30];  // temp variable to read the tips
  total = 0;         // global variable to count the games in the round

  JsonDocument filter;  // begin filtering the JSON data from the API. The following code was made with the ArduinoJSON Assistant (v7)

  JsonObject filter_tips_0 = filter["tips"].add<JsonObject>();
  filter_tips_0["gameid"] = true;
  filter_tips_0["tip"] = true;

  JsonDocument doc;  //deserialization begins

  DeserializationError error = deserializeJson(doc, rawData, DeserializationOption::Filter(filter));

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  for (JsonObject tip : doc[F("tips")].as<JsonArray>()) {

    long tip_gameid = tip[F("gameid")];  // 35763, 35771, 35764, 35767, 35769, 35766, 35765, 35768, 35770
    strcpy(tip_tip, tip[F("tip")]);      // Variable changed from const char* to char[] // "Port Adelaide", "Brisbane Lions", "Carlton", "Melbourne", ...

    // Importing temp variables from JSON doc into global arrays to be used outside the function.
    strcpy(get_tip[i], tip_tip);
    if (!(strstr(get_tip[i], "Greater Western") == NULL)) {
      strcpy(get_tip[i], "GWS Giants");
    }
    get_tipId[i] = tip_gameid;
    orderT[i] = i;

    i++;
    total = total + 1;
  }
  client.stop();  // fixes bug where client.print fails every second time.
}

void sortG(long ia[], int len) {  // sortG() function to bubble sort the games into their correct order based on game_id
  for (int x = 0; x < len; x++) {
    for (int y = 0; y < (len - 1); y++) {
      if (ia[y] > ia[y + 1]) {
        long tmp = ia[y + 1];
        int ord = orderG[y + 1];
        ia[y + 1] = ia[y];
        orderG[y + 1] = orderG[y];
        ia[y] = tmp;
        orderG[y] = ord;
      }
    }
  }
}

void sortT(long ia[], int len) {  // sortT() function to bubble sort the tips into their correct order based on game_id
  for (int x = 0; x < len; x++) {
    for (int y = 0; y < (len - 1); y++) {
      if (ia[y] > ia[y + 1]) {
        long tmp = ia[y + 1];
        int ord = orderT[y + 1];
        ia[y + 1] = ia[y];
        orderT[y + 1] = orderT[y];
        ia[y] = tmp;
        orderT[y] = ord;
      }
    }
  }
}

void getLocalTime() {  // function to retrieve the local time according to the set time zone rules
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
  }

  hour = timeinfo.tm_hour;     // sets the hour into the variable. 24 hour clock.
  weekDay = timeinfo.tm_wday;  // sets the weekday into the variable. (0-6) 0 = Sunday.
  currentYear = timeinfo.tm_year;
  currentYear = currentYear + 1900;
  year = currentYear;
  Serial.print("Year = ");
  Serial.println(year);
  Serial.print("Weekday = ");
  Serial.println(weekDay);
  Serial.print("Hour = ");
  Serial.println(hour);
  Serial.println();
}

void timeavailable(struct timeval* t) {  // Callback function (gets called when time adjusts via NTP)
  Serial.println("Got time adjustment from NTP!");
  getLocalTime();
}

void printTFT() {
  tft.fillScreen(background);  // clears the TFT ready for the Title

  if ((tipsLive == 1) || (!(year == currentYear)) || (!(rnd == currentRnd))) {  //Adds a small "API" notification to indicate that tips were accessed from the API
    tft.setCursor(notifX, notifY, 1);
    tft.setTextColor(notifColor, background);
    tft.print("API");
  }
  tft.setCursor(titleX, titleY);
  tft.setFreeFont(FSSB18);
  tft.setTextColor(titleColor, background);
  tft.print("AFL ");
  tft.print(get_roundname[0]);

  picked = 0;
  played = 0;
  gameLive = 0;

  // display routine using the layout parameters defined at the beginning
  for (int i = 0; i < total; i++) {
    if (get_hteam[orderG[i]]) {
      tft.setTextColor(teamColor, background);  // resets the colour for printing the team
      tft.setCursor(hteamLoc, ((i * lineSpacing) + line1Start));
      tft.setFreeFont(FSS9);
      if (strcmp(get_hteam[orderG[i]], get_tip[orderT[i]]) == 0) {
        tft.setTextColor(pickedColor, background);  // set tipped team colour
      }
      tft.print(get_hteam[orderG[i]]);
      tft.setTextColor(teamColor, background);
      tft.setCursor(vsLoc, ((i * lineSpacing) + line1Start));
      tft.print(" vs. ");
      tft.setCursor(ateamLoc, ((i * lineSpacing) + line1Start));
      if (strcmp(get_ateam[orderG[i]], get_tip[orderT[i]]) == 0) {
        tft.setTextColor(pickedColor, background);  // set tipped team colour
      }
      tft.print(get_ateam[orderG[i]]);
      tft.setTextColor(teamColor, background);
      // set font colour to wrongScoreColor if complete, liveScoreColor if playing, pickedScoreColor if complete and picked, green if a draw
      if (get_complete[orderG[i]] != 0) {
        if (get_complete[orderG[i]] == 100) {
          played = played + 1;
          tft.setTextColor(wrongScoreColor, background);  // wrongScoreColor score for a completed game, not tipped
        } else {
          tft.setTextColor(liveScoreColor, background);  // liveScoreColor score for a live game
          if ((get_complete[orderG[i]] == 25) || (get_complete[orderG[i]] == 50) || (get_complete[orderG[i]] == 75)) {
            tft.setTextColor(gamebreakColor, background);  // liveScoreColor score for quarter, half, three quarter time
          }
          gameLive = 1;
        }
        if (strcmp(get_winner[orderG[i]], get_tip[orderT[i]]) == 0) {  // pickedScoreColor for a correct tip
          tft.setTextColor(pickedScoreColor, background);
          picked = picked + 1;
        }
        if ((get_hscore[orderG[i]] == get_ascore[orderG[i]]) && (get_complete[orderG[i]] == 100)) {  // if there's a tie, it counts as a tipped win
          tft.setTextColor(pickedScoreColor, background);
          picked = picked + 1;
        }
        tft.setCursor(scoreLoc, ((i * lineSpacing) + line1Start));            // scores are lined up and printed if the game has started
        if ((get_hscore[orderG[i]] < 100) && (get_hscore[orderG[i]] > 19)) {  // moves the home score over a bit if the home score has two digits
          tft.setCursor((scoreLoc + 10), ((i * lineSpacing) + line1Start));
        }
        if ((get_hscore[orderG[i]] < 20) && (get_hscore[orderG[i]] > 9)) {  // moves the home score over a bit if the first digit is a 1
          tft.setCursor((scoreLoc + 7), ((i * lineSpacing) + line1Start));
        }
        if (get_hscore[orderG[i]] < 10) {  // moves the home score over a bit if the home score is single digit
          tft.setCursor((scoreLoc + 16), ((i * lineSpacing) + line1Start));
        }
        if ((get_hscore[orderG[i]] < 120) && (get_hscore[orderG[i]] > 109)) {  // moves the home score over a bit if the home score has a 1 as the second digit
          tft.setCursor((scoreLoc + 3), ((i * lineSpacing) + line1Start));
        }
        tft.print(get_hscore[orderG[i]]);
        tft.setCursor((scoreLoc + 38), ((i * lineSpacing) + line1Start));
        tft.print(get_ascore[orderG[i]]);
        tft.setTextColor(teamColor, background);
        if ((i == 0) || (strcmp(gameDate[orderG[i]], gameDate[orderG[i - 1]]) != 0)) {  // a line is drawn under the last game of the day
          newdate = newdate + 1;                                                        // indicates that a new day has begun
          if (i != 0) { tft.drawLine(LnStart, (((i - 1) * lineSpacing) + (line1Start + 6)), LnEnd, (((i - 1) * lineSpacing) + (line1Start + 6)), lineColor); }
        }
      } else {  // if the game hasn't started, the start time is listed
        tft.setCursor(timeLoc, ((i * lineSpacing) + line1Start));
        if ((gameTime[orderG[i]][0]) != ' ') {
          tft.setCursor((timeLoc - 4), ((i * lineSpacing) + line1Start));
        }
        if (((gameTime[orderG[i]][0]) == ' ') && ((gameTime[orderG[i]][1]) == '1')) {
          tft.setCursor((timeLoc - 1), ((i * lineSpacing) + line1Start));
        }
        if ((i == 0) || (strcmp(gameDate[orderG[i]], gameDate[orderG[i - 1]]) != 0)) {  // a line is drawn under the last game time of the day
          newdate = newdate + 1;                                                        // indicates that a new day has begun
          if (i != 0) { tft.drawLine(LnStart, (((i - 1) * lineSpacing) + (line1Start + 6)), LnEnd, (((i - 1) * lineSpacing) + (line1Start + 6)), lineColor); }
        }
        if (newdate == 1) {  // changes the games time colour based on grouping same days together
          tft.setTextColor(timeColor1, background);
        }
        if (newdate == 2) {
          tft.setTextColor(timeColor2, background);
        }
        if (newdate == 3) {
          tft.setTextColor(timeColor1, background);
        }
        if (newdate == 4) {
          tft.setTextColor(timeColor2, background);
        }
        if (newdate == 5) {
          tft.setTextColor(timeColor1, background);
        }
        if (newdate == 6) {
          tft.setTextColor(timeColor2, background);
        }
        tft.print(gameTime[orderG[i]]);  // prints the game time
      }
    }
  }


  // Places the tally of correct tips at the bottom
  tft.setTextColor(tallyColor, background);
  tft.setCursor(tallyX, tallyY);
  tft.setFreeFont(FSSB18);
  if (played == 0) {
    tft.print("No Results Yet");
  } else {
    tft.print(picked);
    tft.print(" correct from ");
    tft.print(played);
  }
  tft.setCursor(0, notifY, 1);
  tft.setTextColor(notifColor, background);
  tft.print("MENU");
  newdate = 0;
}

void printSerial() {  // Serial printing for debugging
  Serial.println();
  Serial.print("Delay = ");
  Serial.print(del / 1000);
  Serial.println(" seconds");
  Serial.println();
  Serial.print("AFL ");
  Serial.println(get_roundname[1]);
  for (int i = 0; i < total; i++) {
    if (get_hteam[orderG[i]] != "") {
      Serial.print("Game ");
      Serial.print(i + 1);
      Serial.print(" = ");
      Serial.print(get_hteam[orderG[i]]);
      Serial.print(" vs. ");
      Serial.print(get_ateam[orderG[i]]);
      if (get_complete[orderG[i]]) {
        Serial.print("  Score:  ");
        Serial.print(get_hscore[orderG[i]]);
        Serial.print(" ");
        Serial.print(get_ascore[orderG[i]]);
      }
      Serial.print("  Tip = ");
      Serial.println(get_tip[orderT[i]]);
      Serial.print("Game start time = ");
      Serial.print(gameTime[orderG[i]]);
      Serial.print(" Game date = ");
      Serial.print(gameDate[orderG[i]]);
      Serial.print("  Game complete % = ");
      Serial.println(get_complete[orderG[i]]);
      Serial.println();
    }
  }
}

void startScreenTFT() {  // sets out the loading messages in a rectangle while waiting for the time
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(warnX - 10, warnY - 10, 280, 80, TFT_WHITE);
  tft.setTextColor(TFT_RED, TFT_WHITE);
  tft.setCursor(warnX, warnY, 4);
  tft.println("Connected to internet");
  tft.setCursor(warnX, warnY + 30, 4);
  tft.print("Getting time... ");
}

void stringBuild() {  // build the strings for the URLs for games, scores and tips using the year and rnd variable.
  char rndchr[10];
  char yrchr[10];
  char srcchr[10];
  itoa(tipSource, srcchr, 10);
  itoa(rnd, rndchr, 10);
  itoa(year, yrchr, 10);
  strcpy(requestGames, "/?q=games;year=");
  strcpy(requestTips, "/?q=tips;year=");
  Serial.println(requestGames);
  strcat(requestGames, yrchr);
  strcat(requestGames, part2Games);
  strcat(requestGames, rndchr);
  strcat(requestTips, yrchr);
  strcat(requestTips, part2Tips);
  strcat(requestTips, rndchr);
  strcat(requestTips, part3Tips);
  strcat(requestTips, srcchr);
  Serial.println(requestGames);
  Serial.println(requestTips);
}

void timeZoneDiff() {  // function to take the date/time string from the JSON outpt and parse out the start time for each game and adjust for time zone.
  int gHours[9];
  int gMinutes[9];
  int gzHours[9];
  int gzMinutes[9];
  char gameH[9][2];
  char gameM[9][2];
  for (int i = 0; i < total; i++) {
    strcpy(gameTime[i], &get_gtime[i][11]);                                                     //Gets out the time from the date/time string.
    gameTime[i][5] = NULL;                                                                      // shortens it to 5 characters
    gHours[i] = (((gameTime[i][0] - '0') * 10) + (gameTime[i][1] - '0'));                       // converts to integers using place value
    gMinutes[i] = (((gameTime[i][3] - '0') * 10) + (gameTime[i][4] - '0') + (gHours[i] * 60));  // converts to total minutes
    gMinutes[i] = gMinutes[i] + tz;                                                             // adjusts for time zone
    gzHours[i] = gMinutes[i] / 60;                                                              // converts back to hours
    gzMinutes[i] = (gMinutes[i] - (gzHours[i] * 60));                                           // converts back to minutes
    if (gzHours[i] > 12) {                                                                      // adjusts to 12 hour clock
      gzHours[i] = (gzHours[i] - 12);
    }

    itoa(gzHours[i], gameH[i], 10);    // converts integers to strings
    itoa(gzMinutes[i], gameM[i], 10);  // converts integers to strings
    if (gzHours[i] < 10) {             // adds a leading space if hours under 10
      gameH[i][1] = gameH[i][0];
      gameH[i][0] = ' ';
      gameH[i][2] = NULL;
    }
    if (gzMinutes[i] < 10) {  // adds a leading 0 if minutes are under 10
      gameM[i][1] = gameM[i][0];
      gameM[i][0] = '0';
      gameM[i][2] = NULL;
    }

    strcpy(gameTime[i], gameH[i]);  // builds up the gameTime[] variable again
    strcat(gameTime[i], ":");
    strcat(gameTime[i], gameM[i]);
    gameTime[i][5] = NULL;

    Serial.print(get_gtime[i]);  // builds the gameDate[] variable for detecting games on the same day
    strcpy(gameDate[i], &get_gtime[i][8]);
    gameDate[i][2] = NULL;
  }
}

void menuScreen() {

  int row1Y = 80;
  int row2Y = 200;
  int col1X = 50;
  int col2X = 153;
  int col3X = 256;
  int col4X = 359;
  int butSizeX = 70;
  int butSizeY = 70;
  int offX = 22;
  int offY = 45;
  int dataY = 185;
  int dataXoff = 17;
  uint16_t buttonColor = TFT_SKYBLUE;
  uint16_t menuX = 0, menuY = 0;
  int button = 0;
  bool touched = 0;

  tft.fillScreen(background);
  tft.setCursor(200, 30);
  tft.setFreeFont(FSSB18);
  tft.setTextColor(TFT_CYAN, background);
  tft.print("Menu");
  tft.fillRect(col1X, row1Y, butSizeX, butSizeY, buttonColor);
  tft.fillRect(col2X, row1Y, butSizeX, butSizeY, buttonColor);
  tft.fillRect(col3X, row1Y, butSizeX, butSizeY, buttonColor);
  tft.fillRect(col4X, row1Y, butSizeX, butSizeY, buttonColor);
  tft.fillRect(col1X, row2Y, butSizeX, butSizeY, buttonColor);
  tft.fillRect(col2X, row2Y, butSizeX, butSizeY, buttonColor);
  tft.fillRect(col3X, row2Y, butSizeX, butSizeY, buttonColor);
  tft.fillRect(col4X, row2Y, butSizeX, butSizeY, buttonColor);
  tft.drawRect(col1X + 3, row1Y + 3, butSizeX - 6, butSizeY - 6, background);
  tft.drawRect(col2X + 3, row1Y + 3, butSizeX - 6, butSizeY - 6, background);
  tft.drawRect(col3X + 3, row1Y + 3, butSizeX - 6, butSizeY - 6, background);
  tft.drawRect(col4X + 3, row1Y + 3, butSizeX - 6, butSizeY - 6, background);
  tft.drawRect(col1X + 3, row2Y + 3, butSizeX - 6, butSizeY - 6, background);
  tft.drawRect(col2X + 3, row2Y + 3, butSizeX - 6, butSizeY - 6, background);
  tft.drawRect(col3X + 3, row2Y + 3, butSizeX - 6, butSizeY - 6, background);
  tft.drawRect(col4X + 3, row2Y + 3, butSizeX - 6, butSizeY - 6, background);
  tft.setTextColor(background);
  tft.setFreeFont(FSSB24);
  tft.setCursor(col1X + offX, row1Y + offY);
  tft.print("+");
  tft.setCursor(col2X + offX, row1Y + offY);
  tft.print("+");
  tft.setCursor(col3X + offX, row1Y + offY);
  tft.print("+");
  tft.setCursor(col4X + offX, row1Y + offY);
  tft.print("+");
  tft.setCursor(col1X + offX + 6, row2Y + offY);
  tft.print("-");
  tft.setCursor(col2X + offX + 6, row2Y + offY);
  tft.print("-");
  tft.setCursor(col3X + offX + 6, row2Y + offY);
  tft.print("-");
  tft.setCursor(col4X + offX + 6, row2Y + offY);
  tft.print("-");
  tft.setTextColor(TFT_CYAN, background);
  tft.setFreeFont(FSS9);
  tft.setCursor(col1X + 7, row1Y - 8);
  tft.print("Round");
  tft.setCursor(col2X + 5, row1Y - 8);
  tft.print("Source");
  tft.setCursor(col3X + 15, row1Y - 8);
  tft.print("Year");
  tft.setCursor(col4X + 15, row1Y - 8);
  tft.print("WiFi");
  tft.setCursor(420, 310);
  tft.setFreeFont(FSS12);
  tft.print("EXIT");
  tft.fillRect(0, dataY - 30, col1X - 5, 40, buttonColor);
  tft.drawRect(3, dataY - 27, col1X - 11, 34, background);
  tft.setTextColor(TFT_BLACK);
  tft.setFreeFont(FSS9);
  tft.setCursor(5, dataY - 5);
  tft.print("SET");
  if (rnd == currentRnd) {
    tft.setTextColor(TFT_YELLOW);
  } else {
    tft.setTextColor(TFT_PINK);
  }
  tft.setFreeFont(FSS18);
  if (rnd > 9) {
    tft.setCursor(col1X + dataXoff, dataY);
  } else {
    tft.setCursor(col1X + dataXoff + 10, dataY);
  }
  tft.print(rnd);
  tft.setTextColor(TFT_YELLOW, background);
  tft.setCursor(col2X + dataXoff, dataY);
  tft.print(tipSource);
  tft.setCursor(col3X + dataXoff - 20, dataY);
  if (year == currentYear) {
    tft.setTextColor(TFT_YELLOW);
  } else {
    tft.setTextColor(TFT_PINK);
  }
  tft.print(year);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(col4X + dataXoff - 20, dataY);
  if (wifi == 0) { strcpy(wifihead, "Home"); }
  if (wifi == 1) { strcpy(wifihead, "Phone"); }
  if (wifi == 2) { strcpy(wifihead, "User 1"); }
  if (wifi == 3) { strcpy(wifihead, "User 2"); }
  if (wifi == 4) { strcpy(wifihead, "User 3"); }
  if (wifi == 5) { strcpy(wifihead, "User 4"); }
  tft.print(wifihead);

  while (!(button == 10)) {
    menuX = 0;
    menuY = 0;
    touched = 0;
    button = 0;
    while (!touched) {
      touched = tft.getTouch(&menuX, &menuY);
    }
    if ((menuX > col1X) && (menuX < (col1X + butSizeX)) && (menuY > row1Y) && menuY < (row1Y + butSizeY)) {
      button = 1;
    }
    if ((menuX > col2X) && (menuX < (col2X + butSizeX)) && (menuY > row1Y) && menuY < (row1Y + butSizeY)) {
      button = 2;
    }
    if ((menuX > col3X) && (menuX < (col3X + butSizeX)) && (menuY > row1Y) && menuY < (row1Y + butSizeY)) {
      button = 3;
    }
    if ((menuX > col4X) && (menuX < (col4X + butSizeX)) && (menuY > row1Y) && menuY < (row1Y + butSizeY)) {
      button = 4;
    }
    if ((menuX > col1X) && (menuX < (col1X + butSizeX)) && (menuY > row2Y) && menuY < (row2Y + butSizeY)) {
      button = 5;
    }
    if ((menuX > col2X) && (menuX < (col2X + butSizeX)) && (menuY > row2Y) && menuY < (row2Y + butSizeY)) {
      button = 6;
    }
    if ((menuX > col3X) && (menuX < (col3X + butSizeX)) && (menuY > row2Y) && menuY < (row2Y + butSizeY)) {
      button = 7;
    }
    if ((menuX > col4X) && (menuX < (col4X + butSizeX)) && (menuY > row2Y) && menuY < (row2Y + butSizeY)) {
      button = 8;
    }
    if ((menuX < 40) && (menuY > 140) && (menuY < 180)) {
      button = 9;
    }
    if ((menuX > 420) && (menuY > 290)) {
      button = 10;
    }
    if (button != 0) {
      Serial.print("Button = ");
      Serial.println(button);
    }
    if (button == 1) {
      rnd = rnd + 1;
      if (rnd - 1 > 9) {
        tft.setCursor(col1X + dataXoff, dataY);
      } else {
        tft.setCursor(col1X + dataXoff + 10, dataY);
      }
      tft.setTextColor(background);
      tft.print(rnd - 1);
      if (rnd > 9) {
        tft.setCursor(col1X + dataXoff, dataY);
      } else {
        tft.setCursor(col1X + dataXoff + 10, dataY);
      }
      if (rnd == currentRnd) {
        tft.setTextColor(TFT_YELLOW);
      } else {
        tft.setTextColor(TFT_PINK);
      }
      tft.print(rnd);
    }
    if (button == 5) {
      rnd = rnd - 1;
      if (rnd == -1) {
        rnd = 0;
      }
      if (rnd + 1 > 9) {
        tft.setCursor(col1X + dataXoff, dataY);
      } else {
        tft.setCursor(col1X + dataXoff + 10, dataY);
      }
      tft.setTextColor(background);
      tft.print(rnd + 1);
      if (rnd > 9) {
        tft.setCursor(col1X + dataXoff, dataY);
      } else {
        tft.setCursor(col1X + dataXoff + 10, dataY);
      }
      if (rnd == currentRnd) {
        tft.setTextColor(TFT_YELLOW);
      } else {
        tft.setTextColor(TFT_PINK);
      }
      tft.print(rnd);
    }

    if (button == 2) {
      tipSource = tipSource + 1;
      tft.setCursor(col2X + dataXoff, dataY);
      tft.setTextColor(background);
      tft.print(tipSource - 1);
      tft.setCursor(col2X + dataXoff, dataY);
      tft.setTextColor(TFT_YELLOW);
      tft.print(tipSource);
    }
    if (button == 6) {
      tipSource = tipSource - 1;
      if (tipSource == -1) {
        tipSource = 0;
      }
      tft.setCursor(col2X + dataXoff, dataY);
      tft.setTextColor(background);
      tft.print(tipSource + 1);
      tft.setCursor(col2X + dataXoff, dataY);
      tft.setTextColor(TFT_YELLOW);
      tft.print(tipSource);
    }
    if (button == 3) {
      year = year + 1;
      tft.setCursor(col3X + dataXoff - 20, dataY);
      tft.setTextColor(background);
      tft.print(year - 1);
      tft.setCursor(col3X + dataXoff - 20, dataY);
      if (year == currentYear) {
        tft.setTextColor(TFT_YELLOW);
      } else {
        tft.setTextColor(TFT_PINK);
      }
      tft.print(year);
    }
    if (button == 7) {
      year = year - 1;
      tft.setCursor(col3X + dataXoff - 20, dataY);
      tft.setTextColor(background);
      tft.print(year + 1);
      tft.setCursor(col3X + dataXoff - 20, dataY);
      if (year == currentYear) {
        tft.setTextColor(TFT_YELLOW);
      } else {
        tft.setTextColor(TFT_PINK);
      }
      tft.print(year);
    }
    if (button == 8) {
      tft.setCursor(col4X + dataXoff - 20, dataY);
      tft.setTextColor(background);
      if (wifi == 0) { strcpy(wifihead, "Home"); }
      if (wifi == 1) { strcpy(wifihead, "Phone"); }
      if (wifi == 2) { strcpy(wifihead, "User 1"); }
      if (wifi == 3) { strcpy(wifihead, "User 2"); }
      if (wifi == 4) { strcpy(wifihead, "User 3"); }
      if (wifi == 5) { strcpy(wifihead, "User 4"); }
      tft.print(wifihead);
      wifi = wifi + 1;
      if (wifi == 6) { wifi = 0; }
      tft.setCursor(col4X + dataXoff - 20, dataY);
      tft.setTextColor(TFT_YELLOW);
      if (wifi == 0) { strcpy(wifihead, "Home"); }
      if (wifi == 1) { strcpy(wifihead, "Phone"); }
      if (wifi == 2) { strcpy(wifihead, "User 1"); }
      if (wifi == 3) { strcpy(wifihead, "User 2"); }
      if (wifi == 4) { strcpy(wifihead, "User 3"); }
      if (wifi == 5) { strcpy(wifihead, "User 4"); }
      tft.print(wifihead);
    }
    if (button == 4) {
      tft.setCursor(col4X + dataXoff - 20, dataY);
      tft.setTextColor(background);
      if (wifi == 0) { strcpy(wifihead, "Home"); }
      if (wifi == 1) { strcpy(wifihead, "Phone"); }
      if (wifi == 2) { strcpy(wifihead, "User 1"); }
      if (wifi == 3) { strcpy(wifihead, "User 2"); }
      if (wifi == 4) { strcpy(wifihead, "User 3"); }
      if (wifi == 5) { strcpy(wifihead, "User 4"); }
      tft.print(wifihead);
      wifi = wifi - 1;
      if (wifi == -1) { wifi = 5; }
      tft.setCursor(col4X + dataXoff - 20, dataY);
      tft.setTextColor(TFT_YELLOW);
      if (wifi == 0) { strcpy(wifihead, "Home"); }
      if (wifi == 1) { strcpy(wifihead, "Phone"); }
      if (wifi == 2) { strcpy(wifihead, "User 1"); }
      if (wifi == 3) { strcpy(wifihead, "User 2"); }
      if (wifi == 4) { strcpy(wifihead, "User 3"); }
      if (wifi == 5) { strcpy(wifihead, "User 4"); }
      tft.print(wifihead);
    }



    if ((button == 9) && (year == currentYear)) {
      currentRnd = rnd;
      tft.setCursor(col1X + dataXoff, dataY);
      tft.setTextColor(background);
      tft.print(rnd);
      tft.setCursor(col1X + dataXoff, dataY);
      tft.setTextColor(TFT_YELLOW);
      tft.print(rnd);
    }





    while (touched) {
      touched = 0;
      touched = tft.getTouch(&menuX, &menuY);
    }
  }
  writeWifi();
  writeSource();
  writeRound();
  tft.fillScreen(background);
  stringBuild();
  tipsRequest();            // calls the function to get the tips
  sortT(get_tipId, total);  // sorts the matches into chronological order using the sortT() function
  gameRequest();
  timeZoneDiff();        // adjusts the game time for time zone and change to 12 hour clock
  sortG(get_id, total);  // sorts the matches into chronological order using the sortG() function
  printTFT();
}

void writeRound() {
  File file = LittleFS.open("/currentRnd.txt", FILE_WRITE);  // opens file in LittleFS to write the current round in the file /currentRND.txt
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(currentRnd)) {
    Serial.println("LittleFS - Round file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void writeSource() {
  File file = LittleFS.open("/tipSource.txt", FILE_WRITE);  // opens file in LittleFS to write the current tipping source in the file /tipSource.txt
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(tipSource)) {
    Serial.println("LittleFS - Source file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void writeWifi() {
  File file = LittleFS.open("/WiFi.txt", FILE_WRITE);  // opens file in LittleFS to write the current WiFi settings in the file /WiFi.txt
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(wifi)) {
    Serial.println("LittleFS - WiFi file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void readRound() {
  File file = LittleFS.open("/currentRnd.txt", FILE_READ);  // routine to retrieve the current round from the LittleFS file /currentRnd.txt into rawRnd[]
  if (!file) {
    Serial.println(" - failed to open file for reading");
  }
  int j = 0;
  char rawRnd[4];
  while (file.available()) {
    rawRnd[j] = (file.read());  //reads the file one character at a time and feeds it into the char array rawRnd[] as j increments
    j++;
  }
  file.close();
  rawRnd[j] = '\0';
  currentRnd = atoi(rawRnd);
  rnd = currentRnd;
  Serial.println("LittleFS - Round file read");
}

void readSource() {
  File file = LittleFS.open("/tipSource.txt", FILE_READ);  // routine to retrieve the current tipping source from the LittleFS file /tipSource.txt into rawSrc[]
  if (!file) {
    Serial.println(" - failed to open file for reading");
  }
  int j = 0;
  char rawSrc[4];
  while (file.available()) {
    rawSrc[j] = (file.read());  //reads the file one character at a time and feeds it into the char array rawSrc[] as j increments
    j++;
  }
  file.close();
  rawSrc[j] = '\0';
  tipSource = atoi(rawSrc);
  Serial.println("LittleFS - Source file read");
}

void readWifi() {
  File file = LittleFS.open("/WiFi.txt", FILE_READ);  // routine to retrieve the current WiFi settings from the LittleFS file /WiFi.txt into rawSrc[]
  if (!file) {
    Serial.println(" - failed to open file for reading");
  }
  int j = 0;
  char rawWif[4];
  while (file.available()) {
    rawWif[j] = (file.read());  //reads the file one character at a time and feeds it into the char array rawWif[] as j increments
    j++;
  }
  file.close();
  rawWif[j] = '\0';
  wifi = atoi(rawWif);
  Serial.println("LittleFS - WiFi file read");
}


void loop() {

  ArduinoOTA.handle();  // Handle OTA

  uint16_t touchX = 0, touchY = 0;  // To store the touch coordinates
  bool pressed = tft.getTouch(&touchX, &touchY);
  if ((pressed) && (touchX < 30) && (touchY > 290)) {  //Selects the menu my touching MENU icon
    menuScreen();
  }



  unsigned long currentMillis = millis();  //needed for the millis() delay if statement

  if (gameLive == 1) {  // Sets the delay based on whether a game is live or not.
    del = delLive;
  } else {
    del = delNoGames;
  }

  if (currentMillis - previousMillis >= del) {  // allows the loop code to run if the delay time has elapsed
    previousMillis = currentMillis;             //resets the milliseconds to the current time.

    gameRequest();  // calls the gameRequest() function

    timeZoneDiff();  // adjusts the game time for time zone and change to 12 hour clock

    sortG(get_id, total);  // sorts the matches into chronological order using the sortG() function

    printTFT();  // sends the data to the screen

    printSerial();  // sends data to the serial monitor
  }
}
