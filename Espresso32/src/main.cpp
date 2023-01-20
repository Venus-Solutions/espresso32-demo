#include <Arduino.h>

// Import ThingsBoard library
#include <ThingsBoard.h>
#include <WiFi.h>
#include <ezButton.h>

// Define I/O pins
#define LED_PIN       3 // LED pin
#define BUTTON_PIN    7 // Push button pin

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

ezButton button(BUTTON_PIN);

int ledState = LOW;
int buttonState = LOW;

void initilizeWiFi(void);
const bool reconnect(void);

void setup() {
  pinMode(LED_PIN, OUTPUT);
  // Set debounce time to 50 milliseconds
  button.setDebounceTime(50);

  // If analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));
  
  // Initalize serial connection for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);
  delay(1000);
}

void loop() {
  button.loop();

  if (button.isPressed()) {
    Serial.println("The button is pressed");

    // Toggle state
    ledState = !ledState;
    buttonState = !buttonState;

    digitalWrite(LED_PIN, ledState);
  }

  if (!reconnect()) {
    return;
  }

  if (!tb.connected()) {
    // Connect to the TonySpace
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
      Serial.println("Failed to connect");
      return;
    }
  }

  Serial.println("Sending data...");
  tb.sendTelemetryInt("temperature", random(10, 31));
  tb.sendTelemetryInt("humidity", random(40, 90));
  tb.sendTelemetryBool("led", (bool)ledState);
  tb.sendTelemetryBool("button", (bool)buttonState);

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