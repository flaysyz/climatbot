import paho.mqtt.client as mqtt

MQTT_BROKER = "192.168.1.98"
MQTT_PORT = 1883
MQTT_TOPIC = "test/#"


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Успешное подключение к MQTT брокеру")
        client.subscribe(MQTT_TOPIC)  # Подписываемся на нужный топик
        print(f"Подписка на топик: {MQTT_TOPIC}")
    else:
        print(f"Ошибка подключения. Код: {rc}")


def on_message(client, userdata, msg):
    try:
        topic = msg.topic
        payload = msg.payload.decode("utf-8")
        print(f"Топик: {topic}, Данные: {payload}")
    except Exception as e:
        print(f"Ошибка обработки сообщения: {e}")


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message


try:
    print(f"Подключение к MQTT брокеру {MQTT_BROKER}:{MQTT_PORT}")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
except Exception as e:
    print(f"[ERROR] Не удалось подключиться к брокеру: {e}")
    exit(1)


try:
    client.loop_forever()
except KeyboardInterrupt:
    print("Отключение от MQTT брокера.")
    client.disconnect()
