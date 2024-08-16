#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <BH1750.h>

// Configurações da rede Wi-Fi
const char* ssid = "YOUR LOCAL WIFI";
const char* password = "WIFI PASSWORD";

// Configurações do MQTT
const char* mqtt_server = "BROKER URL";
const int mqtt_port = 8883;
const char* mqtt_user = "USERNAME";
const char* mqtt_pass = "USER PASSWORD";

// Configurações dos sensores
#define DHTPIN 4          // Pino onde o sensor DHT está conectado
#define DHTTYPE DHT11     // Tipo de sensor DHT
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

// Configurações do NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0); // UTC Offset para o seu fuso horário

// Inicializa o cliente WiFi e MQTT
WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup_wifi();
void reconnect();

void setup() {
  Serial.begin(115200);
  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  timeClient.begin();
  espClient.setInsecure();

  dht.begin();
  Wire.begin();
  delay(1000);
  lightMeter.begin();
  delay(1000);
  timeClient.update();
  // Verifica a conexão MQTT
  reconnect();
}

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop até reconectar
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Leitura dos sensores
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  uint16_t lightIntensity = lightMeter.readLightLevel();

  // Publicar dados no MQTT
  String payload = String("temperature=") + temperature +
                   "&humidity=" + humidity +
                   "&light=" + lightIntensity;
  
  if (client.publish("station/1/solarPanel/measurement", payload.c_str())) {
    Serial.println("Data published successfully");
  } else {
    Serial.println("Data publish failed");
  }

  delay(30000); // Publica a cada 30 segundos
}
