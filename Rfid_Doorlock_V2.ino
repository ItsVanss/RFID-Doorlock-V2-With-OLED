#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin for OLED (not used in this case)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SS_PIN 10 // Pin for Slave Select on RFID
#define RST_PIN 9 // Pin for Reset on RFID
#define SERVO_PIN 3 // Pin for servo
#define BUZZER_PIN 8 // Pin for buzzer

Servo myServo; // Servo object
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create RFID instance

// Authorized UID
byte authorizedUID[] = {0xB3, 0x7B, 0xB2, 0x4F}; // Replace with your authorized UID

bool isServoOpen = false; // Status to track if servo is open or closed
unsigned long notificationTimer = 0; // Timer to clear notification after a delay
unsigned long lastActivity = 0; // Timer to track last activity (card scan)

void setup() {
  Serial.begin(9600);  // Initialize serial communication
  SPI.begin();         // Initialize SPI
  mfrc522.PCD_Init();  // Initialize RFID
  myServo.attach(SERVO_PIN); // Attach servo to pin
  myServo.write(0); // Servo in closed position (0 degrees)
  pinMode(BUZZER_PIN, OUTPUT); // Set buzzer pin as output
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Initialize OLED display
  display.clearDisplay(); // Clear OLED display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("Scan kartu RFID untuk membuka/tutup servo.");
  display.display();
}

void loop() {
  // Check if a card is present
  if (!mfrc522.PICC_IsNewCardPresent()) {
    // If no card is present, check if 5 seconds have passed since last activity
    if (millis() - lastActivity > 5000) {
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println("Silahkan tempelkan kartu untuk masuk.");
      display.display();
    }
    return;
  }

  // Check if the card can be read
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Compare the scanned UID with the authorized UID
  if (checkUID()) {
    if (isServoOpen) {
      Serial.println("Menutup servo...");
      tutupServo(); // Close servo if it's open
      buzzerBeep(1); // Buzzer beeps 1 time for closing
      display.clearDisplay(); // Clear OLED display
      display.setCursor(0, 20);
      display.println("Servo tertutup.");
      display.display();
      notificationTimer = millis(); // Set timer to clear notification
    } else {
      Serial.println("Membuka servo...");
      bukaServo(); // Open servo if it's closed
      buzzerBeep(2); // Buzzer beeps 2 times for opening
      display.clearDisplay(); // Clear OLED display
      display.setCursor(0, 20);
      display.println("Servo terbuka.");
      display.display();
      notificationTimer = millis(); // Set timer to clear notification
    }
    isServoOpen = !isServoOpen; // Toggle servo status (open/closed)
    delay(1000); // Avoid multiple scans with a delay
  } else {
    if (isServoOpen) {
      Serial.println("Kartu tidak diotorisasi. Menutup servo...");
      tutupServo(); // Close servo if card is not authorized
      buzzerBeep(3); // Buzzer beeps 3 times for unauthorized card
      display.clearDisplay(); // Clear OLED display
      display.setCursor(0, 20);
      display.println("Kartu tidak diotorisasi.");
      display.display();
      notificationTimer = millis(); // Set timer to clear notification
      isServoOpen = false; // Update servo status to closed
    } else {
      Serial.println("Kartu tidak diotorisasi.");
      buzzerBeep(3); // Buzzer beeps 3 times for unauthorized card
      display.clearDisplay(); // Clear OLED display
      display.setCursor(0, 20);
      display.println("Kartu tidak diotorisasi.");
      display.display();
      notificationTimer = millis(); // Set timer to clear notification
    }
  }

  // Clear notification after a delay
  if (millis() - notificationTimer > 2000) {
    display.clearDisplay();
    display.display();
  }

  // Halt communication with the card
  mfrc522.PICC_HaltA();

  // Update last activity timer
  lastActivity = millis();
}

bool checkUID() {
  for (byte i = 0; i < 4; i++) {
    if (mfrc522.uid.uidByte[i] != authorizedUID[i]) {
      return false;
    }
  }
  return true;
}

void bukaServo() {
  // Smooth movement to open servo from 0 degrees to 90 degrees
  for (int pos = 0; pos <= 90; pos += 5) { // Increment by 5 degrees
    myServo.write(pos);  // Move servo to position "pos"
    delay(15);           // Short delay for smoother movement
  }
}

void tutupServo() {
  // Smooth movement to close servo from 90 degrees to 0 degrees
  for (int pos = 90; pos >= 0; pos -= 5) { // Increment by 5 degrees
    myServo.write(pos);  // Move servo to position "pos"
    delay(15);           // Short delay for smoother movement
  }
}

// Function to make the buzzer beep
void buzzerBeep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH); // Turn on buzzer
    delay(200); // Duration of buzzer tone
    digitalWrite(BUZZER_PIN, LOW);  // Turn off buzzer
    delay(100); // Delay between beeps
  }
}