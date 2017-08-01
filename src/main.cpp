#include "ACS712.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>

const char *ssid = "semard";
const char *password = "SEMARD123";
const char *mqtt_server = "192.168.0.200";
const int pinRelay = 5;
float I;
bool on;

WiFiClient espClient;
PubSubClient client(espClient);
String encodeData(float mAh);

// We have 30 amps version sensor connected to A0 pin of arduino
// Replace with your version if necessary
ACS712 sensor(ACS712_30A, A0);

void setup_wifi() {
  Serial.println();
  Serial.print("conectandose a ");
  Serial.println(ssid);
  WiFi.begin("SEMARD", "SEMARD123");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("conexion a wifi establecida");
  Serial.println("direccion IP:");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length) {
  char json[100];
  char message[100];
  String accion = "";

  StaticJsonBuffer<50> jsonBuffer;

  for (int i = 0; i < length; i++) {
    message[i] = (char)*payload;
    payload++;
  }
  JsonObject &root = jsonBuffer.parseObject(message);

  accion = root["accion"].as<String>();

  //{"accion":on}
  if ((accion == "on")) {
    Serial.println("WELCOME TO THE JUNGLE");
    on = true;
    sensor.calibrate();
    delay(10);
    digitalWrite(pinRelay, HIGH);
  }
  //{"accion":off}
  else if (accion == "off") {
    if (I < 1.0) {
      Serial.println("DNA");
      on = false;
      digitalWrite(pinRelay, LOW);
    }
  }

  Serial.println();
  Serial.print("mensaje recibido [");
  Serial.print(topic);
  Serial.println("] : ");
  Serial.println(accion);
  /*for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }*/
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("intentando conexion a mqtt...");
    String clientId = "impreESP8266";
    if (client.connect(clientId.c_str(), "semard", "semard2017")) {
      Serial.println("conectado");
      client.publish("impresora_info", "esp8266 conectado");
      client.subscribe("impresora_estado");
    } else {
      Serial.print("conexion fallida, rc= ");
      Serial.println(client.state());
      Serial.println("inteta de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);

  Serial.println("proceso inicializado");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW);
  on = false;

  sensor.calibrate();

  Serial.println("Calibrating... Ensure that no current flows through the "
                 "sensor at this moment");
  sensor.calibrate();
  Serial.println("Done!");
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  float U = 12;

  I = sensor.getCurrentAC();

  float W = U * I;

  Serial.print(String("I = ") + I + " Amp");
  Serial.println(String("\tP = ") + W + " Watts");
  encodeData(I);
  String JSON = encodeData(I);
  const char *payload = JSON.c_str();
  String clientId = "impreESP8266";
  if (client.connect(clientId.c_str(), "semard", "semard2017")) {
    // Serial.println(payload);
    client.publish("impresora_info", payload);
  }
  delay(200);
  client.loop();
}

String encodeData(float Ah) {

  //    Example JSON to be sent
  //   {
  //   "pT":"PRUSA I3",
  //   "n":"La bambina",
  //   "cC":"0.2",
  //   "v":"12",
  //   "s":["idle","printing","steppersOn","off"],
  //   "wT":{"hour":6,"minute":23,"second":50}
  //    }

  StaticJsonBuffer<250> jsonBufferRoot;
  StaticJsonBuffer<80> jsonBufferTime;
  JsonObject &time = jsonBufferTime.createObject();
  time["h"] = 6;
  time["m"] = 23;
  time["s"] = 8;
  JsonObject &root = jsonBufferRoot.createObject();
  root["pT"] = "PRUSA I3";
  root["n"] = "La Bambina";
  root["cC"] = Ah;
  root["v"] = 12;
  root["s"] = "idle";
  root["wT"] = time;

  String JSON;
  root.printTo(JSON);
  // root.prettyPrintTo(Serial);
  return JSON;
}
