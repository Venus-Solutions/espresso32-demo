#include <Arduino.h>

// Import ThingsBoard library
#include <ThingsBoard.h>
#include <WiFi.h>

// Define I/O pins
#define LED_PIN       16   // Built-in LED pin
#define LED_EXT_PIN   17   // External LED pin

// Helper macro to calculate array size
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

void initilizeWiFi(void);
const bool reconnect(void);
void ledBlink(void);
RPC_Response processSetGpioState(const RPC_Data &data);
RPC_Response processGetGpioState(const RPC_Data &data);

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
// RPC_Callback callback = {"setValue", processSetGpioState};

RPC_Callback callbacks[] = {
  { "setValue",         processSetGpioState },
  { "getValue",         processGetGpioState },
};

// Set to true if application is subscribed for the RPC messages.
bool subscribed = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_EXT_PIN, OUTPUT);
  
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
    // if (!tb.RPC_Subscribe(callback)) {
    //   Serial.println("Failed to subscribe for RPC");
    //   return;
    // }
    // if (!tb.RPC_Subscribe(callbacks, String(COUNT_OF(callbacks)).c_str())) {
    //   Serial.println("Failed to subscribe for RPC");
    //   return;
    // }

    for (size_t i = 0; i < COUNT_OF(callbacks); i++) {
      if (!tb.RPC_Subscribe(callbacks[i])) {
        Serial.println("Failed to subscribe for RPC");
        return;
      }
    }

    Serial.println("Subscribe done");
    subscribed = true;
  }

  tb.loop();

  ledBlink();
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

void ledBlink(void) {
  digitalWrite(LED_EXT_PIN, LOW);
  tb.sendAttributeBool("ext_value", digitalRead(LED_EXT_PIN));
  delay(500);
  digitalWrite(LED_EXT_PIN, HIGH);
  tb.sendAttributeBool("ext_value", digitalRead(LED_EXT_PIN));
  delay(500);
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

  int state = data;

  Serial.print("Setting LED to state ");
  Serial.println(state);

  digitalWrite(LED_PIN, !state ? HIGH : LOW);
  tb.sendAttributeBool("value", !digitalRead(LED_PIN));

  return RPC_Response(NULL, data);
}

RPC_Response processGetGpioState(const RPC_Data &data) {
  Serial.println("Received the get GPIO RPC method");

  int state = data;

  Serial.print("Get GPIO state ");
  Serial.println(state);

  return RPC_Response(NULL, data);
}