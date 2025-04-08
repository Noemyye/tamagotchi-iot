#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include "Image.h"
#include <SPI.h>

// Screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128

// Pin definitions for software SPI
#define OLED_MOSI  23
#define OLED_SCLK  18
#define OLED_CS    27
#define OLED_DC    26
#define OLED_RST   14
#define BUTTON_FEED_PIN 22 
#define BUTTON_PLAY_PIN 21
#define BUTTON_CLEAN_PIN 19

// Color definitions
#define BLACK    0x0000
#define RED      0xF800
#define YELLOW   0xFFE0
#define CYAN     0x07FF
#define WHITE    0xFFFF

Adafruit_SSD1351 display = Adafruit_SSD1351(
    SCREEN_WIDTH, SCREEN_HEIGHT,
    OLED_CS,
    OLED_DC,
    OLED_MOSI,
    OLED_SCLK,
    OLED_RST
);

// WiFi config
const char* ssid = "iPhonenono";
const char* password = "nonolagrinta";

// MQTT config
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_topic = "tamagotchi/etat";

struct Tamagotchi {
    int hunger;
    int happiness;
    int cleanliness;

    void feed() {
        hunger = min(100, hunger + 10);
    }

    void play() {
        happiness = min(100, happiness + 10);
        if (random(3) == 0) {
            if (random(2) == 0) hunger = max(0, hunger - 5);
            if (random(2) == 0) cleanliness = max(0, cleanliness - 5);
        }
    }

    void clean() {
        cleanliness = min(100, cleanliness + 10);
    }
};

Tamagotchi myTamagotchi = {0, 100, 100};

WiFiClient espClient;
PubSubClient client(espClient);

// Pour gestion du bouton
static bool lastButtonState = HIGH;

void updateDisplay(String text) {
    static int lastHunger = -1;
    static int lastHappiness = -1;
    static int lastCleanliness = -1;

    if (myTamagotchi.hunger == lastHunger && 
        myTamagotchi.happiness == lastHappiness && 
        myTamagotchi.cleanliness == lastCleanliness) {
        return;
    }

    lastHunger = myTamagotchi.hunger;
    lastHappiness = myTamagotchi.happiness;
    lastCleanliness = myTamagotchi.cleanliness;

    display.fillRect(14, 4, 25, 7, BLACK);
    display.fillRect(58, 4, 25, 7, BLACK);
    display.fillRect(101, 4, 25, 7, BLACK);

    display.setTextSize(1);
    display.setTextWrap(false);

    display.setTextColor(RED);
    display.setCursor(15, 4);
    display.print(myTamagotchi.hunger);
    display.print("%");

    display.setTextColor(YELLOW);
    display.setCursor(59, 4);
    display.print(myTamagotchi.happiness);
    display.print("%");

    display.setTextColor(CYAN);
    display.setCursor(102, 4);
    display.print(myTamagotchi.cleanliness);
    display.print("%");
}

void drawImage() {
    if (Image.data == nullptr) {
        Serial.println("Erreur image NULL");
        return;
    }

    if (Image.width != SCREEN_WIDTH || Image.height != SCREEN_HEIGHT) {
        Serial.println("Erreur dimensions image");
        return;
    }

    display.fillScreen(BLACK);
    delay(50);
    display.drawRGBBitmap(0, 0, Image.data, Image.width, Image.height);
}

void setup_wifi() {
    Serial.print("Connexion au WiFi...");
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connecté !");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nÉchec de connexion WiFi !");
        ESP.restart();
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    if (message == "nourrir") {
        myTamagotchi.feed();
    } else if (message == "jouer") {
        myTamagotchi.play();
    } else if (message == "nettoyer") {
        myTamagotchi.clean();
    }
    updateDisplay("");
}

void reconnect() {
    int attempts = 0;
    while (!client.connected() && attempts < 3) {
        String clientId = "ESP32_" + String(random(1000, 9999));
        if (client.connect(clientId.c_str())) {
            client.subscribe(mqtt_topic);
            client.publish("esp32/status", "ESP32 connected!");
        } else {
            delay(5000);
            attempts++;
        }
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(OLED_DC, OUTPUT);
    pinMode(OLED_CS, OUTPUT);
    pinMode(OLED_RST, OUTPUT);
    pinMode(OLED_MOSI, OUTPUT);
    pinMode(OLED_SCLK, OUTPUT);
    pinMode(BUTTON_FEED_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PLAY_PIN, INPUT_PULLUP);
    pinMode(BUTTON_CLEAN_PIN, INPUT_PULLUP);

    // Reset écran
    digitalWrite(OLED_RST, HIGH); delay(100);
    digitalWrite(OLED_RST, LOW); delay(100);
    digitalWrite(OLED_RST, HIGH); delay(100);

    display.begin();
    display.fillScreen(BLACK);
    delay(500);

    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(20, 50);
    display.println("TAMAGO");
    delay(2000);

    drawImage();
    delay(1000);

    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }

    client.loop();
    updateDisplay("");

    // Gestion des 3 boutons physiques
    static bool lastFeedButton = HIGH;
    static bool lastPlayButton = HIGH;
    static bool lastCleanButton = HIGH;

    bool currentFeedButton = digitalRead(BUTTON_FEED_PIN);
    bool currentPlayButton = digitalRead(BUTTON_PLAY_PIN);
    bool currentCleanButton = digitalRead(BUTTON_CLEAN_PIN);

    // Bouton 1
    if (lastFeedButton == HIGH && currentFeedButton == LOW) {
        Serial.println("🔵 Bouton 1 appuyé : Nourrir");
        myTamagotchi.feed();
        updateDisplay("");
    }
    lastFeedButton = currentFeedButton;

    // Bouton 2
    if (lastPlayButton == HIGH && currentPlayButton == LOW) {
        Serial.println("🟡 Bouton 2 appuyé : Jouer");
        myTamagotchi.play();
        updateDisplay("");
    }
    lastPlayButton = currentPlayButton;

    // Bouton 3
    if (lastCleanButton == HIGH && currentCleanButton == LOW) {
        Serial.println("🟢 Bouton 3 appuyé : Nettoyer");
        myTamagotchi.clean();
        updateDisplay("");
    }
    lastCleanButton = currentCleanButton;
}
