import telebot
import paho.mqtt.client as mqtt

TELEGRAM_TOKEN = "7862856391:AAHyVMhgccEw4H45WSrtUF8HlQodFSqB45s"
bot = telebot.TeleBot(TELEGRAM_TOKEN)


MQTT_BROKER = "84.201.174.42"
MQTT_PORT = 1883
MQTT_USER = "gera"
MQTT_PASSWORD = "gera123"


MQTT_TOPIC_TEMP = "test/temp"
MQTT_TOPIC_HUM = "test/hum"
MQTT_TOPIC_BPM = "test/bpm"
MQTT_TOPIC_SPO2 = "test/spo2"


temperature = "Нет данных"
humidity = "Нет данных"
bpm = "Нет данных"
spo2 = "Нет данных"


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Успешное подключение к MQTT брокеру")
        client.subscribe([(MQTT_TOPIC_TEMP, 0), (MQTT_TOPIC_HUM, 0), (MQTT_TOPIC_BPM, 0), (MQTT_TOPIC_SPO2, 0)])
    elif rc == 5:
        print("Ошибка: Неверные учетные данные (код 5)")
        exit(1)
    else:
        print(f"Ошибка подключения к MQTT брокеру. Код: {rc}")


def on_message(client, userdata, msg):
    global temperature, humidity, bpm, spo2
    try:
        if msg.topic == MQTT_TOPIC_TEMP:
            temperature = f"{msg.payload.decode('utf-8')} °C"
        elif msg.topic == MQTT_TOPIC_HUM:
            humidity = f"{msg.payload.decode('utf-8')} %"
        elif msg.topic == MQTT_TOPIC_BPM:
            bpm = f"{msg.payload.decode('utf-8')} BPM"
        elif msg.topic == MQTT_TOPIC_SPO2:
            spo2 = f"{msg.payload.decode('utf-8')} %"
    except Exception as e:
        print(f"Ошибка обработки MQTT сообщения: {e}")


mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message


try:
    print(f"🔌 Подключение к MQTT брокеру {MQTT_BROKER}:{MQTT_PORT}...")
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
    mqtt_client.loop_start()
except Exception as e:
    print(f"Не удалось подключиться к брокеру: {e}")
    exit(1)


@bot.message_handler(commands=['start'])
def start(message):
    markup = telebot.types.ReplyKeyboardMarkup(resize_keyboard=True)
    markup.add(telebot.types.KeyboardButton("Показатели микроклимата"))
    bot.send_message(
        message.chat.id,
        f"Здравствуйте, {message.from_user.first_name}!\n"
        "Я бот для мониторинга микроклимата. Нажмите на кнопку ниже, чтобы посмотреть текущие показатели.",
        reply_markup=markup
    )


@bot.message_handler(func=lambda message: message.text == "Показатели микроклимата")
def send_climate_data(message):
    global temperature, humidity, bpm, spo2
    bot.send_message(
        message.chat.id,
        f"Показатели микроклимата:\n"
        f"Температура: {temperature}\n"
        f"Влажность: {humidity}\n"
        f"Сердцебиение: {bpm}\n"
        f"Насыщенность кислородом: {spo2}"
    )


@bot.message_handler(func=lambda message: True)
def unknown_command(message):
    bot.send_message(message.chat.id, "Извините, я не понимаю эту команду. Нажмите /start для начала работы.")


bot.infinity_polling()
