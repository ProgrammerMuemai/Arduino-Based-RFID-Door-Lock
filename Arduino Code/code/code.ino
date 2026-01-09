// ===================== ไลบรารีที่ใช้ =====================
// ใช้สื่อสาร SPI (จำเป็นสำหรับ MFRC522)
#include <SPI.h>

// ไลบรารี RFID MFRC522
#include <MFRC522.h>

// ใช้สื่อสาร I2C
#include <Wire.h>

// ไลบรารีจอ LCD I2C
#include <LiquidCrystal_I2C.h>

// ===================== กำหนดขาใช้งาน =====================
// ขา SDA/SS ของ MFRC522
#define SS_PIN 10

// ขา Reset ของ MFRC522
#define RST_PIN 9

// ขารีเลย์หรือ SSR (active-low)
#define RELAY 2

// ขา buzzer
#define BUZZER 3

// ปุ่ม LOCK (ต่อแบบ pull-down: ปล่อย=LOW, กด=HIGH)
#define LOCK 4

// ปุ่ม UNLOCK (pull-down)
#define UNLOCK 5

// ===================== สร้างอ็อบเจกต์ =====================
// สร้างอ็อบเจกต์ LCD ขนาด 16x2 ที่ address 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);

// สร้างอ็อบเจกต์ RFID
MFRC522 rfid(SS_PIN, RST_PIN);

// โครงสร้าง key (ใช้กับ MIFARE Classic)
MFRC522::MIFARE_Key key;

// ตัวแปรเก็บ UID (คงไว้จากโค้ดเดิม)
byte nuidPICC[4];

// ===================== รายการ UID ที่อนุญาต =====================
// UID เป็นเลขฐานสิบ (DEC) ขนาด 4 ไบต์
// เพิ่มบัตรใหม่ = เพิ่มแถวใหม่
const byte ALLOW_LIST[][4] = {
  {163, 174, 133, 165},   // บัตรใบแรก (ของคุณ)
  // {12, 34, 56, 78},    // ตัวอย่างบัตรใบที่ 2
};

// จำนวนบัตรทั้งหมดในระบบ
const byte ALLOW_COUNT = sizeof(ALLOW_LIST) / sizeof(ALLOW_LIST[0]);

// ===================== ฟังก์ชันตรวจ UID =====================
// ตรวจว่า UID ที่อ่านมาอยู่ใน ALLOW_LIST หรือไม่
bool isAllowedUID(const byte *uid, byte size) {

  // ระบบนี้ออกแบบมารองรับ UID 4 ไบต์เท่านั้น
  if (size != 4) return false;

  // วนเช็คทีละบัตรใน ALLOW_LIST
  for (byte i = 0; i < ALLOW_COUNT; i++) {

    bool match = true;

    // เทียบ UID ทีละไบต์
    for (byte j = 0; j < 4; j++) {
      if (uid[j] != ALLOW_LIST[i][j]) {
        match = false;
        break;
      }
    }

    // ถ้าตรงครบ 4 ไบต์ → อนุญาต
    if (match) return true;
  }

  // ไม่พบ UID นี้ในระบบ
  return false;
}

// ===================== SETUP =====================
void setup() {

  // เปิด Serial Monitor ที่ 9600 bps
  Serial.begin(9600);

  // เริ่มต้น SPI
  SPI.begin();

  // เริ่มต้นโมดูล RFID
  rfid.PCD_Init();

  // กำหนดโหมดขา I/O
  pinMode(RELAY, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LOCK, INPUT);      // pull-down ภายนอก
  pinMode(UNLOCK, INPUT);

  // รีเลย์ active-low → HIGH = ปิด (ล็อก)
  digitalWrite(RELAY, HIGH);

  // เริ่มต้น LCD
  lcd.init();
  lcd.backlight();

  // แสดงหน้าเริ่มต้น
  lcdHomeScreen();

  // กำหนด key ของ MIFARE Classic เป็น FF FF FF FF FF FF
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // ข้อความ debug
  Serial.println(F("RFID system ready"));
  Serial.print(F("Allow list count: "));
  Serial.println(ALLOW_COUNT);
}

// ===================== LOOP =====================
void loop() {

  // ---------- ส่วนจัดการปุ่ม (debounce + edge detect) ----------
  static bool lastLock = LOW;
  static bool lastUnlock = LOW;
  static unsigned long lastBtnMs = 0;
  const unsigned long DEBOUNCE_MS = 120;

  // อ่านสถานะปุ่ม (pull-down: กด = HIGH)
  bool nowLock   = digitalRead(LOCK);
  bool nowUnlock = digitalRead(UNLOCK);

  // กันการเด้งของปุ่ม
  if (millis() - lastBtnMs > DEBOUNCE_MS) {

    // กด LOCK (LOW → HIGH)
    if (nowLock == HIGH && lastLock == LOW) {
      digitalWrite(RELAY, HIGH);   // ล็อก
      delay(30);                   // ให้ไฟนิ่ง
      rfid.PCD_Init();             // ฟื้น RFID กันหลุด
      lastBtnMs = millis();
    }

    // กด UNLOCK
    if (nowUnlock == HIGH && lastUnlock == LOW) {
      digitalWrite(RELAY, LOW);    // ปลดล็อก
      delay(30);
      rfid.PCD_Init();
      lastBtnMs = millis();
    }
  }

  // เก็บสถานะปุ่มรอบก่อน
  lastLock = nowLock;
  lastUnlock = nowUnlock;

  // ---------- ส่วนอ่าน RFID ----------
  // ถ้าไม่มีบัตรใหม่ → ออกจาก loop ทันที
  if (!rfid.PICC_IsNewCardPresent()) return;

  // ถ้าอ่าน UID ไม่สำเร็จ → ออก
  if (!rfid.PICC_ReadCardSerial()) return;

  // ตรวจชนิดบัตร
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

  // รองรับเฉพาะ MIFARE Classic
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  // ---------- แสดง UID ใน Serial (ใช้เพิ่มบัตรใหม่) ----------
  Serial.print(F("UID size: "));
  Serial.println(rfid.uid.size);

  Serial.print(F("In hex: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();

  Serial.print(F("In dec: "));
  printDec(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();

  // ---------- ตรวจสิทธิ์ ----------
  if (isAllowedUID(rfid.uid.uidByte, rfid.uid.size)) {

    // ===== บัตรผ่าน =====
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print(">> UNLOCK <<");

    lcd.setCursor(1, 1);
    lcd.print("lock in :  sec");

    // ปลดล็อก
    digitalWrite(RELAY, LOW);

    // เสียง beep
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);

    // นับถอยหลัง 5 วินาที
    for (int i = 5; i >= 0; i--) {
      lcd.setCursor(10, 1);
      lcd.print(i);
      delay(1000);
    }

    // กลับหน้าหลัก
    lcdHomeScreen();

  } else {

    // ===== บัตรไม่ผ่าน =====
    digitalWrite(BUZZER, HIGH); delay(50); digitalWrite(BUZZER, LOW);
    delay(50);
    digitalWrite(BUZZER, HIGH); delay(50); digitalWrite(BUZZER, LOW);

    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Access denied!");
    delay(1000);
    lcdHomeScreen();
  }

  // สั่งหยุดการ์ด
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ===================== หน้าจอเริ่มต้น =====================
void lcdHomeScreen() {
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("WELCOME");

  lcd.setCursor(0, 1);
  lcd.print("TAP CARD TO OPEN");

  // กลับสู่สถานะล็อก
  digitalWrite(RELAY, HIGH);
}

// ===================== ฟังก์ชันช่วยแสดง UID =====================
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
