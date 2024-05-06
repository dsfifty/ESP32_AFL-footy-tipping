/*
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

TFT_eSPI tft = TFT_eSPI();  // Activate TFT device
#define TFT_GREY 0x5AEB     // New colour

char rnd[] = "8";      // select round
char year[] = "2024";  // select year

char requestGames[30] = "/?q=games;year=";  // building blocks of the API URL (less base)
char part2Games[] = ";round=";              // for getting games and scores
char requestTips[40] = "/?q=tips;year=";    // building blocks of the API URL (less base)
char part2Tips[] = ";round=";               // for getting tips
char part3Tips[] = ";source=1";             // source is the tipping source. 1 = Squiggle

#define HOST "api.squiggle.com.au"  // Base URL of the API

int line1Start = 30;   // These are the parameters for the setting out of the display - first line location under the title
int lineSpacing = 20;  // Line spacing
int vsLoc = 105;       // Horizontal location of the "vs."
int hteamLoc = 0;      // Horizontal location of the Home Team
int ateamLoc = 150;    // Horizontal location of the Away Team
int scoreLoc = 266;    // Horizontal location of the score
int totalLocX = 70;    // Horizontal location of the tips tally
int totalLocY = 215;   // Vertical location of the tips tally

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

int picked = 0;    // counts the successful tips
int total = 0;     // counts the games in each round
int played = 0;    // counts the games played in each round
int gameLive = 0;  // used to test if a game is ive to shorten the loop delay
int delLive = 30000;
int delNoGames = 120000;

char ssid[] = "Jerry";                 // your network SSID (name)
char password[] = "ABCNEWSBREAKFAST";  // your network key

WiFiClientSecure client;  // starts the client for HTTPS requests

void setup() {

  Serial.begin(115200);  // for debugging

  WiFi.begin(ssid, password);  // Connect to the WiFI
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {  // Wait for connection
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  client.setInsecure();  //bypasses certificates

  // build the strings for the URLs for games, scores and tips using the year and rnd variable.
  strcat(requestGames, year);
  strcat(requestGames, part2Games);
  strcat(requestGames, rnd);
  strcat(requestTips, year);
  strcat(requestTips, part2Tips);
  strcat(requestTips, rnd);
  strcat(requestTips, part3Tips);

  tft.init();          // initialises the TFT
  tft.setRotation(3);  // sets the rotation to match the device

  tipsRequest();            // calls the function to get the tips
  sortT(get_tipId, total);  // sorts the matches into chronological order using the sortT() function

  // get_tip[] repair
  strcpy(get_tip[orderT[2]], "Sydney");
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

    strcpy(game_roundname, game[F("roundname")]);  // Variable changed from const char* to char[] "Round 7", "Round 7", "Round 7", "Round 7", "Round ...
    int game_hscore = game[F("hscore")];           // 85, 95, 118, 112, 113, 42, 81, 82, 42
    strcpy(game_ateam, game[F("ateam")]);          // Variable changed from const char* to char[] "Collingwood", "Western Bulldogs", "Carlton", "West ...
    long game_id = game[F("id")];                  // 35755, 35760, 35759, 35761, 35756, 35762, 35758, 35757, 35754
    if (game[F("winner")]) {                       // deals with null when there's no winners
      strcpy(game_winner, game[F("winner")]);      // Variable changed from const char* to char[] nullptr, "Fremantle", "Geelong", "Gold Coast", "Greater ...
    }
    int game_ascore = game[F("ascore")];      // 85, 71, 105, 75, 59, 118, 138, 72, 85
    int game_complete = game[F("complete")];  // 100, 100, 100, 100, 100, 100, 100, 100, 100
    strcpy(game_hteam, game[F("hteam")]);     // Variable changed from const char* to char[] "Essendon", "Fremantle", "Geelong", "Gold Coast", "Greater ...


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

void tipsRequest() {

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

  int i = 0;         // count for parsing
  char tip_tip[30];  // temp variable to read the tips
  total = 0;         // global variable to count the games in the round

  JsonDocument filter;  // begin filtering the JSON data from the API. The following code was made with the ArduinoJSON Assistant (v7)

  JsonObject filter_tips_0 = filter["tips"].add<JsonObject>();
  filter_tips_0["gameid"] = true;
  filter_tips_0["tip"] = true;

  JsonDocument doc;  //deserialization begins

  DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(filter));

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  for (JsonObject tip : doc[F("tips")].as<JsonArray>()) {

    long tip_gameid = tip[F("gameid")];  // 35763, 35771, 35764, 35767, 35769, 35766, 35765, 35768, 35770
    strcpy(tip_tip, tip[F("tip")]);      // Variable changed from const char* to char[] "Port Adelaide", "Brisbane Lions", "Carlton", "Melbourne", ...

    // Importing temp variables from JSON doc into global arrays to be used outside the function.
    strcpy(get_tip[i], tip_tip);
    if (!(strstr(get_tip[i], "Greater Western") == NULL)) {
      strcpy(get_tip[i], "GWS Giants");
    }
    get_tipId[i] = tip_gameid;
    orderT[i] = i;

    Serial.println(get_tip[i]);
    i++;
    total = total + 1;
  }
  client.stop();  // fixes bug where client.print fails every second time.
}

// sortG() function to bubble sort the games into their correct order based on game_id
void sortG(long ia[], int len) {
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

// sortT() function to bubble sort the tips into their correct order based on game_id
void sortT(long ia[], int len) {
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

void loop() {

  gameRequest();  // calls the gameRequest() function

  for (int i = 0; i < 9; i++) {
    Serial.print(get_id[i]);
    Serial.print(" ");
    Serial.println(orderG[i]);
  }

  sortG(get_id, total);  // sorts the matches into chronological order using the sortG() function

  tft.fillScreen(TFT_BLACK);  // clears the TFT ready for the Title
  tft.setCursor(0, 0, 4);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("AFL ");
  tft.println(get_roundname[0]);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);  // change to yellow print at font 2 for the games list
  tft.setTextFont(2);

  picked = 0;
  played = 0;
  gameLive = 0;

  // display routine using the layout parameters defined at the beginning
  for (int i = 0; i < total; i++) {
    if (get_hteam[orderG[i]]) {
      tft.setCursor(hteamLoc, ((i * lineSpacing) + line1Start), 2);
      if (strcmp(get_hteam[orderG[i]], get_tip[orderT[i]]) == 0) {
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);  // set tipped team colour
      }
      tft.print(get_hteam[orderG[i]]);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setCursor(vsLoc, ((i * lineSpacing) + line1Start), 2);
      tft.print(" vs. ");
      tft.setCursor(ateamLoc, ((i * lineSpacing) + line1Start), 2);
      if (strcmp(get_ateam[orderG[i]], get_tip[orderT[i]]) == 0) {
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);  // set tipped team colour
      }
      tft.print(get_ateam[orderG[i]]);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      // set font colour to red if complete, cyan if playing, green if complete and picked, green if a draw
      if (get_complete[orderG[i]] != 0) {
        if (get_complete[orderG[i]] == 100) {
          played = played + 1;
          tft.setTextColor(TFT_RED, TFT_BLACK);  // red score for a completed game, not tipped
        } else {
          tft.setTextColor(TFT_CYAN, TFT_BLACK);  // cyan score for a live game
          gameLive = 1;
        }
        if (strcmp(get_winner[orderG[i]], get_tip[orderT[i]]) == 0) {  // Green for a correct tip
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          picked = picked + 1;
        }
        if ((get_hscore[orderG[i]] == get_ascore[orderG[i]]) && (get_complete[orderG[i]] == 100)) {  // if there's a tie, it counts as a tipped win
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          picked = picked + 1;
        }
        tft.setCursor(scoreLoc, ((i * lineSpacing) + line1Start), 2);
        if (get_hscore[orderG[i]] < 100) { tft.print(" "); }
        if (get_hscore[orderG[i]] < 10) { tft.print(" "); }
        tft.print(get_hscore[orderG[i]]);
        tft.print(" ");
        tft.print(get_ascore[orderG[i]]);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      }
    }
  }

  // Places the tally of correct tips at the bottom
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(totalLocX, totalLocY, 4);
  if (played == 0) {
    tft.setCursor(totalLocX, totalLocY, 4);
    tft.print("No Results Yet");
  } else {
    tft.print(picked);
    tft.print(" correct from ");
    tft.print(played);
  }


  // Serial printing for debugging
  Serial.print("AFL ");
  Serial.println(get_roundname[1]);
  for (int i = 0; i < total; i++) {
    if (get_hteam[orderG[i]] != "") {
      Serial.print(get_hteam[orderG[i]]);
      Serial.print(" vs. ");
      Serial.print(get_ateam[orderG[i]]);
      if (get_complete[orderG[i]]) {
        Serial.print("  Score:  ");
        Serial.print(get_hscore[orderG[i]]);
        Serial.print(" ");
        Serial.print(get_ascore[orderG[i]]);
      }
      Serial.println();
      Serial.println(get_tip[orderT[i]]);
    }
  }

  Serial.print(get_roundname[0]);

  // delay between API requests. Based on whether a game is live or not. Prob best to keep this above 60000
  if (gameLive == 1) {
    delay(delLive);
  } else {
    delay(delNoGames);
  }
}
