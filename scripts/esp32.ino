#include <WiFi.h>
#include <WebSocketsClient.h>

// Replace with your network credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// WebSocket server details
const char* websocket_server = "ws://your-websocket-server.com";
const int websocket_port = 80; // Change if your WebSocket server uses a different port

// Create a WebSocket client
WebSocketsClient webSocket;

// Function prototypes
void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void reconnectWebSocket();

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

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
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket server");
      break;
    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket server");
      webSocket.sendTXT("Hello Server!"); // Example message sent to the server
      break;
    case WStype_TEXT:
      Serial.printf("Message from server: %s\n", payload);
      break;
    case WStype_BIN:
      Serial.println("Binary message received");
      break;
    case WStype_PING:
      Serial.println("Ping received");
      break;
    case WStype_PONG:
      Serial.println("Pong received");
      break;
    default:
      Serial.printf("Unknown WebSocket event type: %d\n", type);
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
