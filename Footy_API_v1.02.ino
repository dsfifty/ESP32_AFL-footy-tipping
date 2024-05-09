/*
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

*/

#include <WiFi.h>              // Wifi library
#include <WiFiClientSecure.h>  // HTTP Client for ESP32 (comes with the ESP32 core download)
#include <ArduinoJson.h>       // Required for parsing the JSON coming from the API calls
#include <TFT_eSPI.h>          // Required library for TFT Dsplay. Pin settings found in User_Setup.h in the library directory
#include <SPI.h>               // Communicate with the TFT
#include <LittleFS.h>          // for saving tips data
#include <time.h>              // for using time functions
#include <sntp.h>              // for synchronising clock online

TFT_eSPI tft = TFT_eSPI();  // Activate TFT device
#define TFT_GREY 0x5AEB     // New colour

#define HOST "api.squiggle.com.au"  // Base URL of the API

char rnd[] = "9";      // select round
char year[] = "2024";  // select year

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
uint16_t pickedScoreColor = TFT_GREEN;  // colour of the score - tipped. Default: TFT_GREEN
uint16_t wrongScoreColor = TFT_RED;     // colour of the score - not tipped. Default: TFT_RED
uint16_t liveScoreColor = TFT_CYAN;     // colour of the live score. Default: TFT_GREEN
uint16_t notifColor = TFT_CYAN;         // colour of the API notification

int titleX = 70;       // These are the parameters for the setting out of the display. Title x position
int titleY = 0;        // title y position
int line1Start = 30;   // location of the first line location under the title
int lineSpacing = 20;  // Line spacing
int vsLoc = 105;       // Horizontal location of the "vs."
int hteamLoc = 0;      // Horizontal location of the Home Team
int ateamLoc = 150;    // Horizontal location of the Away Team
int scoreLoc = 266;    // Horizontal location of the score
int tallyX = 70;       // Horizontal location of the tips tally
int tallyY = 215;      // Vertical location of the tips tally
int warnX = 30;        // Horizontal location of the startup notification
int warnY = 90;        // Vertical location of the startup notification
int notifX = 302;      // Horizontal location of the API notification
int notifY = 229;      // Horizontal location of the API notification

const char* ntpServer1 = "pool.ntp.org";  // Time servers 1 and 2
const char* ntpServer2 = "time.nist.gov";

const char* time_zone = "ACST-9:30ACDT,M10.1.0,M4.1.0/3";  // TimeZone rule for Adelaide including daylight adjustment rules

char requestGames[30] = "/?q=games;year=";  // building blocks of the API URL (less base)
char part2Games[] = ";round=";              // for getting games and scores
char requestTips[40] = "/?q=tips;year=";    // building blocks of the API URL (less base)
char part2Tips[] = ";round=";               // for getting tips
char part3Tips[] = ";source=1";             // source is the tipping source. 1 = Squiggle

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

int picked = 0;           // counts the successful tips
int total = 0;            // counts the games in each round
int played = 0;           // counts the games played in each round
int gameLive = 0;         // used to test if a game is live to shorten the loop delay
int delLive = 30000;      // delay between API calls if there is a live game
int delNoGames = 120000;  // delay between API calls if there is no live game
int weekDay = 10;         // global variable to hold the day of the week when set to 10, it will wait until the time is retrieved
int hour = 23;
int tipsLive = 1;  // global variable 1 = tips are retrieved from API, 0= tips are retrieved from LittleFS

char ssid[] = "Jerry";                 // your network SSID (name)
char password[] = "ABCNEWSBREAKFAST";  // your network key

WiFiClientSecure client;  // starts the client for HTTPS requests

void setup() {

  Serial.begin(115200);  // for debugging

  if (!LittleFS.begin()) {  //LittleFS set up. Put true first time running on a new esp32 to format the LittleFS
    Serial.println("Error mounting LittleFS");
  }

  sntp_set_time_sync_notification_cb(timeavailable);  // set notification call-back function
  sntp_servermode_dhcp(1);                            // (optional)
  configTzTime(time_zone, ntpServer1, ntpServer2);    // these all need to be done before connecting to WiFi

  WiFi.begin(ssid, password);  // Connect to the WiFI
  Serial.println("");

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

  client.setInsecure();  //bypasses certificates

  stringBuild();  // build the strings for the URLs for games, scores and tips using the year and rnd variable.

  tft.init();          // initialises the TFT
  tft.setRotation(3);  // sets the rotation to match the device

  startScreenTFT();  // sets out the loading messages in a rectangle while waiting for the time

  Serial.print("Getting time ");  // waits for the interupt function to get the time.
  while (weekDay == 10) {
    Serial.print(".");
    delay(100);
  }

  tft.fillScreen(background);

  if ((weekDay == 1) || (weekDay == 2) || ((weekDay == 3) && hour < 20)) {  // decides whether to use live tips or saved tips
    tipsLive = 1;                                                           // based on the days of the week and hour.
    Serial.println("Getting tips from API and writing to LittleFS");        // 0=Sunday. 24 hour clock
  } else {                                                                  // 1 = get tips from API
    tipsLive = 0;                                                           // 0 = get tips from LittleFS
    Serial.println("Getting tips by reading from LittleFS");
  }

  tipsRequest();  // calls the function to get the tips

  sortT(get_tipId, total);  // sorts the matches into chronological order using the sortT() function
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
  client.println("User-Agent: Arduino Footy Project - dsfifty.smith@gmail.com");  // Required by API for contact if anything goes wrong

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
  total = 0;                // counts thetotal games
  char game_roundname[10];  // variable for the round name
  char game_ateam[30];      // variable for the away team
  char game_hteam[30];      // variable for the home team
  char game_winner[30];     // variable for the game winner

  JsonDocument filter;  // begin filtering the JSON data from the API. The following code was made with the ArduinoJSON Assistant (v7)

  JsonObject filter_games_0 = filter["games"].add<JsonObject>();
  filter_games_0["roundname"] = true;
  filter_games_0["hscore"] = true;
  filter_games_0["ateam"] = true;
  filter_games_0["id"] = true;
  filter_games_0["winner"] = true;
  filter_games_0["ascore"] = true;
  filter_games_0["complete"] = true;
  filter_games_0["hteam"] = true;

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
    get_id[i] = game_id;
    orderG[i] = i;

    total = total + 1;

    i++;
  }
  client.stop();  // fixes bug where client.print fails every second time.
}

void tipsRequest() {  // function to get the current tips

  if (tipsLive == 1) {                 // start of the HTTP get to retrieve tips from the API
    if (!client.connect(HOST, 443)) {  //opening connection to server
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
    client.println("User-Agent: Arduino Footy Project - dsfifty.smith@gmail.com");  // Required by API for contact if anything goes wrong

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

    Serial.print("Tip ");
    Serial.print(i + 1);
    Serial.print(" = ");
    Serial.println(get_tip[i]);
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

  if (tipsLive == 1) {  //Adds a small "API" notification to indicate that tips were accessed from the API
    tft.setCursor(notifX, notifY, 1);
    tft.setTextColor(notifColor, background);
    tft.print("API");
  }

  tft.setCursor(titleX, titleY, 4);
  tft.setTextColor(titleColor, background);
  tft.print("AFL ");
  tft.print(get_roundname[0]);

  tft.setTextColor(teamColor, background);  // change to baseColor with font 2 for the games list
  tft.setTextFont(2);

  picked = 0;
  played = 0;
  gameLive = 0;

  // display routine using the layout parameters defined at the beginning
  for (int i = 0; i < total; i++) {
    if (get_hteam[orderG[i]]) {
      tft.setCursor(hteamLoc, ((i * lineSpacing) + line1Start), 2);
      if (strcmp(get_hteam[orderG[i]], get_tip[orderT[i]]) == 0) {
        tft.setTextColor(pickedColor, background);  // set tipped team colour
      }
      tft.print(get_hteam[orderG[i]]);
      tft.setTextColor(teamColor, background);
      tft.setCursor(vsLoc, ((i * lineSpacing) + line1Start), 2);
      tft.print(" vs. ");
      tft.setCursor(ateamLoc, ((i * lineSpacing) + line1Start), 2);
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
        tft.setCursor(scoreLoc, ((i * lineSpacing) + line1Start), 2);
        if (get_hscore[orderG[i]] < 100) { tft.print(" "); }
        if (get_hscore[orderG[i]] < 10) { tft.print(" "); }
        tft.print(get_hscore[orderG[i]]);
        tft.print(" ");
        tft.print(get_ascore[orderG[i]]);
        tft.setTextColor(teamColor, background);
      }
    }
  }

  // Places the tally of correct tips at the bottom
  tft.setTextColor(tallyColor, background);
  tft.setCursor(tallyX, tallyY, 4);
  if (played == 0) {
    tft.print("No Results Yet");
  } else {
    tft.print(picked);
    tft.print(" correct from ");
    tft.print(played);
  }
}

void printSerial() {  // Serial printing for debugging
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
  strcat(requestGames, year);
  strcat(requestGames, part2Games);
  strcat(requestGames, rnd);
  strcat(requestTips, year);
  strcat(requestTips, part2Tips);
  strcat(requestTips, rnd);
  strcat(requestTips, part3Tips);
}

void setDelay() {  // delay between API requests. Based on whether a game is live or not. Prob best to keep this above 60000
  if (gameLive == 1) {
    delay(delLive);
    Serial.print("Delay = ");
    Serial.print(delLive / 1000);
    Serial.println(" seconds");
    Serial.println();
  } else {
    delay(delNoGames);
    Serial.print("Delay = ");
    Serial.print(delNoGames / 1000);
    Serial.println(" seconds");
    Serial.println();
  }
}

void loop() {

  gameRequest();  // calls the gameRequest() function

  sortG(get_id, total);  // sorts the matches into chronological order using the sortG() function

  printTFT();

  printSerial();

  setDelay();
}
