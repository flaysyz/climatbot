#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SSD1306Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DHT.h"
#include "fontsRus.h"


// Настройки DHT22
#define DHTPIN D5           // Пин подключения DHT22
#define DHTTYPE DHT22       // Тип датчика
DHT dht(DHTPIN, DHTTYPE);

// Настройки Wi-Fi
const char* ssid = "GERSTABILO";        // Имя Wi-Fi сети
const char* password = "Gera20Lyuba07Oleg13";    // Пароль Wi-Fi

// Настройки MQTT
const char* mqtt_server = "192.168.1.98"; // Адрес MQTT брокера
const int mqtt_port = 1883;   // Порт MQTT
const char* mqtt_topic_temp = "test/temp"; // Топик для температуры
const char* mqtt_topic_hum = "test/hum";   // Топик для влажности

WiFiClient espClient;
PubSubClient client(espClient);

// Настройки NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 10800, 60000); // Часовой пояс +1 час (3600)

// Настройки дисплея
SSD1306Wire display(0x3C, 4, 5); // Адрес дисплея, SDA=D2, SCL=D1

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
}

void reconnect() {
  // Подключение к MQTT серверу
  while (!client.connected()) {
    Serial.print("Подключение к MQTT серверу...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
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

    // Чтение данных с DHT22
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

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

    // Вывод данных в Serial Monitor
    Serial.print("Время: ");
    Serial.println(currentTime);
    Serial.print("Температура: ");
    Serial.print(temp);
    Serial.println("°C");
    Serial.print("Влажность: ");
    Serial.print(hum);
    Serial.println("%");

    // Вывод данных на дисплей
    display.clear();
    display.drawString(0, 0, "Время: " + currentTime);
    display.drawString(0, 16, "Температура: " + String(temp) + " C");
    display.drawString(0, 32, "Влажность: " + String(hum) + " %");
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
