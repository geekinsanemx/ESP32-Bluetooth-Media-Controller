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
#define DOWN       12  // Modifier button AND wake-up button (GPIO12)
#define PLAY        8
#define STOP        9
#define BACK       10
#define NEXT       11

BleKeyboard bleKeyboard("RM-MC25C Media Controller", "Waveshare", 100);

// Deep sleep variables
#define DEEP_SLEEP_TIMEOUT 120000 // 2 minutes in milliseconds
unsigned long lastConnectionTime = 0;

void setup() {
  Serial.begin(115200);
  delay(100); // Stabilization delay

  // Check wakeup reason
  if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    ledWakeupFeedback(); // Visual confirmation
  }

  initializeComponents();
  
  // Configure wakeup - using GPIO12 (DOWN button)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, LOW);
  
  bleKeyboard.begin();
  Serial.println("Initialized - Waiting for Bluetooth connection...");
  lastConnectionTime = millis();
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
  pinMode(DOWN, INPUT_PULLUP);  // Wake-up button
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
    lastConnectionTime = millis();
    Serial.println("Bluetooth connected!");
  }
  
  // Blink blue twice every 10 seconds when connected
  if (millis() - lastConnectionCheck >= 10000) {
    for (int i = 0; i < 2; i++) {
      digitalWrite(BLUE_LED, HIGH);
      delay(100);
      digitalWrite(BLUE_LED, LOW);
      delay(100);
    }
    lastConnectionCheck = millis();
  }
  
  checkButtons();
}

void handleDisconnectedState() {
  static unsigned long lastBlink = 0;
  
  // Fast blink when disconnected
  if(millis() - lastBlink > 200) {
    digitalWrite(BLUE_LED, !digitalRead(BLUE_LED));
    lastBlink = millis();
  }
  
  // Enter deep sleep after timeout
  if(millis() - lastConnectionTime > DEEP_SLEEP_TIMEOUT) {
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
    lastConnectionTime = millis(); // Reset sleep timer
  });
  
  checkButton(STOP, "STOP", [](){
    bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
    lastConnectionTime = millis(); // Reset sleep timer
  });
  
  checkButton(BACK, "BACK", "VOL_DOWN", [](){
    bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
    lastConnectionTime = millis(); // Reset sleep timer
  }, [](){
    bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
    lastConnectionTime = millis(); // Reset sleep timer
  });
  
  checkButton(NEXT, "NEXT", "VOL_UP", [](){
    bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
    lastConnectionTime = millis(); // Reset sleep timer
  }, [](){
    bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
    lastConnectionTime = millis(); // Reset sleep timer
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
