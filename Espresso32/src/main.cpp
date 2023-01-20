#include <Arduino.h>

// Import ThingsBoard library
#include <ThingsBoard.h>
#include <WiFi.h>

// Define I/O pins
#define LED_PIN       16   // Built-in LED pin

// Helper macro to calculate array size
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

void initilizeWiFi(void);
const bool reconnect(void);
RPC_Response processSetGpioState(const RPC_Data &data);

constexpr char WIFI_SSID[] PROGMEM = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] PROGMEM = "YOUR_WIFI_PASSWORD";

// to understand how to obtain an access token
constexpr char TOKEN[] PROGMEM = "YOUR_DEVICE_ACCESS_TOKEN";

// Thingsboard we want to establish a connection too
constexpr char THINGSBOARD_SERVER[] PROGMEM = "vsmqtt.space";

constexpr uint16_t THINGSBOARD_PORT PROGMEM = 8080U;

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
constexpr uint32_t MAX_MESSAGE_SIZE PROGMEM = 128U;

// Baud rate for the debugging serial connection
constexpr uint32_t SERIAL_DEBUG_BAUD PROGMEM = 115200U;

// Initialize underlying client, used to establish a connection
WiFiClient espClient;

// Initialize ThingsBoard instance with the maximum needed buffer size
ThingsBoardSized<MAX_MESSAGE_SIZE> tb(espClient);

// RPC handlers
RPC_Callback callback = {"setValue", processSetGpioState};

// Set to true if application is subscribed for the RPC messages.
bool subscribed = false;

int ledState = LOW;
int currentLedState = LOW;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  
  // Initalize serial connection for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);
  delay(1000);
}

void loop() {
  if (!reconnect()) {
    return;
  }

  if (!tb.connected()) {
    subscribed = false;

    // Connect to the TonySpace
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
      Serial.println("Failed to connect");
      return;
    }
  }

  if (!subscribed) {
    Serial.println("Subscribing for RPC... ");

    // Perform a subscription. All consequent data processing will happen in
    // callbacks as denoted by callbacks[] array.
    if (!tb.RPC_Subscribe(callback)) {
      Serial.println("Failed to subscribe for RPC");
      return;
    }

    Serial.println("Subscribe done");
    subscribed = true;
  }

  tb.loop();
}

void initilizeWiFi(void) {
  Serial.println("Connecting to AP ...");
  // Attempting to establish a connection to the given WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    // Delay 500ms until a connection has been succesfully established
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

const bool reconnect(void) {
  // Check to ensure we aren't connected yet
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    return true;
  }

  // If we aren't establish a new connection to the given WiFi network
  initilizeWiFi();
  return true;
}

RPC_Response processSetGpioState(const RPC_Data &data) {
  Serial.println("Received the set GPIO RPC method");

  int led = data;

  Serial.print("Setting LED to state ");
  Serial.println(led);

  digitalWrite(LED_PIN, data ? HIGH : LOW);

  return RPC_Response(NULL, data);
}