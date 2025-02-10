#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SSD1306Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DHT.h"
#include "fontsRus.h"
#include "MAX30105.h"

MAX30105 particleSensor;

const int BUFFER_SIZE = 10;
int bpmBuffer[BUFFER_SIZE] = {0};
int bufferIndex = 0;
int validMeasurements = 0;
unsigned long lastBeatTime = 0;
int bpm;
float spo2;

// Настройки DHT22
#define DHTPIN 2           // Пин подключения DHT22
#define DHTTYPE DHT22       // Тип датчика
DHT dht(DHTPIN, DHTTYPE);

// Настройки Wi-Fi
const char* ssid = "flaysyz";        // Имя Wi-Fi сети
const char* password = "12345678";    // Пароль Wi-Fi



// Настройки MQTT
const char* mqtt_server = "84.201.174.42"; // Адрес MQTT брокера

const char* mqtt_user = "gera";  // Имя пользователя MQTT  
const char* mqtt_password = "gera123";  // Пароль MQTT  

const int mqtt_port = 1883;   // Порт MQTT
const char* mqtt_topic_temp = "test/temp"; // Топик для температуры
const char* mqtt_topic_hum = "test/hum";
const char* mqtt_topic_bpm = "test/bpm";
const char* mqtt_topic_spo2 = "test/spo2";

WiFiClient espClient;
PubSubClient client(espClient);

// Настройки NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 10800, 60000); // Часовой пояс +1 час (3600)

// Настройки дисплея
SSD1306Wire display(0x3c, SDA, SCL); // Адрес дисплея, SDA=D2, SCL=D1

unsigned long lastMsg = 0; // Таймер для периодической отправки данных
const long interval = 1000; // Интервал отправки данных (мс)


void setup() {
  Serial.begin(115200);
  dht.begin();
  
  // Подключение к Wi-Fi
  Serial.print("Подключение к Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi подключен");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());

  // Настройка MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  // Настройка NTP
  timeClient.begin();
  // Настройка дисплея

  display.init();
  display.flipScreenVertically();
  display.setFontTableLookupFunction(FontUtf8Rus);
  display.setFont(ArialRus_Plain_10);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
        Serial.println("MAX30102 не найден. Проверьте подключение.");
    }
  particleSensor.setup();


}

void reconnect() {
  // Подключение к MQTT серверу
  while (!client.connected()) {
    Serial.print("Подключение к MQTT серверу...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("Успешное подключение!");
    } else {
      Serial.print("Ошибка подключения. Код: ");
      Serial.print(client.state());
      Serial.println(". Повтор через 5 секунд...");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Получение текущего времени
  timeClient.update();
  String currentTime = timeClient.getFormattedTime();

  unsigned long now = millis();



  if (now - lastMsg > interval) {
    lastMsg = now;
    long irValue = particleSensor.getIR();
    long redValue = particleSensor.getRed();
    Serial.print("IR: "); Serial.println(irValue);
    Serial.print("RED: "); Serial.println(redValue);
    // Чтение данных с DHT22
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    if (irValue < 5000) {  
      Serial.println("Нет контакта с пальцем!");
      bpm = 0;
      spo2 = 0.0;
    } else {
      bpm = calculateBPM(irValue);
      spo2 = calculateSpO2(irValue, redValue);
    }
    
    String timeStr = timeClient.getFormattedTime();
    if (isnan(temp) || isnan(hum)) {
      Serial.println("Ошибка чтения с DHT22");
      return;
    }

    // Отправка данных на MQTT
    char tempStr[8];
    dtostrf(temp, 6, 2, tempStr);
    client.publish(mqtt_topic_temp, tempStr);

    char humStr[8];
    dtostrf(hum, 6, 2, humStr);
    client.publish(mqtt_topic_hum, humStr);

    char bpmStr[8];
    dtostrf(bpm, 6, 2, bpmStr);  
    client.publish(mqtt_topic_bpm, bpmStr);

    char spo2Str[8];
    dtostrf(spo2, 6, 2, spo2Str);  
    client.publish(mqtt_topic_spo2, spo2Str);


    // Вывод данных в Serial Monitor
    Serial.print("Время: ");
    Serial.println(currentTime);
    Serial.print("Температура: ");
    Serial.print(temp);
    Serial.println("°C");
    Serial.print("Влажность: ");
    Serial.print(hum);
    Serial.println("%");
    Serial.print(bpm);
    Serial.println(" BPM");
    Serial.print(spo2);
    Serial.println(" SP02");

    // Вывод данных на дисплей
    display.clear();
    display.drawString(0, 0, "Время: " + currentTime);
    display.drawString(0, 16, "Температура: " + String(temp) + " C");
    display.drawString(0, 32, "Влажность: " + String(hum) + " %");
    display.drawString(0, 48, "BPM: " + String(bpm) + " SpO₂: " + String(spo2) + "%");
    display.display();
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Сообщение получено. Топик: ");
  Serial.println(topic);
}

char FontUtf8Rus(const byte ch) { 
    static uint8_t LASTCHAR;

    if ((LASTCHAR == 0) && (ch < 0xC0)) {
      return ch;
    }

    if (LASTCHAR == 0) {
        LASTCHAR = ch;
        return 0;
    }

    uint8_t last = LASTCHAR;
    LASTCHAR = 0;
    
    switch (last) {
        case 0xD0:
            if (ch == 0x81) return 0xA8;
            if (ch >= 0x90 && ch <= 0xBF) return ch + 0x30;
            break;
        case 0xD1:
            if (ch == 0x91) return 0xB8;
            if (ch >= 0x80 && ch <= 0x8F) return ch + 0x70;
            break;
    }

    return (uint8_t) 0;
}

int calculateBPM(long irValue) {
    unsigned long currentTime = millis();
    
    if (irValue > 60000 && (currentTime - lastBeatTime) > 600) {  
        int bpm = 60000 / (currentTime - lastBeatTime);
        lastBeatTime = currentTime;

        // Добавляем измерение в буфер
        bpmBuffer[bufferIndex] = bpm;
        bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
        
        if (validMeasurements < BUFFER_SIZE) {
            validMeasurements++;
        }

        // Рассчитываем среднее значение
        int sum = 0;
        for (int i = 0; i < validMeasurements; i++) {
            sum += bpmBuffer[i];
        }
        return sum / validMeasurements;
    }

    return (validMeasurements > 0) ? bpmBuffer[(bufferIndex - 1 + BUFFER_SIZE) % BUFFER_SIZE] : 0;
}

float calculateSpO2(long ir, long red) {
    if (red == 0 || ir == 0) return 0;  // Защита от деления на 0

    float ratio = (float)red / ir;
    float spo2 = 104 - (17 * ratio);  // Улучшенная формула, ближе к реальным данным

    if (spo2 > 100) spo2 = 100;
    if (spo2 < 80) spo2 = 80;

    return spo2;
}