#include <Adafruit_NeoPixel.h>
#include <BleKeyboard.h>
#include <esp_sleep.h>
#include <EEPROM.h>
#include <esp_mac.h>

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

// EEPROM Configuration
#define EEPROM_SIZE 1
#define EEPROM_ADDR 0

// Device selection
enum Device {
  DEVICE_1 = 0,
  DEVICE_2 = 1,
  DEVICE_3 = 2
};

Device currentDevice = DEVICE_1;
BleKeyboard bleKeyboard("RM-MC25C Blue", "Waveshare", 100);

// Base MAC address (modify last byte per device)
const uint8_t BASE_MAC[6] = {0xE8, 0x06, 0x90, 0x95, 0xF4, 0x29};

// Deep sleep variables
#define DEEP_SLEEP_TIMEOUT 120000 // 2 minutes in milliseconds (disconnected)
#define INACTIVITY_TIMEOUT 300000 // 5 minutes in milliseconds (connected)
unsigned long lastActivityTime = 0;

void setup() {
  Serial.begin(115200);
  delay(100); // Stabilization delay

  // Initialize EEPROM and load saved device
  EEPROM.begin(EEPROM_SIZE);
  currentDevice = static_cast<Device>(EEPROM.read(EEPROM_ADDR));
  if(currentDevice > DEVICE_3) { // Validate stored value
    currentDevice = DEVICE_1;
    EEPROM.write(EEPROM_ADDR, currentDevice);
    EEPROM.commit();
    Serial.println("Invalid EEPROM value, reset to Device 1 (Blue)");
  } else {
    Serial.print("Loaded device from EEPROM: ");
    printDeviceName(currentDevice);
  }

  // Check wakeup reason
  if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woken from deep sleep by button press");
    ledWakeupFeedback(); // Visual confirmation
  }

  initializeComponents();

  pinMode(STOP, INPUT_PULLUP);  // Enable internal pull-up (backup)
  gpio_pullup_en(GPIO_NUM_12);  // Force strong pull-up
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, LOW);  // Wake on LOW (button press)

  setupBluetoothForDevice(currentDevice);
  lastActivityTime = millis();
}

void printDeviceName(Device device) {
  switch(device) {
    case DEVICE_1: Serial.println("Device 1 (Blue)"); break;
    case DEVICE_2: Serial.println("Device 2 (Green)"); break;
    case DEVICE_3: Serial.println("Device 3 (Red)"); break;
  }
}

void printMac(const uint8_t* mac) {
  for(int i=0; i<6; i++) {
    if(i > 0) Serial.print(":");
    Serial.print(mac[i], HEX);
  }
  Serial.println();
}

void setupBluetoothForDevice(Device device) {
  // Create device-specific MAC from base MAC
  uint8_t deviceMac[6];
  memcpy(deviceMac, BASE_MAC, 6);
  deviceMac[5] = BASE_MAC[5] + device;

  // Convert to valid Bluetooth LE random static address
  deviceMac[0] = (deviceMac[0] | 0x02) & 0xFE;
  
  // Set MAC address
  esp_base_mac_addr_set(deviceMac);

  // Set device name based on current device
  String deviceName;
  switch(device) {
    case DEVICE_1: 
      deviceName = "RM-MC25C Blue";
      break;
    case DEVICE_2: 
      deviceName = "RM-MC25C Green";
      break;
    case DEVICE_3: 
      deviceName = "RM-MC25C Red";
      break;
  }
  
  // Initialize BLE Keyboard
  bleKeyboard.setName(deviceName.c_str());
  bleKeyboard.begin();

  // Print debug info
  Serial.print("Advertising as: ");
  Serial.print(deviceName);
  Serial.print(" with MAC: ");
  printMac(deviceMac);
  Serial.println();
}

void ledWakeupFeedback() {
  Serial.println("Playing wakeup LED sequence");
  // Quick LED cycle (red->blue->green)
  digitalWrite(RED_LED, HIGH); delay(200);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, HIGH); delay(200);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, HIGH); delay(200);
  digitalWrite(GREEN_LED, LOW);
}

void initializeComponents() {
  Serial.println("Initializing hardware components...");
  
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
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(PLAY, INPUT_PULLUP);
  pinMode(STOP, INPUT_PULLUP);
  pinMode(BACK, INPUT_PULLUP);
  pinMode(NEXT, INPUT_PULLUP);

  // Startup LED sequence
  Serial.println("Playing startup LED sequence");
  digitalWrite(RED_LED, HIGH); delay(300);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, HIGH); delay(300);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, HIGH); delay(300);
  digitalWrite(GREEN_LED, LOW);
}

void loop() {
  checkDeviceChange(); // Check if user wants to change device
  
  if(bleKeyboard.isConnected()) {
    handleConnectedState();
  } else {
    handleDisconnectedState();
  }
  delay(10);
}

void checkDeviceChange() {
  static bool checkingChange = false;
  static unsigned long changeStartTime = 0;
  
  // Check if DOWN + STOP are pressed together
  if (digitalRead(DOWN) == LOW && digitalRead(STOP) == LOW) {
    if (!checkingChange) {
      checkingChange = true;
      changeStartTime = millis();
      Serial.println("Device change mode activated");
      // Visual feedback that device change mode is active
      blinkControlLed(5, 100);
    }
    
    // If held for 1 second, change device and reboot
    if (millis() - changeStartTime > 1000) {
      Serial.println("Changing device and rebooting...");
      changeDevice();
    }
  } else if (checkingChange) {
    checkingChange = false;
    Serial.println("Device change cancelled");
  }
}

void changeDevice() {
  // Cycle to next device
  Device newDevice = static_cast<Device>((currentDevice + 1) % 3);
  Serial.print("Changing from ");
  printDeviceName(currentDevice);
  Serial.print(" to ");
  printDeviceName(newDevice);
  
  // Save to EEPROM
  EEPROM.write(EEPROM_ADDR, newDevice);
  if(!EEPROM.commit()) {
    Serial.println("Failed to save device to EEPROM!");
  } else {
    Serial.println("Device saved to EEPROM");
  }
  
  // Visual feedback before reboot
  blinkControlLed(10, 50);
  delay(500);
  
  // Reboot the device
  ESP.restart();
}

void blinkControlLed(int times, int duration) {
  Serial.print("Blinking control LED ");
  Serial.print(times);
  Serial.println(" times");
  switch(currentDevice) {
    case DEVICE_1:
      blinkLed(BLUE_LED, times, duration);
      break;
    case DEVICE_2:
      blinkLed(GREEN_LED, times, duration);
      break;
    case DEVICE_3:
      blinkLed(RED_LED, times, duration);
      break;
  }
}

void blinkLed(int ledPin, int times, int duration) {
  for(int i = 0; i < times; i++) {
    digitalWrite(ledPin, HIGH);
    delay(duration);
    digitalWrite(ledPin, LOW);
    if(i < times - 1) delay(duration);
  }
}

void handleConnectedState() {
  static unsigned long lastConnectionCheck = 0;
  static bool isConnected = false;

  if (!isConnected) {
    isConnected = true;
    lastActivityTime = millis();
    Serial.println("Bluetooth connected!");

    // turn off all leds
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
  }

  // Periodic blink with device's control LED
  if (millis() - lastConnectionCheck >= 30000) {
    Serial.println("Sending connection keep-alive");
    blinkControlLed(2, 100);
    lastConnectionCheck = millis();
  }

  checkButtons();

  if (millis() - lastActivityTime > INACTIVITY_TIMEOUT) {
    Serial.println("Inactivity timeout - entering deep sleep");
    enterDeepSleep();
  }
}

void handleDisconnectedState() {
  static unsigned long lastBlink = 0;

  // Fast blink with device's control LED when disconnected
  if(millis() - lastBlink > 200) {
    switch(currentDevice) {
      case DEVICE_1:
        digitalWrite(BLUE_LED, !digitalRead(BLUE_LED));
        break;
      case DEVICE_2:
        digitalWrite(GREEN_LED, !digitalRead(GREEN_LED));
        break;
      case DEVICE_3:
        digitalWrite(RED_LED, !digitalRead(RED_LED));
        break;
    }
    lastBlink = millis();
  }

  // Enter deep sleep after timeout (2 minutes)
  if(millis() - lastActivityTime > DEEP_SLEEP_TIMEOUT) {
    Serial.println("Disconnected timeout - entering deep sleep");
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
    buttonFeedback();
  });

  checkButton(STOP, "STOP", "CHANGE", [](){
    bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
    buttonFeedback();
  }, [](){
    Serial.println("Sending changing device request...");
  });

  checkButton(BACK, "BACK", "VOL_DOWN", [](){
    bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
    buttonFeedback();
  }, [](){
    bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
    buttonFeedback();
  });

  checkButton(NEXT, "NEXT", "VOL_UP", [](){
    bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
    buttonFeedback();
  }, [](){
    bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
    buttonFeedback();
  });
}

void buttonFeedback() {
  lastActivityTime = millis(); // Reset activity timer
  
  // Blink with device's control LED when any button is pressed
  switch(currentDevice) {
    case DEVICE_1:
      digitalWrite(BLUE_LED, HIGH); delay(100); digitalWrite(BLUE_LED, LOW);
      break;
    case DEVICE_2:
      digitalWrite(GREEN_LED, HIGH); delay(100); digitalWrite(GREEN_LED, LOW);
      break;
    case DEVICE_3:
      digitalWrite(RED_LED, HIGH); delay(100); digitalWrite(RED_LED, LOW);
      break;
  }
}

// For buttons without secondary action
void checkButton(int button, const char* name, void (*action)()) {
  static bool lastStates[12] = {false};
  bool currentState = (digitalRead(button) == LOW);

  if(currentState != lastStates[button]) {
    delay(5); // Debounce
    currentState = (digitalRead(button) == LOW);

    if(currentState) {
      Serial.print(name);
      Serial.println(" pressed");
      action();
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
      if(downPressed) {
        Serial.print(altName);
        Serial.println(" pressed");
        altAction();
      } else {
        Serial.print(name);
        Serial.println(" pressed");
        action();
      }
    }

    lastStates[button] = currentState;
  }
}
