/*

*/
const char *Ver = "1.3"; // Program version

#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h> // For non-volatile memory
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFiManager.h>
#include <ESPmDNS.h> // Header for mDNS functions
// Custom name for  web page (e.g., http://myboard.local)
const char *mdnsName = "Zpad";

// CYD Touchscreen Pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
// Board-specific tri-color LED PINs
#define CYD_LED_RED 22   //  4 or 22
#define CYD_LED_GREEN 16 // 16
#define CYD_LED_BLUE 17  // 17

// Declare subroutines and functions
void CYD_LED(String Color); // Valid calls: Off, Red, Green, Blue, Yellow, Cyan, Magenta, White
void checkHeader();
void setBackgroundColor();
void configModeCallback(WiFiManager *myWiFiManager);
void updateHeader();
void drawRSSI();
void drawConfig();
String readConfig(String &codeString);
void doConfig();
void drawButtons();
String readButtons(String &codeString);
void setLED();
void drawStatus();
void loadConfiguration();
void handleRoot();
void handleSave();
void connectToRadio();

// Instantiate using the standard library parameters
Preferences preferences;
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();
WiFiManager wm;
WebServer server(80);

// PWM related parameter settings for dimming screen backlight.
#define BACKLIGHT_PIN 21
#define TFT_BACKLIGHT_ON HIGH
int freq = 2000;
int channel = 0;
int resolution = 8;
int BL_Level = 100;        // Backlight level 0-255
int bgColorIndex = 1;      // 0 = Black, 1 = Blue, 2 = Green
uint16_t bgColor = 0x0000; // Selected background color

// Button Grid Definition and button declaration
const int ROWS = 5;
const int COLS = 2;
const int TOTAL_BUTTONS = ROWS * COLS;
TFT_eSPI_Button buttons[TOTAL_BUTTONS];
TFT_eSPI_Button configButtons[TOTAL_BUTTONS];

char configLabels[TOTAL_BUTTONS][10] = {
    "CLR WIFI", "REBOOT", "DIS WIFI",
    "ENA WIFI", "DIMMER", "BRIGHTER",
    "MAN HOST", "AUTO HOST", "LOG OUT", "LOG IN"};

uint16_t configColors[TOTAL_BUTTONS] = {
    TFT_YELLOW, TFT_RED, TFT_DARKGREY,
    TFT_LIGHTGREY, TFT_DARKGREY, TFT_LIGHTGREY,
    TFT_DARKGREY, TFT_LIGHTGREY,
    TFT_DARKGREEN, TFT_GREEN};

uint16_t buttonColors[TOTAL_BUTTONS] = {
    TFT_DARKGREY, TFT_LIGHTGREY,
    TFT_DARKGREY, TFT_LIGHTGREY,
    TFT_DARKGREY, TFT_LIGHTGREY,
    TFT_DARKGREY, TFT_LIGHTGREY,
    TFT_DARKGREY, TFT_LIGHTGREY};

char configCode[TOTAL_BUTTONS][41] = {
    "CLR",
    "REBOOT",
    "DIS",
    "ENA",
    "DIMMER",
    "BRIGHTER",
    "MAN",
    "AUTO",
    "LOGOUT",
    "LOGIN",
};

// Calibration values (Adjust if your touch registers far away from where you press)
#define TS_MINX 300
#define TS_MAXX 3800
#define TS_MINY 200
#define TS_MAXY 3800

// WiFi settings
WiFiServer HttpServer(80);
int ConnectTimeout = 15;
int PortalTimeout = 300;
int PortalRetry = 3;

// Variable to store the HTTP request
String header;

// --- Data Structures ---
String labelArray[10];
String macroArray[10];
String hostArray[5];
String macroCode;

// TS-890 KNS Configurations
const char *radioIP = "192.168.1.89";           // Replace with your TS-890 IP
const uint16_t port = 60000;                    // Default Kenwood KNS TCP Port
const char *adminID = "##ID00606WD5ACPWD5ACP;"; // As configured in radio LAN menu
const char *adminPwd = "WD5ACP";                // As configured in radio LAN menu
WiFiClient clientHost;
unsigned long lastActivityTime = 0;
unsigned long lostHostTime = 0;

// configFlags
bool isLogout = false;
bool autoLogin = false;
// Variables to track  timing
unsigned long touchStartTime = 0;
unsigned long duration = 0;
unsigned long refreshTime = 0;

int menuNum = 2;    //
int newMenuNum = 0; // Default menu here, 0 = system menu with pre-defined buttons
int maxMenu = 2;

void setup()
{
  setCpuFrequencyMhz(240); // Reduce 240 CPU clock speed to 160 MHz for power saving and stability
  Serial.begin(115200);
  // On-board tri-color LED setup
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  CYD_LED("White");

  // Initialize NVS and load variables
  loadConfiguration();

  // 1. Initialize the default global SPI bus with the Touch Screen Pins
  SPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  // 2. Initialize Touchscreen with 0 arguments to fix compilation error
  ts.begin();
  ts.setRotation(0); // Standard Landscape orientation
  // 3. Initialize Display (TFT_eSPI uses separate hardware pin mapping internally)
  tft.init();
  tft.setRotation(0);
  ledcSetup(channel, freq, resolution);  // Setup the PWM channel for backlight control
  ledcAttachPin(BACKLIGHT_PIN, channel); // Attach the PWM channel to the backlight pin

  preferences.begin("AppSettings", false);               // This is the namespace for local storage
  BL_Level = preferences.getInt("BackLight_Lvl", 100);   //
  bgColorIndex = preferences.getInt("BG_ColorIndex", 2); // Look for background color, default to green
  autoLogin = preferences.getBool("loginMode", false);
  preferences.end();
  ledcWrite(channel, BL_Level);
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setCursor(2, 12);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // Start WiFi

  tft.println("Starting WiFi Manager");
  CYD_LED("Magenta");
  wm.setConnectRetries(PortalRetry);
  wm.setAPCallback(configModeCallback);
  wm.setConnectTimeout(ConnectTimeout);
  wm.setConfigPortalTimeout(PortalTimeout);
  WiFi.begin(); // Need to start WiFi for a moment in order to read SSID from memory...
  tft.printf("Connecting to %s\n", wm.getWiFiSSID().c_str());
  WiFi.disconnect();                  //
  wm.autoConnect("Zpad_WiFi_Config"); // password can be added as second parameter, e.g. autoConnect("AmpServer_AP","password");
  // continue after connection or timeout...
  tft.println("WiFi Manager finished");
  if (WiFi.isConnected())
  {
    tft.println("Connected to " + WiFi.SSID());
    tft.println("");
    // tft.println("Using IP address " + WiFi.localIP().toString());
  }
  else
  {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("No WiFi connection");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    delay(1000);
  }

  // Initialize mDNS
  if (!MDNS.begin(mdnsName))
  {
    tft.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  tft.println("mDNS responder started.");
  // HttpServer.begin(); // Start the HTTP server
  // Add HTTP service to MDNS
  MDNS.addService("http", "tcp", 80);
  tft.println(String("http://") + mdnsName + ".local");
  tft.println("or use IP address");
  tft.println("to view webpage");
  tft.println("");
  tft.println("Starting in 10 seconds...");

  // Define Web Interfaces
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);

  server.begin();

  setBackgroundColor();

  tft.fillRect(0, 0, TFT_WIDTH, 70, bgColor);
  tft.drawRect(0, 0, TFT_WIDTH, 70, TFT_WHITE);

  if (autoLogin)
  {
    menuNum = 1;
  }

  // drawButtons();
} // end setup()

void loop()
{
  server.handleClient();

  // refesh header every 1 seconds
  if ((millis() - refreshTime > 1000))
  {
    updateHeader();
    drawRSSI();
    refreshTime = millis();
  }

  checkHeader();
  if ((newMenuNum != menuNum) && (newMenuNum == 0))
  {
    drawConfig();
    menuNum = newMenuNum;
    Serial.println("menu 0");
  }
  if ((newMenuNum != menuNum) && (newMenuNum == 1))
  {
    drawButtons();
    menuNum = newMenuNum;
    Serial.println("menu 1");
  }
  if ((newMenuNum != menuNum) && (newMenuNum == 2))
  {
    drawStatus();
    menuNum = newMenuNum;
    Serial.println("menu 2");
  }

  if (menuNum == 0)
  {
    doConfig();
  }
  if (menuNum == 1)
  {
    String codeString;
    readButtons(codeString);
  }
  if (menuNum == 2)
  {
  }

  // Read incoming traffic/responses from the TS-890
  while (clientHost.available())
  {
    char c = clientHost.read();
    Serial.print(c);
    lastActivityTime = millis(); // Reset watchdog timer on received data
  }

  // Mandatory 10-second Keep-Alive check (we send a read command every 5 seconds)
  if (clientHost.connected() && (millis() - lastActivityTime > 4900))
  {
    clientHost.print("RM51;");
    clientHost.print("RM;");
    lastActivityTime = millis();
  }

  if (!clientHost.connected() && (millis() - lostHostTime > 10000) && (!isLogout) && (autoLogin))
  {
    Serial.println("\nDisconnected from radio. Retrying in 10 seconds...");
    lostHostTime = millis();
    clientHost.stop();
    connectToRadio();
  }

  void setLED();

} // end loop()

void checkHeader()
{
  bool isTouched = ts.touched();
  int16_t t_x = 0, t_y = 0;

  if (isTouched)
  {
    TS_Point p = ts.getPoint();

    // Map raw touch digits to actual TFT screen pixels
    t_x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    t_y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

    // Check for back menu
    if (t_x > 1 && t_x < (TFT_WIDTH * 0.33) && t_y > 1 && t_y < 50)
    {
      newMenuNum = menuNum - 1;
      if (newMenuNum < 0)
      {
        newMenuNum = maxMenu;
      }
      delay(200); // Debounce delay
    }

    if (t_x > (TFT_WIDTH * 0.66) && t_x < (TFT_WIDTH) && t_y > 1 && t_y < 50)
    {
      newMenuNum = menuNum + 1;
      if (newMenuNum > maxMenu)
      {
        newMenuNum = 0;
      }
      delay(200); // Debounce delay
    }

    // Check for background color change
    if (t_x > (TFT_WIDTH * .33) && t_x < (TFT_WIDTH * 0.66) && t_y > 1 && t_y < 50)
    {
      bgColorIndex++;
      if (bgColorIndex > 2)
      {
        bgColorIndex = 0;
      }
      setBackgroundColor();
      updateHeader();
      preferences.begin("AppSettings", false);           // This is the namespace for local storage
      preferences.putInt("BG_ColorIndex", bgColorIndex); // Write level
      preferences.end();
      delay(200); // Debounce delay
    }
  }
}

void CYD_LED(String Color)
{
  if (Color == "Off")
  {
    digitalWrite(CYD_LED_RED, HIGH);
    digitalWrite(CYD_LED_GREEN, HIGH);
    digitalWrite(CYD_LED_BLUE, HIGH);
  }
  else if (Color == "Red")
  {
    digitalWrite(CYD_LED_RED, LOW);
    digitalWrite(CYD_LED_GREEN, HIGH);
    digitalWrite(CYD_LED_BLUE, HIGH);
  }
  else if (Color == "Green")
  {
    digitalWrite(CYD_LED_RED, HIGH);
    digitalWrite(CYD_LED_GREEN, LOW);
    digitalWrite(CYD_LED_BLUE, HIGH);
  }
  else if (Color == "Blue")
  {
    digitalWrite(CYD_LED_RED, HIGH);
    digitalWrite(CYD_LED_GREEN, HIGH);
    digitalWrite(CYD_LED_BLUE, LOW);
  }
  else if (Color == "Yellow")
  {
    digitalWrite(CYD_LED_RED, LOW);
    digitalWrite(CYD_LED_GREEN, LOW);
    digitalWrite(CYD_LED_BLUE, HIGH);
  }
  else if (Color == "Magenta")
  {
    digitalWrite(CYD_LED_RED, LOW);
    digitalWrite(CYD_LED_GREEN, HIGH);
    digitalWrite(CYD_LED_BLUE, LOW);
  }
  else if (Color == "Cyan")
  {
    digitalWrite(CYD_LED_RED, HIGH);
    digitalWrite(CYD_LED_GREEN, LOW);
    digitalWrite(CYD_LED_BLUE, LOW);
  }
  else if (Color == "White")
  {
    digitalWrite(CYD_LED_RED, LOW);
    digitalWrite(CYD_LED_GREEN, LOW);
    digitalWrite(CYD_LED_BLUE, LOW);
  }
}

void setBackgroundColor()
{
  bgColor = tft.color565(0, 0, 0); // Black
  if (bgColorIndex == 1)
  {
    bgColor = tft.color565(0, 0, 60); // Blue
  }
  else if (bgColorIndex == 2)
  {
    bgColor = tft.color565(0, 60, 50); // Green
  }
  tft.fillScreen(bgColor);
  tft.fillRect(0, 0, TFT_WIDTH, 70, bgColor);
  tft.drawRect(0, 0, TFT_WIDTH, 70, TFT_WHITE);
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  // Turn on Red LED to indicate configuration mode
  CYD_LED("Red");

  // Update CYD Screen
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString("WiFi Portal Active", 120, 40, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Connect your device to:", 120, 90, 2);

  // Display the Access Point SSID broadcasted by the CYD
  String apSSID = myWiFiManager->getConfigPortalSSID();
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(apSSID, 120, 120, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Go to: 192.168.4.1", 120, 170, 2);

  tft.drawCentreString("WiFi portal automatically aborts after", 120, 190, 2);
  tft.drawCentreString(String(PortalTimeout), 120, 210, 2);
  tft.drawCentreString("Seconds", 120, 230, 2);
}

void updateHeader()
{

  // top row
  if (!WiFi.isConnected())
  {
    tft.setTextColor(TFT_DARKGREY, bgColor);
  }
  else
  {
    tft.setTextColor(TFT_CYAN, bgColor);
  }
  tft.drawString(String(mdnsName) + ".local", 2, 3, 2);
  tft.drawCentreString("                  ", TFT_WIDTH / 2, 3, 2);
  tft.drawCentreString(WiFi.localIP().toString(), TFT_WIDTH / 2, 3, 2);
  tft.drawRightString("        ", TFT_WIDTH - 2, 3, 2);

  // middle rows for host info
  if (!clientHost.connected())
  {
    tft.setTextColor(TFT_DARKGREY, bgColor);
  }
  else
  {
    tft.setTextColor(TFT_GREEN, bgColor);
  }
  tft.drawString("                ", 2, 18, 2);
  tft.drawString(hostArray[3], 2, 18, 2);
  // tft.drawCentreString("", TFT_WIDTH / 2, 18, 2);
  tft.drawRightString("                  ", TFT_WIDTH - 2, 18, 2);
  tft.drawRightString(hostArray[1], TFT_WIDTH - 2, 18, 2);

  // bottom row
  tft.setTextColor(TFT_LIGHTGREY, bgColor);
  tft.drawString("<<< MENU", 2, 50, 2);
  tft.drawCentreString("-COLOR-", TFT_WIDTH / 2, 50, 2);
  tft.drawRightString("MENU >>>", TFT_WIDTH - 2, 50, 2);
}

void drawButtons()
{
  // Grid layout layout calculations
  int btnWidth = 114;
  int btnHeight = 42;
  int spacingX = 8;
  int spacingY = 8;
  int startX = 59;
  int startY = 96;
  tft.fillRect(0, 75, TFT_WIDTH, TFT_HEIGHT - 75, bgColor);
  for (int r = 0; r < ROWS; r++)
  {
    for (int c = 0; c < COLS; c++)
    {
      int index = (r * COLS) + c;
      int x = startX + c * (btnWidth + spacingX);
      int y = startY + r * (btnHeight + spacingY);
      tft.setFreeFont(&FreeSans9pt7b);

      buttons[index].initButton(
          &tft, x, y, btnWidth, btnHeight,
          TFT_WHITE, buttonColors[index], TFT_BLACK,
          const_cast<char *>(labelArray[index + 1].c_str()), 1);

      buttons[index].drawButton();
      Serial.println("Buttons Drawn");
    }
  }
}

void drawRSSI()
{
  int32_t dbm = WiFi.RSSI();
  tft.setTextColor(TFT_ORANGE, bgColor);
  if (dbm > -80)
  {
    tft.setTextColor(TFT_YELLOW, bgColor);
  }
  if (dbm > -65)
  {
    tft.setTextColor(TFT_WHITE, bgColor);
  }
  if (dbm > -50)
  {
    tft.setTextColor(TFT_CYAN, bgColor);
  }
  if (dbm == 0)
  {
    tft.setTextColor(TFT_DARKGREY, bgColor);
  }
  tft.drawRightString(String(dbm) + " dBm", tft.width() - 3, 2, 2);
  tft.setTextColor(TFT_WHITE, bgColor);
}

void drawConfig()
{
  // Grid layout layout calculations
  int btnWidth = 114;
  int btnHeight = 42;
  int spacingX = 8;
  int spacingY = 8;
  int startX = 59;
  int startY = 96;
  tft.fillRect(0, 75, TFT_WIDTH, TFT_HEIGHT - 75, bgColor);
  for (int r = 0; r < ROWS; r++)
  {
    for (int c = 0; c < COLS; c++)
    {
      int index = (r * COLS) + c;
      int x = startX + c * (btnWidth + spacingX);
      int y = startY + r * (btnHeight + spacingY);
      /*
      • Sizes: 9pt, 12pt, 18pt, 24pt
      • Families: FreeMono (Monospaced), FreeSans (Sans Serif), FreeSerif (Serif)
      • Styles: Regular, Bold, Oblique (Italic), BoldOblique
        7b = 7 bit charachters
      */
      tft.setFreeFont(&FreeSans9pt7b);
      // black text
      configButtons[index].initButton(
          &tft, x, y, btnWidth, btnHeight,
          TFT_WHITE, configColors[index], TFT_BLACK,
          configLabels[index], 1);

      configButtons[index].drawButton();
      Serial.println("Config Drawn");
    }
  }
}

String readButtons(String &codeString)
{
  bool isTouched = ts.touched();
  int16_t t_x = 0, t_y = 0;

  if (isTouched)
  {
    TS_Point p = ts.getPoint();

    // Map raw touch digits to actual TFT screen pixels
    t_x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    t_y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

    // Debug line to verify touch values are coming through
    // Serial.printf("Raw Touch X:%d Y:%d | Mapped X:%d Y:%d\n", p.x, p.y, t_x, t_y);
  }

  // Process button array states
  for (int i = 0; i < TOTAL_BUTTONS; i++)
  {
    if (isTouched && buttons[i].contains(t_x, t_y))
    {
      buttons[i].press(true);
    }
    else
    {
      buttons[i].press(false);
    }

    if (buttons[i].justReleased())
    {
      buttons[i].drawButton();
    }

    if (buttons[i].justPressed())
    {
      macroCode = macroArray[i + 1];
      Serial.printf("Button %s Pressed!\n", String(i));
      buttons[i].drawButton(true); // Invert colors while pressed

      tft.setTextColor(TFT_GREEN, bgColor);
      tft.fillRect(2, 34, TFT_WIDTH - 4, 15, bgColor);
      tft.drawCentreString(String("Tx: ") + String(macroCode), TFT_WIDTH / 2, 34, 2);
      clientHost.print(macroCode);
    }
  }

  return codeString;
  delay(20);
}

String readConfig(String &codeString)
{
  bool isTouched = ts.touched();
  int16_t t_x = 0, t_y = 0;

  if (isTouched)
  {
    TS_Point p = ts.getPoint();
    // Map raw touch digits to actual TFT screen pixels
    t_x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    t_y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  }

  // Process button array states
  for (int i = 0; i < TOTAL_BUTTONS; i++)
  {
    if (isTouched && configButtons[i].contains(t_x, t_y))
    {
      configButtons[i].press(true);
    }
    else
    {
      configButtons[i].press(false);
    }

    if (configButtons[i].justReleased())
    {
      duration = millis() - touchStartTime;
      // Serial.println("Duration = " + String(duration));
      // Serial.printf("Button %s Released\n", buttonLabels[i]);
      configButtons[i].drawButton();
    }

    if (configButtons[i].justPressed())
    {
      touchStartTime = millis();
      // Serial.printf("Button %s Pressed!\n", configCode[i]);
      codeString = configCode[i];
      // Serial.printf("Button %s Pressed!\n", codeString);
      configButtons[i].drawButton(true); // Invert colors while pressed

      tft.setTextColor(TFT_WHITE, bgColor);
      tft.fillRect(2, 34, TFT_WIDTH - 4, 15, bgColor);
      tft.drawCentreString(String("CONFIG: ") + String(codeString), TFT_WIDTH / 2, 34, 2);
    }
  }
  return codeString;
  delay(20);
}

void doConfig()
{
  String configString = "";
  readConfig(configString);

  if ((configString == "CLR") && (duration > 2000))
  {
    duration = 0;
    wm.resetSettings();
    delay(200); // Debounce delay
  }

  if ((configString == "REBOOT") && (duration > 2000))
  {
    duration = 0;
    ESP.restart();
    delay(200); // Debounce delay
  }

  if (configString == "DIS")
  {
    duration = 0;
    WiFi.disconnect();
    delay(200); // Debounce delay
  }

  if (configString == "ENA")
  {
    duration = 0;
    wm.autoConnect("Zpad WiFi Config");
    delay(200); // Debounce delay
  }

  if (configString == "DIMMER")
  {
    duration = 0;
    BL_Level = BL_Level - 25;
    if (BL_Level < 25)
    {
      BL_Level = 25;
    }
    ledcWrite(channel, BL_Level);
    preferences.begin("AppSettings", false);       // This is the namespace for local storage
    preferences.putInt("BackLight_Lvl", BL_Level); // Write level
    preferences.end();
    delay(200); // Debounce delay
  }

  if (configString == "BRIGHTER")
  {
    duration = 0;
    {
      BL_Level = BL_Level + 25;
      if (BL_Level > 250)
      {
        BL_Level = 250;
      }
      ledcWrite(channel, BL_Level);
      preferences.begin("AppSettings", false);       // This is the namespace for local storage
      preferences.putInt("BackLight_Lvl", BL_Level); // Write level
      preferences.end();
      Serial.println("BL = " + String(BL_Level));
      delay(200); // Debounce delay
    }
  }

  if (configString == "MAN")
  {
    duration = 0;
    autoLogin = false;
    preferences.begin("AppSettings", false); // This is the namespace for local storage
    preferences.putBool("loginMode", false);
    preferences.end();
    delay(200); // Debounce delay
  }

  if (configString == "AUTO")
  {
    duration = 0;
    autoLogin = true;
    preferences.begin("AppSettings", false); // This is the namespace for local storage
    preferences.putBool("loginMode", true);
    preferences.end();
    delay(200); // Debounce delay
  }

  if (configString == "LOGOUT")

  {
    duration = 0;
    clientHost.stop();
    isLogout = true;
  }

  if (configString == "LOGIN")
  {
    duration = 0;
    isLogout = false;
    connectToRadio();
  }
}

void setLED()
{
  if ((clientHost.connected()) && (WiFi.isConnected()))
  {
    CYD_LED("Green");
  }
  if ((!clientHost.connected()) && (WiFi.isConnected()))
  {
    CYD_LED("Cyan");
  }
  if ((!clientHost.connected()) && (!WiFi.isConnected()))
  {
    CYD_LED("Yellow");
  }
}

void drawStatus()
{
  tft.fillRect(0, 75, TFT_WIDTH, TFT_HEIGHT - 75, bgColor);
  tft.setTextColor(TFT_YELLOW, bgColor);
  tft.setCursor(0, 90);
  tft.drawString(String("Version: ") + Ver, 2, 80, 2);
  tft.drawString(String("WD5ACP Guy Zebrick") + Ver, 2, 100, 2);
  tft.drawString("WiFi Network: " + WiFi.SSID(), 2, 120, 2);
  tft.drawString("MAC: " + WiFi.macAddress(), 2, 140, 2);
  tft.drawString("Host IP: " + hostArray[1], 2, 180, 2);
  tft.drawString("Host Port: " + hostArray[2], 2, 200, 2);
  tft.drawString("Host User: " + hostArray[3], 2, 220, 2);
  tft.drawString("Host Password: " + hostArray[4], 2, 240, 2);
}

void connectToRadio()
{
  Serial.print("Connecting to TS-890 at ");
  Serial.println(radioIP);

  if (!clientHost.connect(radioIP, port))
  {
    Serial.println("TCP Connection Failed!");
    return;
  }

  Serial.println("TCP Connected. Sending ##CN command...");
  // Step 1: Send Connection Request command (##CN // --- Web Server Request Handlers ---)
  clientHost.print("##CN;");
  // clientHost.write(0x02);
  // clientHost.write(static_cast<uint8_t>(0x00));

  // Give the radio a brief moment to process and reply
  delay(500);

  // Step 2: Create login ID
  String TotalPass;
  String UserString;
  String UserPass;
  int UserLen = String(hostArray[3]).length();
  int PassLen = String(hostArray[4]).length();

  if (UserLen < 10)
  {
    UserString = "0" + String(UserLen);
  }
  else
  {
    UserString = String(UserLen);
  }

  if (PassLen < 10)
  {
    UserPass = "0" + String(PassLen);
  }
  else
  {
    UserPass = String(PassLen);
  }
  TotalPass = "##ID0" + UserString + UserPass + hostArray[3] + hostArray[4] + ";";

  Serial.println("Using Admin login Code: >>> >> " + TotalPass);
  Serial.println("Sending login credentials...");
  // clientHost.print("##ID");
  clientHost.print(TotalPass);
  // clientHost.write(0x02);
  // clientHost.print(adminPwd);
  // clientHost.write(0x02);

  lastActivityTime = millis();
}

void loadConfiguration()
{
  preferences.begin("cyd_settings", false); // Open namespace "cyd_settings" in RW mode

  // Load Array 1
  for (int i = 1; i < 11; i++)
  {
    String key = "a1_" + String(i);
    labelArray[i] = preferences.getString(key.c_str(), "Default A1-" + String(i));
  }

  // Load Array 2
  for (int i = 1; i < 11; i++)
  {
    String key = "a2_" + String(i);
    macroArray[i] = preferences.getString(key.c_str(), "Default A2-" + String(i));
  }

  // Load 5 Single Strings
  for (int i = 1; i < 6; i++)
  {
    String key = "s_" + String(i);
    hostArray[i] = preferences.getString(key.c_str(), "Default String " + String(i));
  }

  preferences.end();
}

void saveConfiguration()
{
  preferences.begin("cyd_settings", false);

  // Save Array 1
  for (int i = 1; i < 11; i++)
  {
    String key = "a1_" + String(i);
    preferences.putString(key.c_str(), labelArray[i]);
  }

  // Save Array 2
  for (int i = 1; i < 11; i++)
  {
    String key = "a2_" + String(i);
    preferences.putString(key.c_str(), macroArray[i]);
  }

  // Save 5 Single Strings
  for (int i = 1; i < 6; i++)
  {
    String key = "s_" + String(i);
    preferences.putString(key.c_str(), hostArray[i]);
  }
  preferences.end();
}

void handleRoot() // --- Web Server Request Handlers ---
{
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";

  // UI Styling with Flexbox columns
  html += "<style>";
  html += "body{font-family:sans-serif; background:#f4f4f9; padding:20px; margin:0;}";
  html += "h2{color:#333; text-align:center;}";
  html += ".container{max-width:1000px; margin:0 auto;}";
  html += ".row{display:flex; flex-wrap:wrap; gap:20px;}";
  html += ".col{flex:1; min-width:200px;}";
  html += ".card{background:white; padding:15px; margin-bottom:15px; border-radius:8px; box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += "label{font-weight:bold; color:#555; display:block; margin-top:8px;}";
  html += "input[type=text]{width:100%; padding:8px; margin:5px 0 10px 0; border:1px solid #ccc; border-radius:4px; box-sizing:border-box;}";
  html += "button{display:block; width:100%; max-width:200px; margin:20px auto; background:#4CAF50; color:white; padding:12px; border:none; border-radius:4px; cursor:pointer; font-size:16px; font-weight:bold;}";
  html += "button:hover{background:#45a049;}";
  html += "</style>";

  // html += "<title>CYD Configuration Panel</title></head><body>";
  html += String("<title>") + mdnsName + " Configuration Panel</title></head><body>";
  html += "<div class='container'>";
  html += String("<h2>") + mdnsName + " Configuration Panel</h2>";
  html += "<form action='/save' method='POST'>";

  // Flexbox Row containing the 2 side-by-side columns
  html += "<div class='row'>";

  // Column 1: Array 1
  html += "<div class='col'><div class='card'><h3>Button Label</h3>";
  for (int i = 1; i < 11; i++)
  {
    html += "<label>Button " + String(i) + ":</label>";
    html += "<input type='text' maxlength='9' name='a1_" + String(i) + "' value='" + labelArray[i] + "'>";
  }
  html += "</div></div>";

  // Column 2: Array 2
  html += "<div class='col'><div class='card'><h3>Command String (include end of line code)</h3>";
  for (int i = 1; i < 11; i++)
  {
    html += "<label>Code " + String(i) + ":</label>";
    html += "<input type='text' name='a2_" + String(i) + "' value='" + macroArray[i] + "'>";
  }
  html += "</div></div>";

  html += "</div>"; // End of row

  // Standalone Settings (Below the columns)
  html += "<div class='card'><h3>Host Config IP, PORT, USER NAME, PASSWORD, SPARE</h3>";
  for (int i = 1; i < 6; i++)
  {
    html += "<label>String Parameter " + String(i) + ":</label>";
    html += "<input type='text' name='s_" + String(i) + "' value='" + hostArray[i] + "'>";
  }
  html += "</div>";

  html += "<button type='submit'>Save to Non-Volatile Memory</button>";
  html += "</form></div></body></html>";

  server.send(200, "text/html", html);
}

void handleSave()
{
  // Process Array 1 input
  for (int i = 1; i < 11; i++)
  {
    String key = "a1_" + String(i);
    if (server.hasArg(key))
    {
      labelArray[i] = server.arg(key);
    }
  }

  // Process Array 2 input
  for (int i = 1; i < 11; i++)
  {
    String key = "a2_" + String(i);
    if (server.hasArg(key))
    {
      macroArray[i] = server.arg(key);
    }
  }

  // Process 5 Standalone strings
  for (int i = 1; i < 6; i++)
  {
    String key = "s_" + String(i);
    if (server.hasArg(key))
    {
      hostArray[i] = server.arg(key);
    }
  }

  // Commit parameters straight to Flash NVS
  saveConfiguration();

  // Redirect back to root dashboard with a success message
  String response = "<html><body><h1>Settings Saved to NVS Memory!</h1><a href='/'>Go Back</a></body></html>";
  server.send(200, "text/html", response);
}
