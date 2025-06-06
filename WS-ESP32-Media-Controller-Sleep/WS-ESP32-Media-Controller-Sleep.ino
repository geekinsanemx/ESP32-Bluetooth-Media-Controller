#include <Adafruit_NeoPixel.h>
#include <BleKeyboard.h>
#include <esp_sleep.h>

#define NEOPIXEL_PIN 21
#define NUMPIXELS 1
Adafruit_NeoPixel pixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

#define RED_LED 6
#define BLUE_LED 5
#define GREEN_LED 4

// Button Pins (using INPUT_PULLUP)
#define DOWN        7  // Modifier button
#define PLAY        9
#define BACK       10
#define NEXT       11
#define STOP       12  // wake-up button (GPIO12)

BleKeyboard bleKeyboard("RM-MC25C Media Controller", "Waveshare", 100);

// Deep sleep variables
#define DEEP_SLEEP_TIMEOUT 120000 // 2 minutes in milliseconds (disconnected)
#define INACTIVITY_TIMEOUT 300000 // 5 minutes in milliseconds (connected)
unsigned long lastActivityTime = 0;

void setup() {
  Serial.begin(115200);
  delay(100); // Stabilization delay

  // Check wakeup reason
  if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    ledWakeupFeedback(); // Visual confirmation
  }

  initializeComponents();
  
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, LOW);  // Wake on LOW (button press)

  bleKeyboard.begin();
  Serial.println("Initialized - Waiting for Bluetooth connection...");
  lastActivityTime = millis();
}

void ledWakeupFeedback() {
  // Quick LED cycle (red->blue->green)
  digitalWrite(RED_LED, HIGH); delay(200);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, HIGH); delay(200);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, HIGH); delay(200);
  digitalWrite(GREEN_LED, LOW);
}

void initializeComponents() {
  // Initialize LEDs
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  // Initialize NeoPixel
  pixel.begin();
  pixel.clear();
  pixel.show();

  // Initialize buttons - special handling for GPIO12
  pinMode(DOWN, INPUT_PULLUP);  // Modifier button
  pinMode(PLAY, INPUT_PULLUP);
  pinMode(STOP, INPUT_PULLUP);
  pinMode(BACK, INPUT_PULLUP);
  pinMode(NEXT, INPUT_PULLUP);

  // Startup LED sequence
  digitalWrite(RED_LED, HIGH); delay(300);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, HIGH); delay(300);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, HIGH); delay(300);
  digitalWrite(GREEN_LED, LOW);
}

void loop() {
  if(bleKeyboard.isConnected()) {
    handleConnectedState();
  } else {
    handleDisconnectedState();
  }
  delay(10);
}

void handleConnectedState() {
  static unsigned long lastConnectionCheck = 0;
  static bool isConnected = false;

  if (!isConnected) {
    isConnected = true;
    lastActivityTime = millis();
    Serial.println("Bluetooth connected!");
    delay(100);
    digitalWrite(BLUE_LED, LOW);
  }

  if (millis() - lastConnectionCheck >= 30000) {
    for (int i = 0; i < 2; i++) {
      digitalWrite(BLUE_LED, HIGH);
      delay(100);
      digitalWrite(BLUE_LED, LOW);
      delay(100);
    }
    lastConnectionCheck = millis();
  }

  checkButtons();

  if (millis() - lastActivityTime > INACTIVITY_TIMEOUT) {
    enterDeepSleep();
  }
}

void handleDisconnectedState() {
  static unsigned long lastBlink = 0;

  // Fast blink when disconnected
  if(millis() - lastBlink > 200) {
    digitalWrite(BLUE_LED, !digitalRead(BLUE_LED));
    lastBlink = millis();
  }

  // Enter deep sleep after timeout (2 minutes)
  if(millis() - lastActivityTime > DEEP_SLEEP_TIMEOUT) {
    enterDeepSleep();
  }
}

void enterDeepSleep() {
  Serial.println("Entering deep sleep...");

  // Visual warning (3 red blinks)
  for(int i = 0; i < 3; i++) {
    digitalWrite(RED_LED, HIGH); delay(200);
    digitalWrite(RED_LED, LOW); delay(200);
  }

  // Cleanup
  pixel.clear(); pixel.show();
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  bleKeyboard.end();

  // Final preparations
  Serial.flush();
  delay(100);

  esp_deep_sleep_start();
}

void checkButtons() {
  checkButton(PLAY, "PLAY", [](){
    bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
    lastActivityTime = millis(); // Reset activity timer
  });

  checkButton(STOP, "STOP", [](){
    bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
    lastActivityTime = millis(); // Reset activity timer
  });

  checkButton(BACK, "BACK", "VOL_DOWN", [](){
    bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
    lastActivityTime = millis(); // Reset activity timer
  }, [](){
    bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
    lastActivityTime = millis(); // Reset activity timer
  });

  checkButton(NEXT, "NEXT", "VOL_UP", [](){
    bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
    lastActivityTime = millis(); // Reset activity timer
  }, [](){
    bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
    lastActivityTime = millis(); // Reset activity timer
  });
}

// For buttons without secondary action
void checkButton(int button, const char* name, void (*action)()) {
  static bool lastStates[12] = {false};
  bool currentState = (digitalRead(button) == LOW);

  if(currentState != lastStates[button]) {
    delay(5); // Debounce
    currentState = (digitalRead(button) == LOW);

    if(currentState) {
      // Blink green once when any button is pressed
      digitalWrite(GREEN_LED, HIGH);
      Serial.print(name);
      Serial.println(" pressed");
      action();
      delay(100);
      digitalWrite(GREEN_LED, LOW);
    }

    lastStates[button] = currentState;
  }
}

// For buttons with secondary action (when DOWN is pressed)
void checkButton(int button, const char* name, const char* altName,
                void (*action)(), void (*altAction)()) {
  static bool lastStates[12] = {false};
  bool currentState = (digitalRead(button) == LOW);
  bool downPressed = (digitalRead(DOWN) == LOW);

  if(currentState != lastStates[button]) {
    delay(5); // Debounce
    currentState = (digitalRead(button) == LOW);

    if(currentState) {
      // Blink green once when any button is pressed
      digitalWrite(GREEN_LED, HIGH);
      if(downPressed) {
        Serial.print(altName);
        Serial.println(" pressed");
        altAction();
      } else {
        Serial.print(name);
        Serial.println(" pressed");
        action();
      }
      delay(100);
      digitalWrite(GREEN_LED, LOW);
    }

    lastStates[button] = currentState;
  }
}
