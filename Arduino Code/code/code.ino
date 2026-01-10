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
#define SS_PIN 10          // ขา SDA/SS ของ MFRC522
#define RST_PIN 9          // ขา Reset ของ MFRC522
#define RELAY 2            // ขารีเลย์หรือ SSR (active-low)
#define BUZZER 3           // ขา buzzer
#define LOCK 4             // ปุ่ม LOCK (pull-down: ปล่อย=LOW, กด=HIGH)
#define UNLOCK 5           // ปุ่ม UNLOCK (pull-down)

// ===================== สร้างอ็อบเจกต์ =====================
LiquidCrystal_I2C lcd(0x27, 16, 2);   // LCD 16x2 ที่ address 0x27
MFRC522 rfid(SS_PIN, RST_PIN);        // RFID object
MFRC522::MIFARE_Key key;              // key สำหรับ MIFARE Classic
byte nuidPICC[4];                     // คงไว้จากโค้ดเดิม

// ===================== รายการ UID ที่อนุญาต =====================
const byte ALLOW_LIST[][4] = {
  {163, 174, 133, 165},   // บัตรใบแรก (ของคุณ)
  // {12, 34, 56, 78},    // ตัวอย่างบัตรใบที่ 2
};
const byte ALLOW_COUNT = sizeof(ALLOW_LIST) / sizeof(ALLOW_LIST[0]);

// ===================== ตัวแปรระบบกันเด้ง/กันบูตเด้ง =====================
unsigned long bootMs = 0;             // เวลาเริ่มบูต เพื่อ ignore ปุ่มช่วงแรก
const unsigned long BOOT_IGNORE_MS = 1000;  // ไม่รับปุ่มช่วง 1 วินาทีแรกหลังเปิดเครื่อง

// ===================== ฟังก์ชันตรวจ UID =====================
bool isAllowedUID(const byte *uid, byte size) {
  if (size != 4) return false; // โค้ดนี้รองรับ UID 4 ไบต์เท่านั้น

  for (byte i = 0; i < ALLOW_COUNT; i++) {
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (uid[j] != ALLOW_LIST[i][j]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

// ===================== ฟังก์ชันหน้าเริ่มต้น =====================
void lcdHomeScreen() {
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("WELCOME");
  lcd.setCursor(0, 1);
  lcd.print("TAP CARD TO OPEN");

  // บังคับกลับไป LOCK ทุกครั้งที่กลับหน้าหลัก
  // relay/SSR active-low => HIGH = ปิดวงจร (ล็อก)
  digitalWrite(RELAY, HIGH);
}

// ===================== SETUP =====================
void setup() {
  // เปิด Serial Monitor
  Serial.begin(9600);

  // ---------- ตั้งค่า I/O ก่อน (กันอาการรีเลย์เด้งตอนบูต) ----------
  pinMode(RELAY, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LOCK, INPUT);       // คุณต่อแบบ pull-down ภายนอก
  pinMode(UNLOCK, INPUT);

  // บังคับให้ "ล็อกแน่นอน" ตั้งแต่เริ่ม
  // (กันโมดูลรีเลย์/SSR บางรุ่นพริบตอนบูต)
  digitalWrite(RELAY, HIGH);
  delay(200);
  digitalWrite(RELAY, HIGH);

  // ---------- บังคับ UNO เป็น SPI master แน่ ๆ (สำคัญ) ----------
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);

  // เริ่มต้น SPI + RFID
  SPI.begin();
  rfid.PCD_Init();

  // เริ่มต้น LCD
  lcd.init();
  lcd.backlight();
  lcdHomeScreen();

  // ตั้ง key เป็น FF FF FF FF FF FF (คงไว้ตามเดิม)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // ตั้งเวลาเริ่มบูต เพื่อ ignore ปุ่มช่วงแรก
  bootMs = millis();

  // ข้อความ debug
  Serial.println(F("RFID system ready"));
  Serial.print(F("Allow list count: "));
  Serial.println(ALLOW_COUNT);
}

// ===================== LOOP =====================
void loop() {

  // ---------- ignore ปุ่มช่วงบูต 1 วินาทีแรก ----------
  // กันปุ่มเด้ง/สัญญาณรบกวนช่วงเปิดเครื่อง ทำให้ปลดล็อกเอง
  if (millis() - bootMs < BOOT_IGNORE_MS) {
    // แต่ยังให้สแกนบัตรได้ตามปกติ ถ้าคุณอยากให้สแกนได้เลย
    // หากอยาก freeze ทั้งระบบช่วงบูต ให้ return; ได้เลย
    // return;
  }

  // ---------- ส่วนจัดการปุ่ม (debounce + edge detect) ----------
  static bool lastLock = LOW;
  static bool lastUnlock = LOW;
  static unsigned long lastBtnMs = 0;
  const unsigned long DEBOUNCE_MS = 120;

  bool nowLock   = digitalRead(LOCK);    // pull-down: กด = HIGH
  bool nowUnlock = digitalRead(UNLOCK);  // pull-down: กด = HIGH

  // ทำงานเฉพาะหลังพ้นช่วง ignore และผ่าน debounce
  if (millis() - bootMs >= BOOT_IGNORE_MS) {
    if (millis() - lastBtnMs > DEBOUNCE_MS) {

      // กด LOCK ครั้งเดียว (LOW -> HIGH)
      if (nowLock == HIGH && lastLock == LOW) {
        digitalWrite(RELAY, HIGH);  // ล็อก (active-low)
        delay(30);                  // ให้ไฟนิ่ง
        rfid.PCD_Init();            // ฟื้น RFID กันหลุด (จาก EMI)
        lastBtnMs = millis();
      }

      // กด UNLOCK ครั้งเดียว (LOW -> HIGH)
      if (nowUnlock == HIGH && lastUnlock == LOW) {
        digitalWrite(RELAY, LOW);   // ปลดล็อก
        delay(30);
        rfid.PCD_Init();
        lastBtnMs = millis();
      }
    }
  }

  lastLock = nowLock;
  lastUnlock = nowUnlock;

  // ---------- ส่วนอ่าน RFID ----------
  if (!rfid.PICC_IsNewCardPresent()) return;
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

    // เสียง beep 1 ครั้ง
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);

    // นับถอยหลัง 5 วินาที
    for (int i = 5; i >= 0; i--) {
      lcd.setCursor(10, 1);
      lcd.print(i);
      delay(1000);
    }

    // กลับหน้าหลัก (และล็อก)
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

  // จบ session ของบัตร
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
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
