// sketch feb 13, 2023 cankurttekin

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#ifdef ARDUINO_ESP32C3_DEV
const uint16_t kRecvPin = 2;  // 14 on a ESP32-C3 causes a boot loop.
#else                         // ARDUINO_ESP32C3_DEV
const uint16_t kRecvPin = 2;
#endif                        // ARDUINO_ESP32C3_DEV

IRrecv irrecv(kRecvPin);

decode_results results;

// variables
// menu variables
int upButton = 10;
int downButton = 11;
int selectButton = 12;
int menu = 1;
int randNumber;

WiFiClient client;

const char apiUrl[] = "api.openweathermap.org";
// Latitude and Longitude for weather location
String lat = "39,78"; 
String lon = "30,52"; // eskisehir, turkey
String apiKey = "_API_KEY_HERE_";

String text;

int jsonend = 0;
boolean startJson = false;
int status = WL_IDLE_STATUS;

#define JSON_BUFF_DIMENSION 2500

unsigned long lastConnectionTime = 10 * 60 * 1000;  // last time you connected to the server, in milliseconds
const unsigned long postInterval = 10 * 60 * 1000;  // posting interval of 10 minutes  (10L * 1000L; 10 seconds delay for testing)


const long utcOffset = 10800;
char daysOfTheWeek[7][12] = { "Pazar", "Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi" };

// Network Time Protocol
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffset);

int i = 0;
int statusCode;
const char* ssid = "text";
const char* passphrase = "text";
String st;
String content;

// function declaration
bool testWifi(void);
void launchWeb(void);
void setupAP(void);
void updateMenu(void);
void executeAction(void);

// local server at port 80 when required
ESP8266WebServer server(80);

// lcd custom character variables
byte Heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

byte Alarm[8] = {
  0b00100,
  0b01110,
  0b01110,
  0b01110,
  0b11111,
  0b11111,
  0b00100,
  0b00000
};

byte Yagmur[8] = {
  0b00101,
  0b01111,
  0b00110,
  0b10000,
  0b00001,
  0b00100,
  0b10001,
  0b00100
};

byte Thunder[8] = {
  0b01100,
  0b11110,
  0b00100,
  0b00100,
  0b01000,
  0b10000,
  0b10000,
  0b00000
};

byte MerdivenAlt[8] = {
  0b00000,
  0b00000,
  0b00001,
  0b00001,
  0b00111,
  0b00111,
  0b11111,
  0b11111
};


byte Zemin[8] = {
  B00000,
  B00000,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};


byte PersonTall[8] = {
  B00110,
  B00110,
  B00000,
  B01111,
  B10110,
  B00110,
  B01011,
  B11001
};


byte PersonShort[8] = {
  B00000,
  B00110,
  B00110,
  B00000,
  B11111,
  B00100,
  B11010,
  B10011
};

byte Dog[8] = {
  B00000,
  B00000,
  B00000,
  B00011,
  B10011,
  B11110,
  B11110,
  B10010
};


void setup() {
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, Yagmur);
  lcd.createChar(1, Thunder);
  lcd.createChar(2, Alarm);
  lcd.createChar(3, MerdivenAlt);
  lcd.createChar(4, Heart);
  lcd.createChar(5, PersonTall);
  lcd.createChar(6, PersonShort);
  lcd.createChar(7, Dog);

  lcd.clear();
  lcd.print("WEATHER STATION");
  lcd.setCursor(0, 1);
  lcd.print("      v1.0      ");

  delay(1200);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" XXX ");
  lcd.setCursor(0, 1);
  lcd.print(" XXX ");
  delay(1200);


  Serial.begin(115200);

  text.reserve(JSON_BUFF_DIMENSION);

  Serial.println("disconnecting wifi");
  WiFi.disconnect();

  EEPROM.begin(512);  // initializing EEPROM
  delay(10);
  /*
  for (int i = 0; i < 96; ++i) {  // EEPROM cleaning for debugging
    EEPROM.write(i, 0);
  }
*/
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("started");

  // Read eeprom for ssid and password
  Serial.println("reading wifi ssid from EEPROM...");

  String epssid;
  for (int i = 0; i < 32; ++i) {
    epssid += char(EEPROM.read(i));
  }
  Serial.println("SSID: ");
  Serial.println(epssid);
  Serial.println("reading pass from EEPROM...");

  String eppass = "";
  for (int i = 32; i < 96; ++i) {
    eppass += char(EEPROM.read(i));
  }
  Serial.print("PASSWORD: ");
  Serial.println(eppass);

  WiFi.begin(epssid.c_str(), eppass.c_str());

  if (testWifi()) {
    Serial.println("connected.");
    irrecv.enableIRIn();  // Start the receiver
    Serial.println();
    Serial.print("IRrecvDemo is now running and waiting for IR message on Pin: ");
    Serial.println(kRecvPin);
    timeClient.begin();
    updateMenu();
    return;
  } else {
    Serial.println("Turning the AP On");
    launchWeb();
    setupAP();  // setup accesspoint 
  }

  while ((WiFi.status() != WL_CONNECTED)) {
    Serial.print(".");
    delay(100);
    server.handleClient();
  }
}


void loop() {
  if ((WiFi.status() == WL_CONNECTED)) {
    // menu

    if (irrecv.decode(&results)) {

      serialPrintUint64(results.value, HEX);
      Serial.println("");
      //Serial.print(results.value);
      //Serial.println(".");
      uint64_t x = results.value;
      Serial.println(x);
      switch (results.value) {
        case 0xF7807F:
          menu++;
          updateMenu();
          delay(100);
          break;
        case 0xF700FF:
          menu--;
          updateMenu();
          delay(100);
          break;
        case 0xF720DF:
          executeAction();
          updateMenu();
          delay(100);
          break;
        case 0xF740BF:
          menu = 1;
          break;
      }
    }
    irrecv.resume();
  }
  delay(400);
}

// WiFi credentials test
bool testWifi(void) {
  int c = 0;
  Serial.println("waiting for wifi to connect");
  while (c < 20) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("connection timed out.");
  return false;
}

void launchWeb() {
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");

  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan completed");
  if (n == 0)
    Serial.println("No WiFi Networks found");
  else {
    Serial.print(n);
    Serial.println(" Networks found");
    for (int i = 0; i < n; ++i) {
      // printing SSID and RSSI for found networks
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }

  Serial.println("");
  st = "<dl>";
  for (int i = 0; i < n; ++i) {
    // Print SSID and RSSI for each network found
    st += "<dt>";
    st += WiFi.SSID(i);
    st += "</dt><dd>";
    st += "Sinyal Gücü(rssi): ";
    st += WiFi.RSSI(i);


    st += " Şifreli: ";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "Hayır" : "Evet";
    st += "</dd><hr>";
  }
  st += "</dl>";
  delay(100);
  WiFi.softAP("_SSID_NAME_HERE_FOR_AP_", "_PASSWORD_HERE_");
  Serial.println("Initializing_Wifi_accesspoint");
  launchWeb();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("wifi set mode");
  lcd.setCursor(0, 1);
  lcd.print("connect to ap");

  delay(1800);

  lcd.clear();
  lcd.print("PASS:_PASS_HERE_");
  lcd.setCursor(0, 1);
  lcd.print("http:192.168.4.1");

  Serial.println("over");
}

void createWebServer() {
  {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><b style=\"font-size:160%;\">[weather station]</b> &nbsp; <i>Wi-Fi kurulumu</i> ";
      content += ipStr;
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"tara\"></form>";
      content += "<form method='get' action='setting'><label>Ağ Adı: </label><input name='ssid' length=32> <br> <label>Şifre: </label> <input name='pass' length=64><br><input type='submit'></form>";
      content += "Kullanılabilir ağlar listeleniyor";

      content += "<p>";
      content += st;
      content += "</p>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

      content = "<!DOCTYPE HTML>\r\n<html>geri";
      server.send(200, "text/html", content);
    });

    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i) {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i) {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();

        content = "{\"Success\":\"EEPROM' kaydedildi...\"}";
        statusCode = 200;
        ESP.reset();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
    });
  }
}


void makeRequest() {
  // close any connection before sending a new request to allow client make connection to server
  client.stop();
  Serial.print("2 ");
  // if theres a successful connection:
  if (client.connect(apiUrl, 80)) {
    Serial.print("3 ");
    // Serial.println("connecting...");
    // send the HTTP PUT request:
    client.println("GET /data/2.5/weather?lat=" + lat + "&lon=" + lon /*+ "&lang=tr"*/ + "&appid=" + apiKey + "&mode=json&units=metric&cnt=2 HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
    Serial.print("4 ");
    unsigned long timeout = millis();
    while (client.available() == 0) {
      Serial.print("5 ");
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }

    char c = 0;
    Serial.print(" 6");
    while (client.available()) {
      c = client.read();
      Serial.print("7 ");
      // since json contains equal number of open and close curly brackets, 
      // we can determine when a json is completely received  by counting
      // the open and close occurences
      //Serial.print(c);
      if (c == '{') {
        startJson = true;  // set startJson true to indicate json message has started
        jsonend++;
      }
      if (c == '}') {
        jsonend--;
      }
      if (startJson == true) {
        text += c;
      }

      // if jsonend = 0 then we have have received equal number of curly braces
      if (jsonend == 0 && startJson == true) {
        Serial.print(" 8");
        parseJson(text.c_str());  // parse c string text in parseJson function
        text = "";                // clear text string for the next time
        startJson = false;        // set startJson to false to indicate that a new message has not yet started
      }
    }
  } else {
    Serial.print("9 ");
    // if no connction was made:
    Serial.println("connection failed");
    return;
  }
}

//to parse json data recieved from OWM
void parseJson(const char* jsonString) {
  Serial.print("10 ");
  //StaticJsonBuffer<4000> jsonBuffer;
  const size_t bufferSize = 2 * JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + 4 * JSON_OBJECT_SIZE(1) + 3 * JSON_OBJECT_SIZE(2) + 3 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 2 * JSON_OBJECT_SIZE(7) + 2 * JSON_OBJECT_SIZE(8) + 720;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  // FIND FIELDS IN JSON TREE
  JsonObject& root = jsonBuffer.parseObject(jsonString);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  JsonArray& list = root["list"];

  String city = root["name"];
  String weather = root["weather"][0]["description"];
  float temp = root["main"]["temp"];

  lcd.setCursor(10, 0);
  lcd.print(temp);
  lcd.setCursor(14, 0);
  lcd.write(223);
  lcd.setCursor(15, 0);
  lcd.print("C");

  lcd.setCursor(12, 1);
  lcd.write(7);
  lcd.setCursor(13, 1);
  lcd.write(6);
  lcd.setCursor(14, 1);
  lcd.write(5);

  lcd.setCursor(0, 1);

  if (weather == "broken clouds") {
    lcd.write(243);
    lcd.setCursor(1, 1);
    lcd.print("ParBulut");
  } else if (weather == "snow") {
    lcd.print("*Kar");
  } else if (weather == "mist") {
    lcd.print("=Sis");
  } else if (weather == "thunderstorm") {
    lcd.write(byte(1));
    lcd.setCursor(1, 1);
    lcd.print("Firtina");
  } else if (weather == "rain") {
    lcd.write(byte(0));
    lcd.setCursor(1, 1);
    lcd.print("Yagmur");
  } else if (weather == "shower rain") {
    lcd.write(byte(0));
    lcd.setCursor(1, 1);
    lcd.print("SagYagmur");
  } else if (weather == "scattered clouds") {
    lcd.write(243);
    lcd.setCursor(1, 1);
    lcd.print("SeyBulut");
  } else if (weather == "few clouds") {
    lcd.write(243);
    lcd.print("Bulutlu");
  } else if (weather == "clear sky") {
    lcd.write(223);
    lcd.setCursor(1, 1);
    lcd.print("Acik");
  } else {
    lcd.print(weather);
  }
}


void updateMenu() {
  switch (menu) {
    case 0:
      menu = 1;
      break;
    case 1:
      lcd.clear();
      lcd.write(126);
      lcd.print("SAAT");
      lcd.setCursor(0, 1);
      lcd.print(" ALARM");

      break;
    case 2:
      lcd.clear();
      lcd.print(" SAAT");
      lcd.setCursor(0, 1);
      lcd.write(126);
      lcd.print("ALARM");
      break;
    case 3:
      lcd.clear();
      lcd.write(126);
      lcd.print("randno to decide");
      lcd.setCursor(0, 1);
      lcd.print(" Animation");
      break;
    case 4:
      lcd.clear();
      lcd.print(" randno to decid");
      lcd.setCursor(0, 1);
      lcd.write(126);
      lcd.print("Animation");
      break;
    case 5:
      menu = 4;
      break;
  }
}

void executeAction() {
  switch (menu) {
    case 1:
      lcd.clear();
      saat();
      break;
    case 2:
      alarm();
      break;
    case 3:
      secici();
      break;
    case 4:
      deneme();
      break;
    case 5:
      action5();
      break;
  }
}

void saat() {
  while (true) {
    if (millis() - lastConnectionTime > postInterval) {

      // note the time that the connection was made:
      lastConnectionTime = millis();
      makeRequest();
    }

    timeClient.update();
    lcd.setCursor(0, 0);
    lcd.print(timeClient.getFormattedTime());

    String strTime = String(timeClient.getFormattedTime()).substring(0, 5);
    if (strTime == "00:00") {
      lcd.clear();
      Serial.println(strTime.substring(0, 5));
      lcd.setCursor(0, 1);
      lcd.print("make a wish!");
    }
    if(strTime == "00:01"){ // crappy solution, will be replaced later, i hope
      lcd.clear();
    }
    delay(1000);
  }
}

void alarm() {
  lcd.clear();
  lcd.print("gelistiriliyo :'");
  delay(1500);
}

void secici() {
  lcd.clear();
  lcd.setCursor(0,0);
  randNumber = ((random(1, 100) / 10) % 2) + 1;
  lcd.print(randNumber);
  lcd.setCursor(1,0);
  lcd.print("'lorem ipsum do ");
  lcd.setCursor(0,1);
  lcd.print("lorem ipsum dolo");
  delay(1800);
}

void deneme() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.write(2);

  lcd.setCursor(1, 0);
  lcd.write(2);

  lcd.setCursor(4, 0);
  lcd.write(2);

  lcd.setCursor(6, 0);
  lcd.write(2);

  lcd.setCursor(9, 0);
  lcd.write(2);

  lcd.setCursor(11, 0);
  lcd.write(2);

  lcd.setCursor(12, 0);
  lcd.write(2);

  lcd.setCursor(13, 0);
  lcd.write(2);

  lcd.setCursor(15, 0);
  lcd.write(2);

  delay(1000);
  i = 16;
  while (i > 0) {
    for (i; i >= 0; i--) {
      lcd.setCursor(i, 1);
      lcd.write(byte(5));

      delay(400);

      lcd.setCursor(i, 1);
      lcd.write(32);
    }
  }

  lcd.clear();
  // ground
  lcd.setCursor(15, 1);
  lcd.write(255);
  // stairs
  lcd.setCursor(14, 1);
  lcd.write(byte(3));

  lcd.setCursor(8, 0);
  lcd.write(255);
  lcd.setCursor(5, 0);
  lcd.print("???");

  lcd.setCursor(3, 0);
  lcd.write(255);
  lcd.setCursor(3, 1);
  lcd.write(255);
  lcd.setCursor(2, 0);
  lcd.write(219);
  lcd.setCursor(2, 1);
  lcd.write(219);
  lcd.setCursor(1, 0);
  lcd.write(255);
  lcd.setCursor(1, 1);
  lcd.write(255);
  lcd.setCursor(0, 0);
  lcd.write(255);
  lcd.setCursor(0, 1);
  lcd.write(255);

  lcd.setCursor(5, 1);
  lcd.print("|");

  delay(600);

  // karakter zemin
  lcd.setCursor(15, 0);
  lcd.write(byte(5));

  delay(300);
  lcd.setCursor(15, 0);
  lcd.write(32);

  // karakter merdiven
  lcd.setCursor(14, 0);
  lcd.write(byte(5));

  delay(300);

  lcd.setCursor(14, 0);
  lcd.write(32);

  // karakter yer
  lcd.setCursor(13, 0);
  lcd.write(byte(0));
  lcd.setCursor(13, 1);
  lcd.write(byte(5));

  delay(300);

  lcd.setCursor(13, 1);
  lcd.write(32);
  lcd.setCursor(13, 0);
  lcd.write(32);

  delay(300);

  lcd.setCursor(13, 1);
  lcd.write(32);

  // karakter adım
  lcd.setCursor(12, 0);
  lcd.write(byte(0));
  lcd.setCursor(12, 1);
  lcd.write(byte(5));

  delay(1000);

  // dog 1
  lcd.setCursor(4, 1);
  lcd.write(byte(7));

  delay(300);

  lcd.setCursor(12, 0);
  lcd.write(32);

  delay(600);

  lcd.write(32);

  // dog 2
  lcd.setCursor(5, 1);
  lcd.write(byte(7));

  delay(300);

  // karakter2 1
  lcd.setCursor(4, 1);
  lcd.write(byte(6));

  delay(600);

  // dog 3
  lcd.setCursor(5, 1);
  lcd.write(32);
  lcd.setCursor(6, 1);
  lcd.write(byte(7));

  delay(300);

  // karakter2 2
  lcd.setCursor(4, 1);
  lcd.write(32);
  lcd.setCursor(5, 1);
  lcd.write(byte(6));

  delay(300);
  // dog 4
  lcd.setCursor(6, 1);
  lcd.write(32);
  lcd.setCursor(7, 1);
  lcd.write(byte(7));

  delay(300);

  // dog 5
  lcd.setCursor(7, 1);
  lcd.write(32);
  lcd.setCursor(8, 1);
  lcd.write(byte(7));

  delay(300);

  // dog 6
  lcd.setCursor(8, 1);
  lcd.write(32);
  lcd.setCursor(9, 1);
  lcd.write(byte(7));

  delay(300);

  // karakter2 3
  lcd.setCursor(5, 1);
  lcd.write(32);
  lcd.setCursor(6, 1);
  lcd.write(byte(6));

  delay(300);

  // karakter3 4
  lcd.setCursor(6, 1);
  lcd.write(32);
  lcd.setCursor(7, 1);
  lcd.write(byte(6));

  delay(300);

  // karakter4 5
  lcd.setCursor(7, 1);
  lcd.write(32);
  lcd.setCursor(8, 1);
  lcd.write(byte(6));

  delay(300);

  // karakter5 5
  lcd.setCursor(8, 1);
  lcd.write(32);
  lcd.setCursor(10, 1);
  lcd.write(byte(6));

  delay(300);

  // karakter 2
  lcd.setCursor(12, 1);
  lcd.write(32);
  lcd.setCursor(11, 1);
  lcd.write(byte(5));

  delay(600);
  // THUNDERSTRUCK
  lcd.setCursor(10, 1);
  lcd.write(1);
  delay(300);
  lcd.setCursor(11, 1);
  lcd.write(1);
  delay(300);
  lcd.setCursor(9, 1);
  lcd.write(1);

  delay(300);
  for (i = 0; i < 16; i++) {
    for (int j = 0; j < 2; j++) {
      lcd.setCursor(i, j);
      lcd.write(1);
    }
  }

  delay(600);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" lorem ipsum");
  lcd.setCursor(0, 1);
  lcd.print("dolor sit amet");
  delay(10000);
  delay(1500);
}

void action5() {
  lcd.clear();
  lcd.print(">fonk aktif #5");
  delay(1500);
}