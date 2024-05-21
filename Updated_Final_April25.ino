#include <TM1637Display.h>  // Include TM1637 Display library
#include "U8glib.h"  // Include U8glib for OLED display
#include "DHT.h"  // Include DHT sensor library

// Define the connections pins to TM1637 and buzzer
#define CLK 2
#define DIO 3
#define BUZZER_PIN  5 // Define the pin connected to the buzzer

// Define button pins
#define BUTTON_SELECT 10 // for pulsing selection
#define BUTTON_INCREASE 8
#define BUTTON_DECREASE 9
#define BUTTON_START_STOP 7
#define BUTTON_RESET 6

// Create display object for TM1637
TM1637Display display(CLK, DIO);

// Define DHT sensor setup
#define DHTPIN 4  // Change as per your connection
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Create OLED display object
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

// Variables to store time, temperature, and timer state
int timeHours = 0;
int timeMinutes = 0;
bool timerRunning = false;
String temp;
String humi;

// Variables for timing with millis() and LED control
unsigned long previousMillis = 0;
bool ledState = LOW;
unsigned long lastLedToggleTime = 0;

// Frequency Selection
int frequencies[] = {0, 500, 1000}; // Frequencies: 0 (constant), 1000 ms, 2000 ms
int currentFrequency = 0; // Default to constant frequency

unsigned long buzzerStartTime = 0;
const unsigned long buzzerDuration = 30000; // 30 seconds


void updateTimeDisplay();
void increaseTime();
void decreaseTime();
void startStopTimer();
void resetTimer();
void timerTick();
void draw();
void updateLed(); // Updated function for LED control

void setup() {
  // Initialize the TM1637 display
  display.setBrightness(0x0f);
  updateTimeDisplay();

  // Initialize the DHT sensor
  dht.begin();

  // Setup button pins
  pinMode(BUTTON_INCREASE, INPUT_PULLUP);
  pinMode(BUTTON_DECREASE, INPUT_PULLUP);
  pinMode(BUTTON_START_STOP, INPUT_PULLUP);
  pinMode(BUTTON_RESET, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);

  // Setup buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off initially

  // Setup the LED pin as an output
  pinMode(12, OUTPUT); 

  // Start serial communication for debugging
  Serial.begin(9600);
  Serial.println("here!");
}

void loop() {
  // Get the current time
  unsigned long currentMillis = millis();

  // Handle the timer functionality
  if (timerRunning && (currentMillis - previousMillis >= 60000)) {
    previousMillis = currentMillis;
    timerTick();
  }

  // Flash LED if the timer is running
  if (timerRunning) {
    updateLed();
  }

  // Read button states
  if (digitalRead(BUTTON_INCREASE) == LOW) {
    increaseTime();
    delay(200); // Simple debounce
  }

  if (digitalRead(BUTTON_DECREASE) == LOW) {
    decreaseTime();
    delay(200); // Simple debounce
  }

  if (digitalRead(BUTTON_START_STOP) == LOW) {
    startStopTimer();
    delay(200); // Simple debounce
  }

  if (digitalRead(BUTTON_RESET) == LOW && !timerRunning) {
    resetTimer();
    delay(200); // Simple debounce
  }

  if (digitalRead(BUTTON_SELECT) == LOW) {
    currentFrequency = (currentFrequency + 1) % 3; // Cycle through frequencies
    delay(200); // Debounce delay
  }

  // Read temperature and humidity from DHT sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Convert readings to String for display
  temp = String(t);
  humi = String(h);

  // Check if readings are valid
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  } else {
    // Display temperature and humidity on OLED
    u8g.firstPage();
    do {
      draw();
    } while (u8g.nextPage());
  }
  delay(10);  // Delay for stability

 // Manage the buzzer time-out
  if (buzzerStartTime > 0 && (millis() - buzzerStartTime >= buzzerDuration)) {
    digitalWrite(BUZZER_PIN, LOW);  // Turn off the buzzer after 30 seconds
    digitalWrite(12, LOW);  // Turn off the LEDs
    buzzerStartTime = 0;  // Reset the buzzer start time
  }


}

void updateTimeDisplay() {
  // Display time in HH:MM format
  display.showNumberDecEx(timeHours * 100 + timeMinutes, 0b01000000, true);
}

void increaseTime() {
  // Increase time by one minute
  timeMinutes = timeMinutes + 10;
  if (timeMinutes >= 60) {
    timeMinutes = 0;
    timeHours++;
    if (timeHours >= 24) {
      timeHours = 0;
    }
  }
  updateTimeDisplay();
}

void decreaseTime() {
  // Decrease time by one minute
  timeMinutes = timeMinutes - 10;
  if (timeMinutes < 0) {
    timeMinutes = 59;
    timeHours--;
    if (timeHours < 0) {
      timeHours = 23;
    }
  }
  updateTimeDisplay();
}

void startStopTimer() {
  timerRunning = !timerRunning; // Toggle timer state
  if (timerRunning) {
    // Adjust previousMillis so the timer immediately counts down
    previousMillis = millis() - 60000;
    digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off when timer starts
  }
}

void resetTimer() {
  // Reset timer to 00:00 and stop it
  timeHours = 0;
  timeMinutes = 0;
  updateTimeDisplay();
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off when timer is reset
}

void timerTick() {
  // Decrement time by one minute
  if (timeMinutes > 0) {
    timeMinutes--;
  } else {
    if (timeHours > 0) {
      timeHours--;
      timeMinutes = 59;
    } else {
      // Timer has reached 00:00
      timerRunning = false;
      digitalWrite(BUZZER_PIN, HIGH); // Activate buzzer
    }

    if (timeMinutes == 0){
      tone(BUZZER_PIN,HIGH);
    }
  }
  updateTimeDisplay();
}

void draw() {
  // Function to draw temperature, humidity, and LED frequency on OLED
  u8g.setFont(u8g_font_unifont);
  u8g.drawStr(0, 11, "Temp");
  u8g.setPrintPos(70, 11);
  u8g.print(temp + " C");
  u8g.drawStr(0, 28, "Humid");
  u8g.setPrintPos(70, 28);
  u8g.print(humi + "%");
  u8g.drawStr(0, 45, "Freq (ms)");
  u8g.setPrintPos(70, 45);
  u8g.print(frequencies[currentFrequency]);
}

void updateLed() {
 if (frequencies[currentFrequency] == 0) {
    // If frequency is 0 ms, keep the LED constantly on
    digitalWrite(12, HIGH);
  } 
  else {
  // Update LED based on selected frequency
  unsigned long currentMillis = millis();
  if (currentMillis - lastLedToggleTime >= frequencies[currentFrequency]) {
    ledState = !ledState;
    digitalWrite(12, ledState);
    lastLedToggleTime = currentMillis;
  }
}
}
