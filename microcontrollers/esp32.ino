#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// Replace with your network credentials
//const char* ssid = "your_SSID";
//const char* password = "your_PASSWORD";
const char* ssid = "Rm117";
const char* password = "Huskies1337$";
const char* devicename = "ESP1";

// Set the hostname
const char* hostname = "ESP32SocketClient";

// WebSocket server details
const char* websocket_server = "ws://10.0.0.170"; //change to server address
const int websocket_port = 8080; // Change if your WebSocket server uses a different port

// GPIO pins to control
const int gpioPins[] = {2,4,5,12,13,14,15,18,19,21,22,23,25,26,27,32,33,34,35,36,39};  // Example array of GPIO pins
const int numPins = sizeof(gpioPins) / sizeof(gpioPins[0]);

// Create a WebSocket client
WebSocketsClient webSocket;

// Function prototypes
void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void reconnectWebSocket();

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Set the GPIO pins as outputs
  for (int i = 0; i < numPins; i++) {
    pinMode(gpioPins[i], OUTPUT);
  }

    // Set the hostname
  WiFi.setHostname(hostname);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");

  // Set up WebSocket event handler
  webSocket.onEvent(onWebSocketEvent);

  // Connect to WebSocket server
  webSocket.begin(websocket_server, websocket_port, "/");
}

void loop() {
  // Handle WebSocket events
  webSocket.loop();

  // Optionally, handle reconnection
  if (!webSocket.isConnected()) {
    reconnectWebSocket();
  }

  // Your code to interact with WebSocket
}

void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  // Declare variables outside the switch statement
    StaticJsonDocument<200> doc;
    DeserializationError error;
    String jsonString;
  
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket server");
      break;
    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket server");
      
      doc["type"] = "connect";
      doc["name"] = hostname;
      doc["room"] = "room1";

      serializeJson(doc, jsonString);

      // Send JSON string to the server
      webSocket.sendTXT(jsonString);
      webSocket.sendTXT("Hello Server!"); // Example message sent to the server
      break;
    case WStype_TEXT:
      Serial.printf("Message from server: %s\n", payload);
      // Use a local block to handle JSON parsing
      jsonString = String((char*)payload);
      error = deserializeJson(doc, jsonString);
      if (error) {
          Serial.print("DeserializeJson failed: ");
          Serial.println(error.c_str());
          return;
      }

      // Check for a command to turn off all pins
      const char* command = doc["command"];
      if (command && strcmp(command, "OFF_ALL") == 0) {
        turnOffAllPins();
      } else {
            // Extract the "pin" and "state" fields
            int pinNumber = doc["pin"];   // Expected to be a GPIO pin number (e.g., 2, 4, 5)
            bool state = doc["state"];    // true for ON, false for OFF

            // Convert the boolean state to "ON" or "OFF"
            const char* stateStr = state ? "ON" : "OFF";

            // Control the GPIO pin based on the converted state
            bool validPin = false;
            for (int i = 0; i < numPins; i++) {
                if (gpioPins[i] == pinNumber) {
                    validPin = true;
                    if (strcmp(stateStr, "ON") == 0) {
                        digitalWrite(pinNumber, HIGH);  // Turn on the GPIO pin
                        Serial.printf("GPIO pin %d turned ON\n", pinNumber);
                    } else if (strcmp(stateStr, "OFF") == 0) {
                        digitalWrite(pinNumber, LOW);   // Turn off the GPIO pin
                        Serial.printf("GPIO pin %d turned OFF\n", pinNumber);
                    } else {
                        Serial.println("Unknown state received");
                    }
                    // Send status back to server with true/false state
                    sendStatusToServer(pinNumber, state);
                    
                    break;
                }
            }

            if (!validPin) {
                Serial.println("Invalid GPIO pin received");
            }
        }
        

      break;

  }
}

void reconnectWebSocket() {
  static unsigned long lastReconnectAttempt = 0;
  unsigned long now = millis();
  
  if (now - lastReconnectAttempt > 5000) { // Try to reconnect every 5 seconds
    lastReconnectAttempt = now;
    webSocket.begin(websocket_server, websocket_port, "/");
  }
}

void turnOffAllPins() {
  for (int i = 0; i < numPins; i++) {
    digitalWrite(gpioPins[i], LOW);  // Turn off each GPIO pin
  }
  Serial.println("All GPIO pins turned OFF");
}

void turnOnAllPins() {
  for (int i = 0; i < numPins; i++) {
    digitalWrite(gpioPins[i], HIGH);  // Turn on each GPIO pin
  }
  Serial.println("All GPIO pins turned ON");
}

void sendStatusToServer(int pin, bool state) {
  // Create a JSON document
  StaticJsonDocument<200> doc;

  // Add data to the JSON object
  doc["pin"] = pin;
  doc["state"] = state ? "true" : "false";

  // Serialize JSON to a string
  String jsonString;
  serializeJson(doc, jsonString);

  // Send JSON string to the server
  webSocket.sendTXT(jsonString);

  Serial.print("Sent to server: ");
  Serial.println(jsonString);
}
