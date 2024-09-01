/*
	Esp32 Websockets Client
	Created 15/02/2019
	By Gil Maimon
	https://github.com/gilmaimon/ArduinoWebsockets

*/

#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <ArduinoJson.h>

const char* ssid = "Rm117"; //Enter SSID
const char* password = "Huskies1337$"; //Enter Password
const char* websockets_server_host = "10.0.0.211"; //Enter server adress
const uint16_t websockets_server_port = 4444; // Enter server port
const char* hostname = "ESP32SocketClient";
const char* room = "room1";

using namespace websockets;

// GPIO pins to control
const int gpioPins[] = {2,4,5,12,13,14,15,18,19,21,22,23,25,26,27,32,33,34,35,36,39};  // Example array of GPIO pins
const int numPins = sizeof(gpioPins) / sizeof(gpioPins[0]);

WebsocketsClient client;

String createJsonString(const String& type, const String& name, const String& room) {
    return String("{") +
           "\"type\":\"" + type + "\"," +
           "\"name\":\"" + name + "\"," +
           "\"room\":\"" + room + "\"" +
           "}";
}

void setup() {
    Serial.begin(115200);

    // Set the GPIO pins as outputs
    for (int i = 0; i < numPins; i++) {
      pinMode(gpioPins[i], OUTPUT);
    }

    // Connect to wifi
    WiFi.begin(ssid, password);

    // Set the hostname
    WiFi.setHostname(hostname);

    // Wait some time to connect to wifi
    for(int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
        Serial.print(".");
        delay(1000);
    }

    // Check if connected to wifi
    if(WiFi.status() != WL_CONNECTED) {
        Serial.println("No Wifi!");
        return;
    }

    Serial.println("Connected to Wifi, Connecting to server.");
    reconnectWebSocket();
    // try to connect to Websockets server
    bool connected = client.connect(websockets_server_host, websockets_server_port, "/");
    if(connected) {
        // Send JSON response upon successful connection
        String jsonString = createJsonString("connect", hostname, room);
        client.send(jsonString);
    } else {
        Serial.println("Not Connected!");
    }
    
    // run callback when messages are received
    client.onMessage([&](WebsocketsMessage message){
        // Use a local block to handle JSON parsing
      // Parse the JSON message
      StaticJsonDocument<200> doc;  // Adjust size as needed
      //jsonString = String((char*)payload);
      DeserializationError error = deserializeJson(doc, message.data());
      if (error) {
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
                    } else if (strcmp(stateStr, "OFF") == 0) {
                        digitalWrite(pinNumber, LOW);   // Turn off the GPIO pin
                    } else {
                    }
                    // Send status back to server with true/false state
                    sendStatusToServer(pinNumber, state);
                    
                    break;
                }
            }
            if (!validPin) {
            }
        }       
    });
}

void loop() {
    // let the websockets client check for incoming messages
    if(client.available()) {
        client.poll();
    } 

    static bool connected = false;

    if (!connected) {
        Serial.println("Attempting to reconnect...");
        connected = client.connect(websockets_server_host, websockets_server_port, "/");
        if (connected) {
            Serial.println("Reconnected to server!");
        } else {
            Serial.println("Reconnect failed.");
            reconnectWebSocket();
        }
    }

    delay(500);
}

void reconnectWebSocket() {
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();
    
    if (now - lastReconnectAttempt > 5000) { // Try to reconnect every 5 seconds
        lastReconnectAttempt = now;
        if (client.connect(websockets_server_host, websockets_server_port, "/")) {
            Serial.println("Reconnected to server!");
            String jsonString = createJsonString("connect", hostname, room);
            client.send(jsonString);
        } else {
            Serial.println("Reconnect failed.");
        }
    }
}

void turnOffAllPins() {
  for (int i = 0; i < numPins; i++) {
    digitalWrite(gpioPins[i], LOW);  // Turn off each GPIO pin
  }
}

void turnOnAllPins() {
  for (int i = 0; i < numPins; i++) {
    digitalWrite(gpioPins[i], HIGH);  // Turn on each GPIO pin
  }
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
  client.send(jsonString);
}



