//Select ESP32 Dev Module in board manager
//CP210x USB driver is required to flash this board https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
//To update driver go to device manager then find the usb driver then update by pointing to the folder you extracted
//Flash at 115200 Baud

#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// Configuration
const char* ssid = "Rm117"; // Enter SSID
const char* password = "Huskies1337$"; // Enter Password
const char* websockets_server_host = "10.0.0.211"; // Enter server address
const uint16_t websockets_server_port = 4444; // Enter server port
const char* hostname = "ESP32_1"; //set device name
const char* room = "room1"; //set room

using namespace websockets;

// GPIO pins to control
const int gpioPins[] = {2,4,5,12,14,15,18,19,21,22,23};  // Example array of GPIO pins
const int numPins = sizeof(gpioPins) / sizeof(gpioPins[0]);

WebsocketsClient client;

// Function prototypes
void setupWiFi();
void setupWebSocket();
void handleMessage(WebsocketsMessage message);
void reconnectWebSocket();
void turnOffAllPins();
void turnOnAllPins();
void sendStatusToServer(int pin, bool state);
String createJsonString(const String& type, const String& name, const String& room);
void onWebSocketEvent(WebsocketsEvent event, String data);


// Connection status
bool connected = false;

void setup() {
    Serial.begin(115200);

    // Initialize GPIO pins
    for (int i = 0; i < numPins; i++) {
        pinMode(gpioPins[i], OUTPUT);
    }

    // Perform setup tasks
    setupWiFi();
    setupWebSocket();

    // Run callback when messages are received
    client.onMessage(handleMessage);
    client.onEvent(onWebSocketEvent);
}

void loop() {
    if (!connected) {  // Check the connection status flag
        Serial.println("WebSocket not open, attempting to reconnect...");
        reconnectWebSocket();
    } else {
        client.poll();  // Process WebSocket events if connected
    }

    // Add your other code here, like checking sensors or handling other tasks
}

// Function to set up WiFi connection
void setupWiFi() {
    WiFi.begin(ssid, password);
    WiFi.setHostname(hostname);

    // Wait some time to connect to WiFi
    for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
        Serial.print(".");
        delay(1000);
    }

    // Check if connected to WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("No Wifi!");
        return;
    }

    Serial.println("Connected to Wifi");
}

// Function to set up WebSocket connection
void setupWebSocket() {
    Serial.println("Connecting to WebSocket server...");
    if (client.connect(websockets_server_host, websockets_server_port, "/")) {
        Serial.println("Connected to server!");
        connected = true; // Update connection status
        String jsonString = createJsonString("connect", hostname, room);
        client.send(jsonString);
    } else {
        Serial.println("Failed to connect to server.");
        connected = false; // Update connection status
    }
}

// Function to handle incoming WebSocket messages
void handleMessage(WebsocketsMessage message) {
    StaticJsonDocument<200> doc;  // Adjust size as needed
    DeserializationError error = deserializeJson(doc, message.data());
    if (error) {
        Serial.println("Failed to parse JSON.");
        return;
    }

    // Check for a command to turn off all pins
    const char* command = doc["command"];
    if (command && strcmp(command, "OFF_ALL") == 0) {
        turnOffAllPins();
    } else {
        // Extract the "pin" and "state" fields
        int pinNumber = doc["pin"];
        bool state = doc["state"];
        const char* name = doc["name"];
        const char* id = doc["id"];

        // Control the GPIO pin based on the state
        bool validPin = false;
        for (int i = 0; i < numPins; i++) {
            if (gpioPins[i] == pinNumber) {
                validPin = true;
                digitalWrite(pinNumber, state ? HIGH : LOW);
                sendStatusToServer(state, name, id);
                break;
            }
        }
        if (!validPin) {
            Serial.println("Invalid Pin Number");
        }
    }
}

// Function to reconnect to WebSocket server
void reconnectWebSocket() {
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();
    
    if (now - lastReconnectAttempt > 5000) { // Try to reconnect every 5 seconds
        lastReconnectAttempt = now;
        if (client.connect(websockets_server_host, websockets_server_port, "/")) {
            Serial.println("Reconnected to server!");
            connected = true; // Update connection status
            String jsonString = createJsonString("connect", hostname, room);
            client.send(jsonString);
        } else {
            Serial.println("Reconnect failed.");
            connected = false; // Update connection status
        }
    }
}

// Function to turn off all GPIO pins
void turnOffAllPins() {
    for (int i = 0; i < numPins; i++) {
        digitalWrite(gpioPins[i], LOW);  // Turn off each GPIO pin
    }
}

// Function to turn on all GPIO pins
void turnOnAllPins() {
    for (int i = 0; i < numPins; i++) {
        digitalWrite(gpioPins[i], HIGH);  // Turn on each GPIO pin
    }
}

// Function to send the status of a GPIO pin to the server
void sendStatusToServer(bool state, const char* name, const char* id) {
    const char* direction = "touser";
    const char* type = "toggle";
    // Create a JSON document
    StaticJsonDocument<200> doc;

    // Add data to the JSON object
    doc["type"] = type;
    doc["state"] = state ? "true" : "false";
    doc["name"] = name;
    doc["id"] = id;
    doc["direction"] = direction;

    // Serialize JSON to a string
    String jsonString;
    serializeJson(doc, jsonString);

    // Send JSON string to the server
    if (connected) {
        client.send(jsonString);
    }
}

// Function to create a JSON string
String createJsonString(const String& type, const String& name, const String& room) {
    return String("{") +
           "\"type\":\"" + type + "\"," +
           "\"name\":\"" + name + "\"," +
           "\"room\":\"" + room + "\"" +
           "}";
}

// Function to handle WebSocket events
void onWebSocketEvent(WebsocketsEvent event, String data) {
    switch (event) {
        case WebsocketsEvent::ConnectionOpened:
            Serial.println("WebSocket connection opened.");
            connected = true;
            break;
        case WebsocketsEvent::ConnectionClosed:
            Serial.println("WebSocket connection closed.");
            connected = false;
            reconnectWebSocket();
            break;
        default:
            Serial.println("WebSocket event: " + data);
            break;
    }
}