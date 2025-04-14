#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include "Image.h"
#include <SPI.h>
#include <algorithm>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define OLED_MOSI  23
#define OLED_SCLK  18
#define OLED_CS    27
#define OLED_DC    26
#define OLED_RST   14
#define BUTTON_FEED_PIN 22 
#define BUTTON_PLAY_PIN 21
#define BUTTON_CLEAN_PIN 19
#define BUTTON_SELECT_PIN 17
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

const char* ssid = "iPhonenono";
const char* password = "nonolagrinta";
const char* serverUrl = "http://172.20.10.2:8080";
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_topic = "tamagotchi/etat";

WiFiClient espClient;
PubSubClient client(espClient);

struct Tamagotchi {
    int hunger;
    int happiness;
    int cleanliness;
    bool isAlive;
    void feed() {
        Serial.print("pute");
        if (isAlive) {
            hunger = min(100, hunger + 10);
            
        }
    }

    void play() {
        if (isAlive) {
            happiness = min(100, happiness + 10);
            if (random(3) == 0) {
                if (random(2) == 0) hunger = max(0, hunger - 5);
                if (random(2) == 0) cleanliness = max(0, cleanliness - 5);
            }
            
        }
    }

    void clean() {
        if (isAlive) {
            cleanliness = min(100, cleanliness + 10);
          
        }
    }

    void decreaseStats() {
        if (!isAlive) return;
        
        int randomDecrease = random(1, 4); 

        if (randomDecrease == 1) hunger = std::max<long int>((long int)0, (long int)(hunger - random(5, 16)));
        if (randomDecrease == 2) happiness = std::max<long int>((long int)0, (long int)(happiness - random(5, 16)));
        if (randomDecrease == 3) cleanliness = std::max<long int>((long int)0, (long int)(cleanliness - random(5, 16)));
        
    }
};

struct TamaInfo {
  const char* id;
  const char* name;
  const char* character;
};

TamaInfo tamagotchiList[] = {
  {"tama1", "Lulu", "dragon"},
  {"tama2", "Pika", "pikachu"},
  {"tama3", "Mimi", "chat"}
};
const int tamagotchiCount = sizeof(tamagotchiList) / sizeof(TamaInfo);
int selectedIndex = 0;
bool selected = false;
const char* selectedId = tamagotchiList[0].id;
const char* selectedName = tamagotchiList[0].name;
const char* selectedCharacter = tamagotchiList[0].character;

Tamagotchi myTamagotchi = {0, 100, 100, true};

// état du jeu
bool gameOver = false;
unsigned long lastActionTime = 0;
const unsigned long GAME_OVER_TIMEOUT = 300000; 

void setup_wifi();
void sendData();
void displaySelectionScreen(int index);
void updateDisplay();
void drawImage();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void checkGameOver();

void sendData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Attempting to reconnect...");
    setup_wifi();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Failed to reconnect to WiFi. Cannot send data.");
      return;
    }
  }

  HTTPClient http;
  int maxRetries = 3;
  int retryCount = 0;
  int httpResponseCode = -1;
  while (retryCount < maxRetries && httpResponseCode < 0) {
    Serial.print("Attempting to connect to server (attempt ");
    Serial.print(retryCount + 1);
    Serial.print("/");
    Serial.print(maxRetries);
    Serial.println(")");
    
    http.begin(String(serverUrl) + "/api/tamagotchi/update");
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000); // timeout de 10 secondes

    StaticJsonDocument<200> doc;
    doc["id"] = selectedId;
    doc["name"] = selectedName;
    doc["character"] = selectedCharacter;
    doc["hunger"] = myTamagotchi.hunger;
    doc["fun"] = myTamagotchi.happiness;
    doc["cleanliness"] = myTamagotchi.cleanliness;
    doc["alive"] = myTamagotchi.isAlive;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    Serial.print("Sending data: ");
    Serial.println(jsonString);
    httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode > 0) {
      Serial.printf("HTTP Response code: %d\n", httpResponseCode);
      break;
    } else {
      Serial.printf("Error code: %d\n", httpResponseCode);
      Serial.println(http.errorToString(httpResponseCode));
      retryCount++;
      delay(2000); 
    }
    http.end();
  }

  if (httpResponseCode < 0) {
    Serial.println("Failed to send data after multiple attempts");
  }
}

void fetchData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    return;
  }
  HTTPClient http;
  http.begin(String(serverUrl) + "/api/tamagotchi/" + selectedId);  // adapte selon ton endpoint réel
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println("Réponse du serveur :");
    Serial.println(payload);
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      myTamagotchi.hunger = doc["hunger"];
      myTamagotchi.happiness = doc["happiness"];
      myTamagotchi.cleanliness = doc["cleanliness"];
      myTamagotchi.isAlive = doc["alive"];
    } else {
      Serial.print("Erreur de parsing JSON : ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("Erreur GET : ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

static bool lastButtonState = HIGH;

unsigned long lastTime = 0;
const long interval = 30000; 

void displaySelectionScreen(int index) {
  display.fillScreen(BLACK);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 5);
  display.println("Choisis ton Tama:");

  for (int i = 0; i < tamagotchiCount; i++) {
    if (i == index) {
      display.setTextColor(RED);
      display.fillRect(5, 18 + i * 12, 118, 12, BLACK); 
    } else {
      display.setTextColor(WHITE);
      display.fillRect(5, 18 + i * 12, 118, 12, BLACK);
    }
    display.setCursor(10, 20 + i * 12);
    display.print(i + 1);
    display.print(". ");
    display.println(tamagotchiList[i].name);
  }
  
  display.setTextColor(WHITE);
  display.setCursor(10, 100);
  display.print("Up/Down: Select");
  display.setCursor(10, 110);
  display.print("Clean: Choose");
}

void updateDisplay() {
    if (!myTamagotchi.isAlive) {
        display.fillScreen(BLACK); 
        display.setTextSize(2);  
        display.setTextColor(RED);  
        display.setCursor(20, 30); 
        display.println("GAME OVER");    
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(10, 60);
        display.print(selectedName);
        display.println(" has died!");     
        display.setCursor(10, 80);
        display.println("Press SELECT to");
        display.println("choose another");
        display.println("Tamagotchi");
        
        return; 
    }

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
        Serial.print("Adresse IP : ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nÉchec de connexion WiFi !");
        ESP.restart();
    }
}

void reconnect() {
    int attempts = 0;
    while (!client.connected() && attempts < 3) { // Limite de reconnexions
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP32_" + String(random(1000, 9999));
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            client.subscribe(mqtt_topic);
            client.publish("tamagotchi/etat", "ESP32 connected!");
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
        sendData();
    } else if (message == "jouer") {
        Serial.println("Play command received");
        myTamagotchi.play();
        sendData();
    } else if (message == "nettoyer") {
        Serial.println("Clean command received");
        myTamagotchi.clean();
        sendData();
    } else {
        Serial.println("Unknown command received");
    }
  
    fetchData();
    updateDisplay();
}

void checkGameOver() {
    if (myTamagotchi.hunger == 0 && myTamagotchi.happiness == 0 && myTamagotchi.cleanliness == 0) {
        myTamagotchi.isAlive = false;
        gameOver = true;
        sendData(); // Informe le serveur de la mort du Tamagotchi
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
    pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
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

    while (!selected) {
        displaySelectionScreen(selectedIndex);

        bool upPressed = digitalRead(BUTTON_FEED_PIN) == LOW;
        bool downPressed = digitalRead(BUTTON_PLAY_PIN) == LOW;
        bool selectPressed = digitalRead(BUTTON_CLEAN_PIN) == LOW;
        if (upPressed) {
            selectedIndex = (selectedIndex - 1 + tamagotchiCount) % tamagotchiCount;
            delay(300); 
        } else if (downPressed) {
            selectedIndex = (selectedIndex + 1) % tamagotchiCount;
            delay(300);
        } else if (selectPressed) {
            selected = true;
            selectedId = tamagotchiList[selectedIndex].id;
            selectedName = tamagotchiList[selectedIndex].name;
            selectedCharacter = tamagotchiList[selectedIndex].character;
            lastActionTime = millis();
            delay(500);
            updateDisplay();
        }
        
        delay(50);
    }
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    drawImage();
    updateDisplay();
    delay(500);
    // envoyer les données initiales au serveur
    sendData();
}

void loop() {
    if (!client.connected()) reconnect();
    client.loop();

    unsigned long now = millis();
    if (now - lastTime > interval) {
        lastTime = now;
        myTamagotchi.decreaseStats();
        updateDisplay();
        sendData();
        checkGameOver();
    }

    if (digitalRead(BUTTON_FEED_PIN) == LOW) {
        myTamagotchi.feed();
        updateDisplay();
        sendData();
        delay(300);
    }

    if (digitalRead(BUTTON_PLAY_PIN) == LOW) {
        myTamagotchi.play();
        updateDisplay();
        sendData();
        delay(300);
    }

    if (digitalRead(BUTTON_CLEAN_PIN) == LOW) {
        myTamagotchi.clean();
        updateDisplay();
        sendData();
        delay(300);
    }

    if (!myTamagotchi.isAlive && digitalRead(BUTTON_SELECT_PIN) == LOW) {
        selected = false;
        selectedIndex = 0;
        myTamagotchi = {0, 100, 100, true};
        gameOver = false;
        handleTamagotchiSelection();
        drawImage();
        updateDisplay();
    }
}