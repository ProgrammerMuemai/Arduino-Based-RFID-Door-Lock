// ===================== ไลบรารี =====================
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===================== กำหนดขา =====================
#define SS_PIN 10
#define RST_PIN 9

#define RELAY 2
#define BUZZER 3

// จากการทดสอบปุ่มของคุณ
#define UNLOCK 4
#define LOCK   5

// =====================================================
//                 LOGIC ของ SSR
// =====================================================
// LOW  = LOCK
// HIGH = UNLOCK
#define RELAY_LOCK   LOW
#define RELAY_UNLOCK HIGH

// ===================== Object =====================
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);

// =====================================================
//               UID บัตรที่อนุญาต
// =====================================================
// HEX = C0 58 BE 13
// DEC = 192 88 190 19

const byte ALLOW_LIST[][4] = {

  {192, 88, 190, 19},

  // เพิ่มบัตรใบอื่นได้ เช่น
  // {12, 34, 56, 78},
  // {100, 120, 130, 140},

};

const byte ALLOW_COUNT =
  sizeof(ALLOW_LIST) / sizeof(ALLOW_LIST[0]);

// =====================================================
//                  เวลา UNLOCK
// =====================================================

const unsigned long UNLOCK_TIME = 5000;

unsigned long unlockStartMs = 0;

bool doorUnlocked = false;

// =====================================================
//                 Boot / Debounce
// =====================================================

unsigned long bootMs = 0;

const unsigned long BOOT_IGNORE_MS = 1000;
const unsigned long DEBOUNCE_MS = 120;

unsigned long lastBtnMs = 0;

bool lastLock = LOW;
bool lastUnlock = LOW;

// สำหรับอัปเดตเลข Countdown
int lastCountdown = -1;

// =====================================================
//                  ตรวจสอบ UID
// =====================================================

bool isAllowedUID(const byte *uid, byte size) {

  // UID ต้องเป็น 4 byte
  if (size != 4) {
    return false;
  }

  // ตรวจทุกบัตรใน Allow List
  for (byte card = 0; card < ALLOW_COUNT; card++) {

    bool match = true;

    for (byte i = 0; i < 4; i++) {

      if (uid[i] != ALLOW_LIST[card][i]) {

        match = false;
        break;
      }
    }

    if (match) {
      return true;
    }
  }

  return false;
}

// =====================================================
//                    หน้า HOME
// =====================================================

void lcdHomeScreen() {

  lcd.clear();

  lcd.setCursor(4, 0);
  lcd.print("WELCOME");

  lcd.setCursor(0, 1);
  lcd.print("TAP CARD TO OPEN");
}

// =====================================================
//                  หน้า UNLOCK
// =====================================================

void lcdUnlockScreen() {

  lcd.clear();

  lcd.setCursor(2, 0);
  lcd.print(">> UNLOCK <<");

  lcd.setCursor(0, 1);
  lcd.print("Lock in: 5 sec");

  lastCountdown = 5;
}

// =====================================================
//                       LOCK
// =====================================================

void lockDoor() {

  digitalWrite(RELAY, RELAY_LOCK);

  doorUnlocked = false;

  lastCountdown = -1;

  Serial.println(F("DOOR = LOCK"));

  lcdHomeScreen();
}

// =====================================================
//                      UNLOCK
// =====================================================

void unlockDoor() {

  digitalWrite(RELAY, RELAY_UNLOCK);

  doorUnlocked = true;

  // เริ่มนับ 5 วินาทีใหม่ทุกครั้ง
  unlockStartMs = millis();

  Serial.println(F("DOOR = UNLOCK"));
}

// =====================================================
//                  เสียงบัตรผ่าน
// =====================================================

void beepOK() {

  digitalWrite(BUZZER, HIGH);
  delay(80);
  digitalWrite(BUZZER, LOW);
}

// =====================================================
//                  เสียงบัตรไม่ผ่าน
// =====================================================

void beepDenied() {

  digitalWrite(BUZZER, HIGH);
  delay(60);

  digitalWrite(BUZZER, LOW);
  delay(60);

  digitalWrite(BUZZER, HIGH);
  delay(60);

  digitalWrite(BUZZER, LOW);
}

// =====================================================
//                       SETUP
// =====================================================

void setup() {

  Serial.begin(9600);

  // ===================================================
  // I/O
  // ===================================================

  pinMode(RELAY, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // ปุ่มใช้ Pull-down ภายนอก
  // ปล่อย = LOW
  // กด = HIGH
  pinMode(LOCK, INPUT);
  pinMode(UNLOCK, INPUT);

  digitalWrite(BUZZER, LOW);

  // ===================================================
  // เปิดเครื่องมาให้ LOCK ก่อนเสมอ
  // ===================================================

  digitalWrite(RELAY, RELAY_LOCK);

  doorUnlocked = false;

  delay(200);

  digitalWrite(RELAY, RELAY_LOCK);

  // ===================================================
  // SPI
  // ===================================================

  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);

  SPI.begin();

  // ===================================================
  // RFID
  // ===================================================

  rfid.PCD_Init();

  delay(50);

  // ===================================================
  // LCD
  // ===================================================

  lcd.init();
  lcd.backlight();

  lcdHomeScreen();

  // ===================================================
  // เวลาเริ่มระบบ
  // ===================================================

  bootMs = millis();

  // ===================================================
  // Serial Debug
  // ===================================================

  Serial.println();
  Serial.println(F("============================"));
  Serial.println(F(" RFID ACCESS CONTROL READY"));
  Serial.println(F("============================"));

  Serial.println(F("SSR Logic:"));
  Serial.println(F("LOW  = LOCK"));
  Serial.println(F("HIGH = UNLOCK"));

  Serial.println();

  Serial.println(F("Authorized Card:"));
  Serial.println(F("HEX = C0 58 BE 13"));
  Serial.println(F("DEC = 192 88 190 19"));

  Serial.print(F("Allow cards = "));
  Serial.println(ALLOW_COUNT);

  Serial.println();
  Serial.println(F("Default = LOCK"));
}

// =====================================================
//                         LOOP
// =====================================================

void loop() {

  // ===================================================
  //            AUTO LOCK หลัง 5 วินาที
  // ===================================================

  if (doorUnlocked) {

    unsigned long elapsed =
      millis() - unlockStartMs;

    // =================================================
    // คำนวณเวลาที่เหลือ
    // =================================================

    int remaining;

    if (elapsed >= UNLOCK_TIME) {

      remaining = 0;

    } else {

      remaining =
        (UNLOCK_TIME - elapsed + 999) / 1000;
    }

    // =================================================
    // อัปเดต LCD เฉพาะตอนเลขเปลี่ยน
    // =================================================

    if (remaining != lastCountdown) {

      lastCountdown = remaining;

      lcd.setCursor(9, 1);

      // ลบข้อมูลเก่า
      lcd.print("       ");

      lcd.setCursor(9, 1);

      lcd.print(remaining);
      lcd.print(" sec");
    }

    // =================================================
    // ครบ 5 วินาที
    // =================================================

    if (elapsed >= UNLOCK_TIME) {

      Serial.println(F("AUTO LOCK AFTER 5 SEC"));

      lockDoor();
    }
  }

  // ===================================================
  //                   อ่านปุ่ม
  // ===================================================

  bool nowLock =
    digitalRead(LOCK);

  bool nowUnlock =
    digitalRead(UNLOCK);

  // ไม่รับปุ่มช่วงบูต 1 วินาที
  if (millis() - bootMs >= BOOT_IGNORE_MS) {

    if (millis() - lastBtnMs >= DEBOUNCE_MS) {

      // =================================================
      //                   ปุ่ม LOCK
      // =================================================

      if (nowLock == HIGH &&
          lastLock == LOW) {

        Serial.println(F("BUTTON = LOCK"));

        // ล็อกทันที
        lockDoor();

        beepOK();

        lastBtnMs = millis();
      }

      // =================================================
      //                  ปุ่ม UNLOCK
      // =================================================

      if (nowUnlock == HIGH &&
          lastUnlock == LOW) {

        Serial.println(F("BUTTON = UNLOCK"));

        // ปลดล็อก
        unlockDoor();

        // เริ่มหน้าจอนับ 5 วินาที
        lcdUnlockScreen();

        beepOK();

        lastBtnMs = millis();
      }
    }
  }

  // บันทึกสถานะปุ่มล่าสุด
  lastLock = nowLock;
  lastUnlock = nowUnlock;

  // ===================================================
  //                      RFID
  // ===================================================

  if (rfid.PICC_IsNewCardPresent()) {

    if (rfid.PICC_ReadCardSerial()) {

      Serial.println();
      Serial.println(F("----------------------------"));

      // =================================================
      // แสดง UID HEX
      // =================================================

      Serial.print(F("UID HEX = "));

      printHex(
        rfid.uid.uidByte,
        rfid.uid.size
      );

      Serial.println();

      // =================================================
      // แสดง UID DEC
      // =================================================

      Serial.print(F("UID DEC = "));

      printDec(
        rfid.uid.uidByte,
        rfid.uid.size
      );

      Serial.println();

      // =================================================
      // ตรวจสอบ UID
      // =================================================

      bool allowed =
        isAllowedUID(
          rfid.uid.uidByte,
          rfid.uid.size
        );

      // =================================================
      //                    บัตรผ่าน
      // =================================================

      if (allowed) {

        Serial.println(F("ACCESS = GRANTED"));

        // -----------------------------------------------
        // ปลดล็อก
        //
        // ถ้ากำลัง Unlock อยู่แล้ว
        // การแตะบัตรซ้ำจะเริ่มนับ 5 วินาทีใหม่
        // -----------------------------------------------

        unlockDoor();

        lcdUnlockScreen();

        beepOK();
      }

      // =================================================
      //                   บัตรไม่ผ่าน
      // =================================================

      else {

        Serial.println(F("ACCESS = DENIED"));

        beepDenied();

        // ------------------------------------------------
        // ถ้าประตู LOCK อยู่
        // แสดง Access Denied
        // ------------------------------------------------

        if (!doorUnlocked) {

          lcd.clear();

          lcd.setCursor(1, 0);
          lcd.print("ACCESS DENIED!");

          lcd.setCursor(1, 1);
          lcd.print("INVALID CARD");

          delay(700);

          lcdHomeScreen();
        }

        // ------------------------------------------------
        // ถ้ากำลัง UNLOCK อยู่
        // บัตรผิดจะไม่ไปต่อเวลา
        // และไม่เปลี่ยนสถานะประตู
        //
        // ระบบจะนับ 5 วินาทีเดิมต่อ
        // ------------------------------------------------
      }

      // =================================================
      // จบ RFID Session
      // =================================================

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }
}

// =====================================================
//                    แสดง UID HEX
// =====================================================

void printHex(byte *buffer, byte bufferSize) {

  for (byte i = 0; i < bufferSize; i++) {

    if (buffer[i] < 0x10) {
      Serial.print(" 0");
    } else {
      Serial.print(" ");
    }

    Serial.print(buffer[i], HEX);
  }
}

// =====================================================
//                    แสดง UID DEC
// =====================================================

void printDec(byte *buffer, byte bufferSize) {

  for (byte i = 0; i < bufferSize; i++) {

    Serial.print(" ");
    Serial.print(buffer[i], DEC);
  }
}