#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>  // Remplace Adafruit_SSD1306 par Adafruit_SSD1351
#include "image_bitmap.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128  // Taille de l'écran SSD1351
#define OLED_RESET    -1  
Adafruit_SSD1351 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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
        happiness = min(100, hunger + 10);  // Diminuer le bonheur, jusqu'à 0 (triste)

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

// Déclaration de la fonction updateDisplay
void updateDisplay(String text) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1351_WHITE);  // Utilise SSD1351_WHITE
    display.setCursor(0, 0);
    display.println(text);
    display.display();
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
    setup_wifi();  
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    
    if (!display.begin(SSD1351_COLOR, SCREEN_WIDTH, SCREEN_HEIGHT, OLED_RESET)) {  // Initialisation SSD1351
        Serial.println(F("Échec de l'initialisation de l'OLED"));
        for (;;);
    }

    // Test d'affichage
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1351_WHITE);  // Utilise SSD1351_WHITE
    display.setCursor(0, 0);
    display.println("Tamago test");
    display.display();
    delay(2000);  // Attente de 2 secondes

    display.clearDisplay();
    display.drawBitmap(0, 0, image_data, 128, 128, 1);  // Affichage de l'image sur SSD1351
    display.display();
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }

    // Mettre à jour l'affichage du Tamagotchi
    String status = "Faim: " + String(myTamagotchi.hunger) + "\n";
    status += "Bonheur: " + String(myTamagotchi.happiness) + "\n";
    status += "Propreté: " + String(myTamagotchi.cleanliness);
    updateDisplay(status);  // Mise à jour de l'affichage avec les nouvelles valeurs

    client.loop();
}
