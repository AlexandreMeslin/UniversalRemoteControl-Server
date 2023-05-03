//#define DEBUG
//#define SECURE

#include <string.h>
#include <string>
#include <cstring>

// Wi-Fi + OTA
#include <credentials.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

// Web Server
#ifdef SECURE
#include <ESP8266WebServerSecure.h>
#else
#include <ESP8266WebServer.h>
#endif

// From: https://github.com/crankyoldgit/IRremoteESP8266/blob/master/examples/DumbIRRepeater/DumbIRRepeater.ino
// Include the necessary libraries:
// IRRemoteESP8266 by David Conran, Sebastien Warin, Mark Szabo, Ken Shirriff
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ir_Coolix.h>

/*
 * Macros
 */
// Saída de texto
#ifdef DEBUG
#define print(x)    {Serial.print(x);   client.print(x);}
#define println(x)  {Serial.println(x); client.println(x);}
#else
#define print(x)    {client.print(x);}
#define println(x)  {client.println(x);}
#endif

/*
 * Definições para IR
 */
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
#define HOST "10.0.0.25"
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

// Web Server
#ifdef SECURE
ESP8266WebServerSecure server(443);
#else
ESP8266WebServer server(80);
#endif

// Initialize the IR receiver in the setup function:
void setup() {
  Serial.begin(115200);
//  irrecv.enableIRIn();

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(IR_LED_PIN, OUTPUT);    // Provavelmente desnecessário
  println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    println("Connection Failed! Rebooting...");
    delay(10 * 1000);
    ESP.restart();
  }
  println("Connected to WiFi");

  // Hostname defaults to esp8266-[ChipID]
  // hostname = IRServer+<IP address>
//  ArduinoOTA.setHostname((std::string("IRServer-")+std::string(WiFi.localIP().toString().c_str())).c_str());
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

  setupWebServerForSettings();
  setupWebServerForNet();
  setupWebServerForAC();

  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED off (HIGH is the voltage level)
}

// In the loop function, receive IR signals and process them:
void loop() {
  ArduinoOTA.handle();
  server.handleClient();
}

// Implement the IR sending function:
void sendIR(const uint16_t *data, int nbits) {
  irsend.sendRaw(data, nbits, kFrequency);
}

/**
*/
void setupWebServerForSettings() {
  server.on("/settingsBlink", []() {
    blink();
    server.send(200, "text/html", "SettingsBlink");
  });
}

/**
*/
void setupWebServerForAC() {
  server.on("/acOn", [](){
    blink();
    acMidea.on();
    acMidea.send();
    server.send(200, "text/html", "MideaOn");
  });
  server.on("/acOff", [](){
    blink();
    acMidea.off();
    acMidea.send();
    server.send(200, "text/html", "MideaOff");
  });

  server.on("/acModeAuto", [](){
    acMidea.setMode(kCoolixAuto);
    acMidea.send();
    server.send(200, "text/html", "MideaMode");
  });

  server.on("/acModeCool", [](){
    acMidea.setMode(kCoolixCool);
    acMidea.send();
    server.send(200, "text/html", "MideaMode");
  });

  server.on("/acModeDry", [](){
    acMidea.setMode(kCoolixDry);
    acMidea.send();
    server.send(200, "text/html", "MideaMode");
  });

  server.on("/acModeFan", [](){
    acMidea.setMode(kCoolixFan);
    acMidea.send();
    server.send(200, "text/html", "MideaMode");
  });

  server.on("/ac17", [](){
    acMidea.setTemp(17);
    acMidea.send();
    server.send(200, "text/html", "MideaTemperature");
  });

  server.on("/ac18", [](){
    acMidea.setTemp(18);
    acMidea.send();
    server.send(200, "text/html", "MideaTemperature");
  });

  server.on("/ac19", [](){
    acMidea.setTemp(19);
    acMidea.send();
    server.send(200, "text/html", "MideaTemperature");
  });

  server.on("/ac20", [](){
    acMidea.setTemp(20);
    acMidea.send();
    server.send(200, "text/html", "MideaTemperature");
  });

  server.on("/ac21", [](){
    acMidea.setTemp(21);
    acMidea.send();
    server.send(200, "text/html", "MideaTemperature");
  });

  server.on("/ac22", [](){
    acMidea.setTemp(22);
    acMidea.send();
    server.send(200, "text/html", "MideaTemperature");
  });

  server.on("/ac23", [](){
    acMidea.setTemp(23);
    acMidea.send();
    server.send(200, "text/html", "MideaTemperature");
  });

  server.on("/ac24", [](){
    acMidea.setTemp(24);
    acMidea.send();
    server.send(200, "text/html", "MideaTemperature");
  });

  server.on("/ac25", [](){
    acMidea.setTemp(25);
    acMidea.send();
    server.send(200, "text/html", "MideaTemperature");
  });

  server.on("/ac26", [](){
    acMidea.setTemp(26);
    acMidea.send();
    server.send(200, "text/html", "MideaTemperature");
  });

  server.on("/acFanAuto", [](){
    acMidea.setFan(kCoolixFanAuto);
    acMidea.send();
    server.send(200, "text/html", "MideaFan");
  });

  server.on("/acFanLow", [](){
    acMidea.setFan(kCoolixFanMin);
    acMidea.send();
    server.send(200, "text/html", "MideaFan");
  });

  server.on("/acFanMedium", [](){
    acMidea.setFan(kCoolixFanMed);
    acMidea.send();
    server.send(200, "text/html", "MideaFan");
  });

  server.on("/acFanHigh", [](){
    acMidea.setFan(kCoolixFanMax);
    acMidea.send();
    server.send(200, "text/html", "MideaFan");
  });

  server.on("/acSwing", [](){
    acMidea.setSwing();
    acMidea.send();
    server.send(200, "text/html", "MideaSwing");
  });
}

/**
*/
void setupWebServerForNet() {
  server.on("/netOnOff",  []() { netOnOff();  });
  server.on("/netPortal", []() { netPortal(); });

  server.on("/netMosaico", []() { netMosaico(); });
  server.on("/netInfo",    []() { netInfo();    });

  server.on("/netVolumeUp",   []() { netVolumeUp();   });
  server.on("/netVolumeDown", []() { netVolumeDown(); });


  server.on("/netChannelUp",   []() { netChannelUp();   });
  server.on("/netChannelDown", []() { netChannelDown(); });

  server.on("/netSair",  []() { netSair();  });
  server.on("/netNetTV", []() { netNetTV(); });

  server.on("/netUp",    []() { netUp();    });
  server.on("/netDown",  []() { netDown();  });
  server.on("/netLeft",  []() { netLeft();  });
  server.on("/netRight", []() { netRight(); });
  server.on("/netOK",    []() { netOK();    });
  server.on("/netOk",    []() { netOK();    });

  server.on("/netVermelho", []() { netVermelho(); });
  server.on("/netVerde",    []() { netVerde();    });
  server.on("/netBranco",   []() { netBranco();   });
  server.on("/netAmarelo",  []() { netAmarelo();  });
  server.on("/netAzul",     []() { netAzul();     });
  server.on("/netITV",      []() { netVermelho(); });
  server.on("/netAudio",    []() { netVerde();    });
  server.on("/netAgora",    []() { netBranco();   });
  server.on("/netLegenda",  []() { netAmarelo();  });
  server.on("/netMessage",  []() { netAzul();     });

  server.on("/net1", []() { net1(); });
  server.on("/net2", []() { net2(); });
  server.on("/net3", []() { net3(); });
  server.on("/net4", []() { net4(); });
  server.on("/net5", []() { net5(); });
  server.on("/net6", []() { net6(); });
  server.on("/net7", []() { net7(); });
  server.on("/net8", []() { net8(); });
  server.on("/net9", []() { net9(); });
  server.on("/net0", []() { net0(); });

  server.on("/netFav",  []() { netFav();  });
  server.on("/netMenu", []() { netMenu(); });

  server.on("/netRew",    []() { netRew();    });
  server.on("/netPlay",   []() { netPlay();   });
  server.on("/netFwd",    []() { netFwd();    });
  server.on("/netReplay", []() { netReplay(); });
  server.on("/netStop",   []() { netStop();   });
  server.on("/netRec",    []() { netRec();    });

  server.on("/netPPV",    []() { netPPV();    });
  server.on("/netNow",    []() { netNow();    });
  server.on("/netMusica", []() { netMusica(); });

  server.begin();
  println("HTTP Server Started");
}

void netPortal() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A24DB, 32);
  server.send(200, "text/html", "NetPortal");
}
void netOnOff() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A48B7, 32);
  server.send(200, "text/plain", "NetOnOff");
}

void netMosaico() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A847B, 32);
  server.send(200, "text/plain", "NetMosaico");
}
void netInfo() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AC837, 32);
  server.send(200, "text/plain", "NetInfo");
}

void netVolumeUp() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AB04F, 32);
  server.send(200, "text/plain", "NetVolumeUp");
}
void netVolumeDown() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A708F, 32);
  server.send(200, "text/plain", "NetVolumeDown");
}

void netChannelUp() {
#ifdef DEBUG  
  blink();
#endif
  print("NetChannelUp");
  irsend.sendNEC(0xE17A08F7, 32);
  server.send(200, "text/plain", "NetChannelUp");
}
void netChannelDown() {
#ifdef DEBUG  
  blink();
#endif
  print("NetChannelDown");
  irsend.sendNEC(0xE17A58A7, 32);
  server.send(200, "text/plain", "NetChannelDown");
}

void netSair() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A50AF, 32);
  server.send(200, "text/plain", "Net");
}
void netNetTV() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A28D7, 32);
  server.send(200, "text/plain", "Net");
}

void netUp() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AD02F, 32);
  server.send(200, "text/plain", "NetUp");
}
void netDown() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A30CF, 32);
  server.send(200, "text/plain", "NetDown");
}
void netLeft() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AD827, 32);
  server.send(200, "text/plain", "NetLeft");
}
void netRight() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A38C7, 32);
  server.send(200, "text/plain", "NetRight");
}
void netOK() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AA857, 32);
  server.send(200, "text/plain", "NetOK");
}

void netVermelho() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A6897, 32);
  server.send(200, "text/plain", "Net");
}
void netVerde() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AE817, 32);
  server.send(200, "text/plain", "Net");
}
void netBranco() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A7887, 32);
  server.send(200, "text/plain", "Net");
}
void netAmarelo() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A18E7, 32);
  server.send(200, "text/plain", "Net");
}
void netAzul() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A9867, 32);
  server.send(200, "text/plain", "Net");
}

void net1() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A807F, 32);
  server.send(200, "text/plain", "Net");
}
void net2() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A40BF, 32);
  server.send(200, "text/plain", "Net");
}
void net3() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AC03F, 32);
  server.send(200, "text/plain", "Net");
}
void net4() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A20DF, 32);
  server.send(200, "text/plain", "Net");
}
void net5() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AA05F, 32);
  server.send(200, "text/plain", "Net");
}
void net6() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A609F, 32);
  server.send(200, "text/plain", "Net");
}
void net7() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AE01F, 32);
  server.send(200, "text/plain", "Net");
}
void net8() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A10EF, 32);
  server.send(200, "text/plain", "Net");
}
void net9() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A906F, 32);
  server.send(200, "text/plain", "Net");
}
void net0() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A00FF, 32);
  server.send(200, "text/plain", "Net");
}

void netFav() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AB847, 32);
  server.send(200, "text/plain", "Net");
}
void netMenu() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AC43B, 32);
  server.send(200, "text/plain", "Net");
}

void netRew() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A2CD3, 32);
  server.send(200, "text/plain", "Net");
}
void netPlay() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A6C93, 32);
  server.send(200, "text/plain", "Net");
}
void netFwd() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AAC53, 32);
  server.send(200, "text/plain", "Net");
}
void netReplay() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17AEC13, 32);
  server.send(200, "text/plain", "Net");
}
void netStop() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A4CB3, 32);
  server.send(200, "text/plain", "Net");
}
void netRec() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17ACC33, 32);
  server.send(200, "text/plain", "NetRec");
}

void netPPV() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A14EB, 32);
  server.send(200, "text/plain", "NetPPV");
}
void netNow() {
  blink();
  irsend.sendNEC(0xE17A9C63, 32);
  server.send(200, "text/plain", "NetNow");
}
void netMusica() {
#ifdef DEBUG  
  blink();
#endif
  irsend.sendNEC(0xE17A04FB, 32);
  server.send(200, "text/plain", "NetMusica");
}

void blink() {
  digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on (Note that LOW is the voltage level
  // but actually the LED is on; this is because
  // it is active low on the ESP-01)
  delay(1000);                      // Wait for a second
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}