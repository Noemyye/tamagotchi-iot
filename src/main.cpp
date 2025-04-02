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
#define OLED_MOSI  23    // Data out (MOSI) pin
#define OLED_SCLK  18    // Clock (SCK) pin
#define OLED_CS    27    // Chip select pin
#define OLED_DC    26    // Data/Command pin
#define OLED_RST   14    // Reset pin - using GPIO14 instead of -1

// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0
#define WHITE    0xFFFF

// Create the OLED display object with software SPI
Adafruit_SSD1351 display = Adafruit_SSD1351(
    SCREEN_WIDTH, SCREEN_HEIGHT,
    OLED_CS,
    OLED_DC,
    OLED_MOSI,
    OLED_SCLK,
    OLED_RST
);

// **Configuration WiFi**
const char* ssid = "iPhonenono";    
const char* password = "nonolagrinta";

// **Configuration MQTT**
const char* mqtt_server = "broker.emqx.io";  
const int mqtt_port = 1883;  
const char* mqtt_topic = "tamagotchi/etat";  

struct Tamagotchi {
    int hunger;    // Niveau de faim (0-100)
    int happiness; // Niveau de bonheur (0-100)
    int cleanliness; // Propreté (0-100)
    
    void feed() {
        hunger = min(100, hunger + 10);  // Augmenter la faim, jusqu'à 100 (pas affamé)
    }

    void play() {
        happiness = min(100, happiness + 10);  

        if (random(3) == 0) {
            if (random(2) == 0) hunger = max(0, hunger - 5); // 1 chance sur 3 de perdre 5 de faim
            if (random(2) == 0) cleanliness = max(0, cleanliness - 5); // 1 chance sur 3 de perdre 5 de propreté
        }
    }

    void clean() {
        cleanliness = min(100, cleanliness + 10);  // Diminuer la propreté, jusqu'à 0 (sale)
    }
};

Tamagotchi myTamagotchi = {0, 100, 100};  // Par défaut, le Tamagotchi est très affamé, très heureux et très propre

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long previousMillis = 0;
const long interval = 120000; // 2 minutes (120000 ms)

void updateDisplay(String text) {
    static int lastHunger = -1;
    static int lastHappiness = -1;
    static int lastCleanliness = -1;
    
    // Only update if values have changed
    if (myTamagotchi.hunger == lastHunger && 
        myTamagotchi.happiness == lastHappiness && 
        myTamagotchi.cleanliness == lastCleanliness) {
        return;
    }
    
    // Save current values
    lastHunger = myTamagotchi.hunger;
    lastHappiness = myTamagotchi.happiness;
    lastCleanliness = myTamagotchi.cleanliness;
    
    // Clear previous text areas with black rectangles
    display.fillRect(14, 4, 25, 7, BLACK);    // Clear hunger area
    display.fillRect(58, 4, 25, 7, BLACK);    // Clear happiness area
    display.fillRect(101, 4, 25, 7, BLACK);    // Clear cleanliness area
    
    // Set text properties
    display.setTextSize(1);
    display.setTextWrap(false);
    
    // Draw Hunger (left)
    display.setTextColor(RED);
    display.setCursor(15, 4);
    display.print(myTamagotchi.hunger);
    display.print("%");
    
    // Draw Happiness (center)
    display.setTextColor(YELLOW);
    display.setCursor(59, 4);
    display.print(myTamagotchi.happiness);
    display.print("%");
    
    // Draw Cleanliness (right)
    display.setTextColor(CYAN);
    display.setCursor(102, 4);
    display.print(myTamagotchi.cleanliness);
    display.print("%");
}

void drawImage() {
    Serial.println("Starting image draw...");
    
    // Verify image data
    if (Image.data == nullptr) {
        Serial.println("Error: Image data is null!");
        return;
    }
    
    if (Image.width != SCREEN_WIDTH || Image.height != SCREEN_HEIGHT) {
        Serial.println("Error: Image dimensions don't match screen!");
        Serial.print("Image: "); Serial.print(Image.width); Serial.print("x"); Serial.println(Image.height);
        Serial.print("Screen: "); Serial.print(SCREEN_WIDTH); Serial.print("x"); Serial.println(SCREEN_HEIGHT);
        return;
    }
    
    // Clear screen before drawing
    display.fillScreen(BLACK);
    delay(50);  // Short delay to ensure screen is cleared
    
    // Draw the image
    Serial.println("Drawing image to display...");
    display.drawRGBBitmap(0, 0, Image.data, Image.width, Image.height);
    Serial.println("Image draw complete");
}

// **Connexion au WiFi**
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
        Serial.print("Adresse IP : ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nÉchec de connexion WiFi !");
        ESP.restart();
    }
}

// **Callback MQTT**
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("MQTT Message received on topic: ");
    Serial.println(topic);
    
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Message content: ");
    Serial.println(message);

    if (message == "nourrir") {
        Serial.println("Feeding command received");
        myTamagotchi.feed();
        updateDisplay("");  // Update display with new values
    } else if (message == "jouer") {
        Serial.println("Play command received");
        myTamagotchi.play();
        updateDisplay("");  // Update display with new values
    } else if (message == "nettoyer") {
        Serial.println("Clean command received");
        myTamagotchi.clean();
        updateDisplay("");  // Update display with new values
    } else {
        Serial.println("Unknown command received");
    }
}

// **Connexion MQTT avec délai**
void reconnect() {
    int attempts = 0;
    while (!client.connected() && attempts < 3) { // Limit reconnection attempts
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP32_" + String(random(1000, 9999));
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            client.subscribe(mqtt_topic);
            client.publish("esp32/status", "ESP32 connected!");
            Serial.println("Subscribed to topic: " + String(mqtt_topic));
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying in 5 seconds");
            attempts++;
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting setup...");

    // Configure all pins
    pinMode(OLED_DC, OUTPUT);
    pinMode(OLED_CS, OUTPUT);
    pinMode(OLED_RST, OUTPUT);
    pinMode(OLED_MOSI, OUTPUT);
    pinMode(OLED_SCLK, OUTPUT);

    // Reset display
    Serial.println("Resetting display...");
    digitalWrite(OLED_RST, HIGH);
    delay(100);
    digitalWrite(OLED_RST, LOW);
    delay(100);
    digitalWrite(OLED_RST, HIGH);
    delay(100);

    // Initialize display
    Serial.println("Initializing display...");
    display.begin();
    delay(100);
    
    // Basic display tests
    Serial.println("Running display tests...");
    display.fillScreen(BLACK);
    delay(500);
    
    
    // Draw welcome text
    Serial.println("Drawing welcome text...");
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(20, 50);
    display.println("TAMAGO");
    delay(2000);
    
    // Draw the image
    Serial.println("Drawing image...");
    drawImage();
    delay(1000);
    
    // Setup WiFi and MQTT
    Serial.println("Setting up WiFi...");
    setup_wifi();
    
    Serial.println("Setting up MQTT...");
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    
    Serial.println("Setup complete!");
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }

    // Update the display with current stats
    updateDisplay("");

    client.loop();
}
