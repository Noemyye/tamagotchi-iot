#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include "image_bitmap.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define OLED_RESET -1
#define OLED_DC    26    // Data/Command pin
#define OLED_CS    27    // Chip select pin
#define OLED_MOSI  23    // SPI MOSI pin
#define OLED_SCLK  18    // SPI Clock pin

// SPI Settings
#define SPI_SPEED 8000000  // 8MHz

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

Adafruit_SSD1351 display = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_CS, OLED_DC, OLED_RESET);


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
        happiness = min(100, hunger + 10);  

        if (random(3) == 0) {
            if (random(2) == 0) hunger = max(0, hunger - 5); // 1 chance sur 3 de perdre 5 de faim
            if (random(2) == 0) cleanliness = max(0, cleanliness - 5); // 1 chance sur 3 de perdre 5 de propreté
        }
    }

    void clean() {
        cleanliness = min(100, hunger + 10);  // Diminuer la propreté, jusqu'à 0 (sale)
    }
};

Tamagotchi myTamagotchi = {0, 100, 100};  // Par défaut, le Tamagotchi est très affamé, très heureux et très propre

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long previousMillis = 0;
const long interval = 120000; // 2 minutes (120000 ms)

void updateDisplay(String text) {
    static unsigned long lastUpdate = 0;
    unsigned long currentMillis = millis();
    
    // Slower refresh rate - update every 2 seconds
    if (currentMillis - lastUpdate < 2000) {
        return;
    }
    lastUpdate = currentMillis;
    
    // Simple black background
    display.fillScreen(BLACK);
    
    // Simple white text, larger size for better readability
    display.setTextSize(1.8);
    display.setTextColor(WHITE);
    
    // Center the text vertically
    int y = 20;
    int lineHeight = 25;  // More space between lines
    
    // Split and display text
    String line;
    int pos = 0;
    while ((pos = text.indexOf('\n')) != -1) {
        line = text.substring(0, pos);
        display.setCursor(5, y);
        display.println(line);
        text = text.substring(pos + 1);
        y += lineHeight;
    }
    
    // Last line
    display.setCursor(5, y);
    display.println(text);
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
    Serial.print("Message reçu : ");
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);

    if (message == "nourrir") {
        myTamagotchi.feed();
        updateDisplay("Faim: " + String(myTamagotchi.hunger));
        Serial.println("Faim: " + String(myTamagotchi.hunger));
    } else if (message == "jouer") {
        myTamagotchi.play();
        updateDisplay("Bonheur: " + String(myTamagotchi.happiness));
        Serial.println("Bonheur: " + String(myTamagotchi.happiness));
    } else if (message == "nettoyer") {
        myTamagotchi.clean();
        updateDisplay("Propreté: " + String(myTamagotchi.cleanliness));
        Serial.println("Propreté: " + String(myTamagotchi.cleanliness));
    }
}

// **Connexion MQTT avec délai**
void reconnect() {
    while (!client.connected()) {
        Serial.print("Connexion MQTT...");
        String clientId = "ESP32_" + String(random(1000, 9999));
        if (client.connect(clientId.c_str())) {
            Serial.println(" Connecté !");
            client.subscribe(mqtt_topic);
            client.publish("esp32/status", "ESP32 connecté !");
        } else {
            Serial.print("Échec, code erreur: ");
            Serial.println(client.state());
            Serial.println("Nouvelle tentative dans 5 secondes...");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // Initialize SPI with slower speed for stability
    SPI.begin(OLED_SCLK, -1, OLED_MOSI, OLED_CS);
    SPI.setFrequency(4000000);  // Reduced to 4MHz
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    
    // Initialize display
    display.begin();
    Serial.println(F("SSD1351 initialization completed"));
    
    // Simple welcome message
    display.fillScreen(BLACK);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(20, 50);
    display.println("TAMAGO");
    delay(3000);  // Longer delay for stability

    // Setup WiFi after display is initialized
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    // Clear screen
    display.fillScreen(BLACK);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }

    // Mettre à jour l'affichage du Tamagotchi
    String status = "Faim: " + String(myTamagotchi.hunger) + "\n";
    status += "Bonheur: " + String(myTamagotchi.happiness) + "\n";
    status += "Proprete: " + String(myTamagotchi.cleanliness);
    updateDisplay(status);  // Mise à jour de l'affichage avec les nouvelles valeurs

    client.loop();
}
