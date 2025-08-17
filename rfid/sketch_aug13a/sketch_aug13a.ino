#include <SPI.h>
#include <MFRC522.h>

// ===== SPI pin mapping for ESP32-S3 (edit if you wire differently) =====
#define PIN_SCK  13   // RC522 SCK
#define PIN_MISO 11   // RC522 MISO
#define PIN_MOSI 12   // RC522 MOSI
#define PIN_SS 14   // RC522 SDA/SS (chip select)
#define PIN_RST 10   // RC522 RST

#define BUTTON_PIN 1 // Button Pin pull up
#define BUZZER_PIN 2

byte AUTH_UID[] = { 0xBF, 0xFB, 0x22, 0x43};  // placeholder

// create RFID object
MFRC522 rfid(PIN_SS, PIN_RST);

// for button pull up logic
unsigned long lastBtnChange = 0;
bool lastBtnState = HIGH;  // pull-up -> idle HIGH

bool isAuthorized() {
  if (rfid.uid.size != sizeof(AUTH_UID)) return false;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] != AUTH_UID[i]) return false;
  }
  return true;
}

void resetReader() {
  // Reinitialize the chip cleanly
  rfid.PCD_Reset();
  rfid.PCD_Init();
  Serial.println("RFID reset.");
  Serial.println("RFID Locked.");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  ledcAttach(BUZZER_PIN, 1000, 11);

  byte AUTH_UID[] = { 0xDE, 0xAD, 0xBE, 0xEF };  // placeholder

  // Initialized SPI
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SS);

  // Initialized RFID RC522
  rfid.PCD_Init();

  

  // SPI Validation, it check whether SPI is working in serial monitor
  rfid.PCD_DumpVersionToSerial();
  Serial.print("RFID Locked");
}

void loop() {
  // put your main code here, to run repeatedly:
  bool now = digitalRead(BUTTON_PIN);
  unsigned long time = millis();
  if(now != lastBtnState && ((time - lastBtnChange) > 40)){ // only change if the last button state is different and only once every 40ms
    lastBtnChange = time;
    lastBtnState = now;
    if(now == LOW){ // reset if the state is low
      resetReader();
    }
  }

  // Check if card is present, if present continue, else return false
  if (!rfid.PICC_IsNewCardPresent()) return;

  // Check if UID of the card present, if present, then sore it, else return false
  if (!rfid.PICC_ReadCardSerial()) return;

  //print the uid for the first run then copt to byte. JUST FOR FIRST RUN.
  Serial.print("UID:");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(' ');
    if (rfid.uid.uidByte[i] < 0x10) Serial.print('0');
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Authorization
  if(isAuthorized()){
    ledcWriteTone(BUZZER_PIN, 1000);            // 2 kHz beep
    delay(500);
    ledcWriteTone(BUZZER_PIN, 0);               // stop tone
    Serial.println("Unlocked");
  }

  // Only able to do one print per tap
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
