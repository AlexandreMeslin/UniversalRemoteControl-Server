//#define DEBUG

#include <string.h>
#include <string>
#include <cstring>
#include <stdlib.h>

// Wi-Fi + OTA
#include <credentials.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>

// From: https://github.com/crankyoldgit/IRremoteESP8266/blob/master/examples/DumbIRRepeater/DumbIRRepeater.ino
// Include the necessary libraries:
// IRRemoteESP8266 by David Conran, Sebastien Warin, Mark Szabo, Ken Shirriff
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ir_Coolix.h>

// UDP
WiFiUDP udp;
#define UDP_PORT    80
#define BUFFER_SIZE 1024

/*
 * Macros
 */
// Saída de texto
#ifdef DEBUG
#define print(x)    {Serial.print(x);   client.print(x);}
#define println(x)  {Serial.println(x); client.println(x);}
#else
#define print(x)    {}
#define println(x)  {}
#endif

/*
 * Definições para IR
 */
// The following numbers *MUST* match what is set in client application (see Constants class at client)
#define SETTINGS  0
#define NEC       1
#define MIDEA     2
#define SONY      3
#define SAMSUNG   4
// Define the IR LED pin and IR receiver pin:
#define IR_LED_PIN 13
//#define IR_RECEIVER_PIN 5
// Create objects for IR sending and receiving:
IRsend irsend(IR_LED_PIN);
//IRrecv irrecv(IR_RECEIVER_PIN);
decode_results results;
// kFrequency is the modulation frequency all messages will be replayed at.
const uint16_t kFrequency = 38;  // in kHz. e.g. 38kHz.
IRCoolixAC acMidea(IR_LED_PIN);

/*
 * Constants & variables
 */
#ifdef NCE
#define HOST "10.10.15.222"
#define PORT (8752)
#else
#define HOST "10.0.0.195"
#define PORT (8752)
#endif

// WiFi 
#ifdef NCE
const char* ssid = "hsNCE";
const char* password = "";
#else
const char* ssid = MY_SSID;
const char* password = MY_PASSWORD;
#endif

// WiFi console
WiFiClient client;

// Protótipos
void blink(void);
unsigned int hexToUnsigned(char *p);
unsigned int hexToUnsigned(uint8_t *p);

/**
Initialize the IR receiver in the setup function:
- Turn builtin led ON
- Setup serial interface
- Setup WiFi
- Setup Ota
- Setup Client console connection
- Setup UDP server
*/
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Turns on builtin led

#if DEBUG
  // Setup serial interface
  Serial.begin(115200);
#endif

  // Setup WiFi
  println("Booting");
  WiFiManager wifiManager;
  wifiManager.autoConnect("ACMeslinApp");
  /*
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    println("Connection Failed! Rebooting...");
    delay(10 * 1000);
    ESP.restart();
  }
  */
  println("Connected to WiFi");

  // Hostname defaults to esp8266-[ChipID]
  // hostname = IRServer+<IP address>
  ArduinoOTA.setHostname(("IRServer-"+WiFi.localIP().toString()).c_str());
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    printf("Progress: %u%%\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      println("End Failed");
    }
  });
  ArduinoOTA.begin();
  println("Ready!");
  print("IP address: ");
  println(WiFi.localIP().toString());

#ifdef DEBUG
  // WiFi console
  if(!client.connect(HOST, PORT)) {
    print("Could not connect to WiFi console at ");
  } else {
    print("Connected to WiFi console at ");  
  }
  print(HOST);
  print(":");
  println(PORT);
  delay(1000);
#endif

  // inicialize UDP server
  udp.begin(UDP_PORT);

  pinMode(IR_LED_PIN, OUTPUT);    // Provavelmente desnecessário

  // Turn off builtin led indicating that everything is allright
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED off (HIGH is the voltage level)
}

// In the loop function, receive IR signals and process them:
void loop() {
  uint8_t buffer[BUFFER_SIZE];
  uint8_t *pNext; // pointer to the second part of command

  ArduinoOTA.handle();

  int packetSize = udp.parsePacket();
  if(packetSize) {
    int len = udp.read(buffer, BUFFER_SIZE);
    if(len > 0) {
      buffer[len] = 0;  // set the end-of-string
      print("pacote UDP recebido: ");
      println((char *)buffer);
      // first part has 3 digits
      buffer[3] = '\0';
      pNext = &buffer[4];
      switch(atoi((char *)buffer)) {
        case SETTINGS:
          settings(pNext);
          break;
        case NEC:
          nec(pNext);
          break;
        case MIDEA:
          midea(pNext);
          break;
        case SONY:
          sony(pNext);
          break;
        case SAMSUNG:
          samsung(pNext);
          break;
        default:
          break;
      }
    } 
  }
}

/**
Excecutes settings command
String format: XX
*/
void settings(uint8_t *p) {
  switch(atoi((char *)p)) {
    case 0:
      blink();
      break;
    default:
      break;
  }
}

/**
Executes NEC-compatible IR commands
String format: 0xXXXXXXXX,nn
  0xXXXXXXXX -> command in hex
  nn -> size in decimal
*/
void nec(uint8_t *p) {
  uint8_t *pSize;
  unsigned command;
  int size;

  // First split the string in command and size
  pSize = (uint8_t *)strchr((char *)p, ',');
  *pSize = (uint8_t)'\0';
  pSize++;
  command = hexToUnsigned(&p[2]);
  size = atoi((char *)pSize);

  // send IR code
  irsend.sendNEC(command, size);
}

/**
Executes SONY-compatible IR commands
String format: 0xXXXXXX,nn
  0XXXXXXX -> command in hex
  nn -> size in decimal
*/
void sony(uint8_t *p) {
  uint8_t *pSize;
  unsigned command;
  int size;

  // First split the string in command and size
  pSize = (uint8_t *)strchr((char *)p, ',');
  *pSize = (uint8_t)'\0';
  pSize++;
  command = hexToUnsigned(&p[2]);
  size = atoi((char *)pSize);

  // send IR code
  irsend.sendSony(command, size);
}

/**
Executes Samsung-compatible IR commands
String format: 0xXXXXXX,nn
  0XXXXXXX -> command in hex
  nn -> size in decimal
*/
void samsung(uint8_t *p) {
  uint8_t *pSize;
  unsigned command;
  int size;

  // First split the string in command and size
  pSize = (uint8_t *)strchr((char *)p, ',');
  *pSize = (uint8_t)'\0';
  pSize++;
  command = hexToUnsigned(&p[2]);
  size = atoi((char *)pSize);

  // send IR code
  irsend.sendSAMSUNG(command, size);
}

/**
Execute Midea-compatible IR commands
String format: nnn..n (number in decimal)
*/
void midea(uint8_t *p) {
  unsigned int command;

  command = atoi((char *)p);
  switch(command) {
    case 1:
      acMidea.on();
      acMidea.send();
      break;
    case 2:
      acMidea.off();
      acMidea.send();
      break;

    case 3:
      acMidea.setMode(kCoolixAuto);
      acMidea.send();
      break;
    case 4:
      acMidea.setMode(kCoolixCool);
      acMidea.send();
      break;
    case 5:
      acMidea.setMode(kCoolixDry);
      acMidea.send();
      break;
    case 6:
      acMidea.setMode(kCoolixFan);
      acMidea.send();
      break;

    case 7:
      acMidea.setTemp(17);
      acMidea.send();
      break;
    case 8:
      acMidea.setTemp(18);
      acMidea.send();
      break;
    case 9:
      acMidea.setTemp(19);
      acMidea.send();
      break;
    case 10:
      acMidea.setTemp(20);
      acMidea.send();
      break;
    case 11:
      acMidea.setTemp(21);
      acMidea.send();
      break;
    case 12:
      acMidea.setTemp(22);
      acMidea.send();
      break;
    case 13:
      acMidea.setTemp(23);
      acMidea.send();
      break;
    case 14:
      acMidea.setTemp(24);
      acMidea.send();
      break;
    case 15:
      acMidea.setTemp(25);
      acMidea.send();
      break;
    case 16:
      acMidea.setTemp(26);
      acMidea.send();
      break;

    case 17:
      acMidea.setFan(kCoolixFanAuto);
      acMidea.send();
      break;
    case 18:
      acMidea.setFan(kCoolixFanMin);
      acMidea.send();
      break;
    case 19:
      acMidea.setFan(kCoolixFanMed);
      acMidea.send();
      break;
    case 20:
      acMidea.setFan(kCoolixFanMax);
      acMidea.send();
      break;

    case 21:
      acMidea.setSwing();
      acMidea.send();
      break;
    default:
      break;
  }
}

/**
Blinks build-in led for 1 second.
At the end, the led will be off.
*/
void blink() {
  digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on (Note that LOW is the voltage level
  // but actually the LED is on; this is because
  // it is active low on the ESP-01)
  delay(1000);                      // Wait for a second
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

/**
Convert a string containing a hex number to unsigned int
a = 61 0110 0001
A = 41 0100 0001
    DF 1101 1111
*/
unsigned int hexToUnsigned(uint8_t *p) {return hexToUnsigned((char *)p);}
unsigned int hexToUnsigned(char *p) {
  unsigned int res = 0;
  while(*p) {
    if(isdigit(*p)) res = res * 16 + (*p) - '0';
    else res = res * 16 + (*p & 0xDF) - 'A' + 10;
    p++;
  }
  return res;
}