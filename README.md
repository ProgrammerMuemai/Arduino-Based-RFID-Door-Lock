# Arduino-Based-RFID-Door-Lock
ดูวีดีโอการทำงานได้ที่
https://youtu.be/LQMiO3PaC_U

# RFID Door Lock System with Arduino UNO

ระบบควบคุมประตูด้วยบัตร RFID โดยใช้ **Arduino UNO**, **MFRC522**, **LCD I2C** และ **Relay / SSR**

ระบบตรวจสอบสิทธิ์จาก UID ของบัตร RFID รองรับบัตรได้หลายใบ พร้อมปุ่ม **LOCK / UNLOCK แบบ Manual** และระบบ **Auto Lock หลังปลดล็อก 5 วินาที**

---

## 🔐 Features

- รองรับบัตร RFID หลายใบด้วย UID แบบ 4-byte
- ตรวจสอบสิทธิ์จาก UID ของบัตร
- แสดงสถานะผ่านจอ LCD 16x2 แบบ I2C
- ควบคุมกลอนประตูแม่เหล็ก (Magnetic Door Lock)
- รองรับ Relay / SSR
- ปุ่มกด LOCK / UNLOCK แบบ Manual
- ปุ่มต่อแบบ Pull-down
- มีระบบ Debounce ป้องกันปุ่มเด้ง
- กด UNLOCK แล้วล็อกกลับอัตโนมัติภายใน 5 วินาที
- แตะบัตรที่ได้รับอนุญาตแล้วปลดล็อก 5 วินาที
- LCD แสดงเวลานับถอยหลังจนกว่าจะ LOCK
- สามารถกด LOCK เพื่อขัดจังหวะและล็อกทันทีได้
- สามารถกด UNLOCK ซ้ำเพื่อเริ่มนับเวลา 5 วินาทีใหม่
- สามารถแตะบัตร RFID ขณะกำลังนับถอยหลังได้
- แตะบัตรที่ได้รับอนุญาตซ้ำ จะเริ่มนับ 5 วินาทีใหม่
- บัตรที่ไม่ได้รับอนุญาตจะไม่สามารถปลดล็อกได้
- แสดง UID ทั้ง HEX และ DEC ผ่าน Serial Monitor
- เพิ่มบัตรใหม่ใน `ALLOW_LIST` ได้ง่าย
- ใช้ `millis()` สำหรับ Auto Lock ทำให้ระบบไม่ค้างระหว่างนับเวลา

---

## 🧰 Hardware Required

- Arduino UNO
- MFRC522 RFID Reader
- RFID Card / Key Tag
- LCD 16x2 I2C
- Relay Module หรือ SSR
- Magnetic Door Lock 12V
- Buzzer
- Push Button x2
  - LOCK
  - UNLOCK
- Resistor สำหรับ Pull-down ปุ่มกด
- Power Supply 12V
- สาย Jumper
- Breadboard หรือ PCB ตามรูปแบบงาน

> สำหรับ Magnetic Door Lock ควรเลือก Power Supply ที่มีกระแสเพียงพอกับกลอนประตูที่ใช้งานจริง  
> โดยทั่วไปแนะนำอย่างน้อยประมาณ **12V 3A** หากใช้ร่วมกับอุปกรณ์อื่นในระบบ

---

## ⚠️ Important: MFRC522 Voltage

MFRC522 ใช้แรงดันไฟประมาณ **3.3V**

ควรต่อ:

```text
MFRC522 VCC -> Arduino 3.3V
```

ไม่ควรต่อ VCC ของ MFRC522 เข้ากับ 5V โดยตรง

---

## 🔌 Pin Configuration

| Device | Arduino UNO Pin |
|---|---:|
| MFRC522 SDA / SS | D10 |
| MFRC522 RST | D9 |
| MFRC522 MOSI | D11 |
| MFRC522 MISO | D12 |
| MFRC522 SCK | D13 |
| Relay / SSR | D2 |
| Buzzer | D3 |
| Unlock Button | D4 |
| Lock Button | D5 |
| LCD SDA | A4 |
| LCD SCL | A5 |

---

## 📡 MFRC522 Wiring

| MFRC522 | Arduino UNO |
|---|---|
| SDA / SS | D10 |
| SCK | D13 |
| MOSI | D11 |
| MISO | D12 |
| IRQ | Not Connected |
| GND | GND |
| RST | D9 |
| 3.3V | 3.3V |

---

## 🖥 LCD I2C Wiring

| LCD I2C | Arduino UNO |
|---|---|
| VCC | 5V |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

LCD ที่ใช้ในโปรเจกต์นี้:

```cpp
LiquidCrystal_I2C lcd(0x27, 16, 2);
```

ดังนั้น I2C Address คือ:

```text
0x27
```

หากจอไม่แสดงผล อาจต้องตรวจสอบ I2C Address ของจอที่ใช้งานจริง

---

## 🔘 Push Button Wiring

ระบบใช้ปุ่มกดแบบ **Pull-down**

สถานะของปุ่ม:

```text
ไม่กด = LOW
กด     = HIGH
```

ขาที่ใช้:

```text
UNLOCK Button -> D4
LOCK Button   -> D5
```

ตัวอย่างการต่อปุ่ม:

```text
5V
 |
 |
[ Push Button ]
 |
 +---------- Arduino Input
 |
[ 10KΩ ]
 |
GND
```

---

## ⚡ Relay / SSR Logic

ระบบปัจจุบันกำหนด Logic ของขา D2 ดังนี้:

```cpp
#define RELAY_LOCK   LOW
#define RELAY_UNLOCK HIGH
```

ดังนั้น:

| Arduino D2 | Door State |
|---|---|
| LOW | 🔒 LOCK |
| HIGH | 🔓 UNLOCK |

หรือสรุปง่าย ๆ:

```text
LOW  = LOCK
HIGH = UNLOCK
```

> **หมายเหตุ:** Relay หรือ SSR แต่ละรุ่นอาจมี Logic การ Trigger แตกต่างกัน  
> ควรทดสอบโมดูลที่ใช้งานจริงก่อนเชื่อมต่อกับ Magnetic Door Lock

---

## 🧠 How It Works

### 1. เปิดเครื่อง

เมื่อเปิดระบบ Arduino จะกำหนดสถานะเริ่มต้นเป็น:

```text
LOCK
```

และ LCD จะแสดง:

```text
    WELCOME
TAP CARD TO OPEN
```

---

### 2. แตะบัตร RFID

Arduino จะอ่าน UID ของบัตรจาก MFRC522 และนำไปเปรียบเทียบกับรายการ:

```cpp
ALLOW_LIST
```

---

### 3. ถ้าบัตรได้รับอนุญาต

ระบบจะ:

1. ปลดล็อกประตู
2. ส่งเสียง Beep
3. แสดงสถานะ `UNLOCK` บน LCD
4. เริ่มนับถอยหลัง 5 วินาที
5. เมื่อครบเวลา ระบบจะ LOCK อัตโนมัติ
6. LCD กลับไปหน้า WELCOME

ตัวอย่าง:

```text
  >> UNLOCK <<
Lock in: 5 sec
```

จากนั้นจะนับ:

```text
Lock in: 4 sec
Lock in: 3 sec
Lock in: 2 sec
Lock in: 1 sec
```

เมื่อครบเวลา:

```text
LOCK
```

และกลับไป:

```text
    WELCOME
TAP CARD TO OPEN
```

---

### 4. ถ้าบัตรไม่ได้รับอนุญาต

ระบบจะ:

- ไม่ปลดล็อกประตู
- ส่งเสียงเตือน 2 ครั้ง
- แสดงข้อความบน LCD

```text
 ACCESS DENIED!
 INVALID CARD
```

จากนั้นกลับไปหน้า WELCOME

---

## ⏱ Auto Lock System

เวลาปลดล็อกถูกกำหนดไว้ที่:

```cpp
const unsigned long UNLOCK_TIME = 5000;
```

ค่า `5000` หมายถึง:

```text
5000 milliseconds = 5 seconds
```

หากต้องการปลดล็อก 10 วินาที สามารถเปลี่ยนเป็น:

```cpp
const unsigned long UNLOCK_TIME = 10000;
```

---

## 🔄 Non-Blocking Countdown

ระบบใช้ `millis()` สำหรับนับเวลาแทนการใช้:

```cpp
delay(5000);
```

ข้อดีคือ Arduino ยังสามารถทำงานอื่นได้ระหว่างนับเวลา เช่น:

- อ่าน RFID
- อ่านปุ่ม LOCK
- อ่านปุ่ม UNLOCK
- อัปเดต LCD
- ตรวจสอบ Auto Lock

ทำให้ระบบตอบสนองได้ตลอดเวลา

---

## 🔘 Manual LOCK Button

เมื่อกดปุ่ม LOCK:

```text
D5 = HIGH
```

ระบบจะ LOCK ทันที

แม้ว่าขณะนั้นระบบกำลังนับถอยหลัง 5 วินาทีอยู่ก็ตาม

ตัวอย่าง:

```text
UNLOCK
5 sec
4 sec
```

หากกด LOCK:

```text
LOCK
```

ทันทีโดยไม่ต้องรอให้ครบ 5 วินาที

---

## 🔓 Manual UNLOCK Button

เมื่อกดปุ่ม UNLOCK:

```text
D4 = HIGH
```

ระบบจะ:

1. UNLOCK
2. เริ่มนับเวลา 5 วินาที
3. LCD แสดง Countdown
4. LOCK อัตโนมัติเมื่อครบเวลา

หากกด UNLOCK ซ้ำระหว่างที่กำลังปลดล็อก:

```text
5 sec
4 sec
3 sec
```

ระบบจะเริ่มนับใหม่:

```text
5 sec
```

---

## 💳 RFID During Unlock Countdown

ระหว่างที่ประตูกำลัง UNLOCK ระบบยังสามารถอ่าน RFID ได้

ตัวอย่าง:

```text
UNLOCK
Lock in: 3 sec
```

หากแตะบัตรที่ได้รับอนุญาตอีกครั้ง ระบบจะเริ่มนับใหม่:

```text
UNLOCK
Lock in: 5 sec
```

---

## 🔑 Authorized RFID Card

บัตรที่กำหนดไว้ในระบบปัจจุบัน:

### HEX

```text
C0 58 BE 13
```

### DEC

```text
192 88 190 19
```

ในโค้ดจะเขียนเป็น:

```cpp
const byte ALLOW_LIST[][4] = {

  {192, 88, 190, 19},

};
```

---

## ➕ How to Add New RFID Card

### Step 1

เชื่อมต่อ Arduino UNO กับคอมพิวเตอร์

---

### Step 2

เปิด Arduino IDE และเปิด:

```text
Tools -> Serial Monitor
```

ตั้ง Baud Rate เป็น:

```text
9600
```

---

### Step 3

แตะบัตร RFID ที่ต้องการเพิ่ม

Serial Monitor จะแสดงประมาณ:

```text
UID HEX = C0 58 BE 13
UID DEC = 192 88 190 19
```

---

### Step 4

นำค่า DEC ไปเพิ่มใน `ALLOW_LIST`

ตัวอย่างเพิ่ม 3 ใบ:

```cpp
const byte ALLOW_LIST[][4] = {

  {192, 88, 190, 19},     // Card 1
  {12, 34, 56, 78},       // Card 2
  {100, 120, 130, 140}    // Card 3

};
```

ไม่จำเป็นต้องแก้ค่า `ALLOW_COUNT` เพราะโปรแกรมจะคำนวณจำนวนบัตรให้อัตโนมัติ:

```cpp
const byte ALLOW_COUNT =
  sizeof(ALLOW_LIST) / sizeof(ALLOW_LIST[0]);
```

---

## 🖥 Serial Monitor

ตั้ง Serial Monitor ที่:

```text
9600 baud
```

ตัวอย่างเมื่อแตะบัตร:

```text
----------------------------
UID HEX = C0 58 BE 13
UID DEC = 192 88 190 19
ACCESS = GRANTED
DOOR = UNLOCK
```

เมื่อครบ 5 วินาที:

```text
AUTO LOCK AFTER 5 SEC
DOOR = LOCK
```

หากเป็นบัตรที่ไม่ได้รับอนุญาต:

```text
UID HEX = XX XX XX XX
UID DEC = XXX XXX XXX XXX
ACCESS = DENIED
```

---

## 📚 Required Arduino Libraries

โปรเจกต์นี้ใช้ Library ดังต่อไปนี้:

### MFRC522

```cpp
#include <MFRC522.h>
```

สามารถติดตั้งผ่าน Arduino IDE:

```text
Sketch
-> Include Library
-> Manage Libraries
```

ค้นหา:

```text
MFRC522
```

---

### LiquidCrystal I2C

```cpp
#include <LiquidCrystal_I2C.h>
```

ค้นหาใน Library Manager:

```text
LiquidCrystal I2C
```

---

### SPI

```cpp
#include <SPI.h>
```

เป็น Library มาตรฐานของ Arduino

---

### Wire

```cpp
#include <Wire.h>
```

เป็น Library มาตรฐานของ Arduino

---

## 📋 System Logic

| Event | Result |
|---|---|
| Power ON | 🔒 LOCK |
| Authorized RFID Card | 🔓 UNLOCK 5 sec |
| Unauthorized RFID Card | ❌ ACCESS DENIED |
| Press UNLOCK | 🔓 UNLOCK 5 sec |
| Press LOCK | 🔒 LOCK immediately |
| Unlock timeout | 🔒 AUTO LOCK |
| Authorized card during countdown | 🔄 Restart 5 sec |
| UNLOCK button during countdown | 🔄 Restart 5 sec |
| LOCK button during countdown | 🔒 LOCK immediately |

---

## 🔒 Default Safety State

เมื่อ Arduino เปิดเครื่อง ระบบจะกำหนดประตูเป็น:

```text
LOCK
```

ก่อนเริ่มต้น RFID และ LCD

ตัวอย่าง Logic:

```cpp
digitalWrite(RELAY, RELAY_LOCK);
```

โดย:

```cpp
#define RELAY_LOCK LOW
```

ดังนั้นระบบจะเริ่มต้นในสถานะ LOCK เสมอตาม Logic ของฮาร์ดแวร์ชุดนี้

---

## 🛠 Troubleshooting

### RFID อ่านบัตรไม่ได้

ตรวจสอบ:

- MFRC522 ใช้ไฟ 3.3V
- SDA ต่อ D10
- RST ต่อ D9
- MOSI ต่อ D11
- MISO ต่อ D12
- SCK ต่อ D13
- GND ต้องต่อร่วมกัน

---

### LCD ไม่แสดงผล

ตรวจสอบ:

```text
SDA -> A4
SCL -> A5
```

และตรวจสอบ I2C Address

ค่าปัจจุบัน:

```text
0x27
```

---

### Relay / SSR ทำงานกลับด้าน

หากระบบจริงทำงานตรงข้าม เช่น:

```text
LOW  = UNLOCK
HIGH = LOCK
```

ให้สลับค่า:

```cpp
#define RELAY_LOCK   HIGH
#define RELAY_UNLOCK LOW
```

---

### ปุ่ม LOCK / UNLOCK ทำงานสลับกัน

ตรวจสอบว่าสายต่อถูกต้อง:

```text
UNLOCK -> D4
LOCK   -> D5
```

---

### RFID หลุดหรืออ่านไม่ได้เมื่อกลอนทำงาน

กลอนแม่เหล็กและวงจร Relay อาจสร้าง Noise รบกวน Arduino หรือ MFRC522

แนวทางแก้ไข:

- แยกสายไฟกลอนออกจากสายสัญญาณ RFID
- ใช้ Power Supply ที่มีกระแสเพียงพอ
- ต่อ GND ให้เหมาะสม
- เพิ่ม Capacitor ใกล้ Arduino / MFRC522
- หากใช้ Relay กับโหลด DC แบบขดลวด ควรมีวงจรป้องกันแรงดันย้อนกลับตามชนิดวงจร
- หลีกเลี่ยงการเดินสาย MFRC522 ใกล้สายกำลังของ Magnetic Lock

---

## ⚠️ Security Note

ระบบนี้ตรวจสอบสิทธิ์จาก **UID ของบัตร RFID**

เหมาะสำหรับ:

- โปรเจกต์ Arduino
- ระบบทดลอง
- ห้อง Workshop
- ตู้เก็บอุปกรณ์
- ระบบควบคุมการเข้าออกที่ไม่ต้องการ Security ระดับสูง

UID เพียงอย่างเดียวไม่ควรถูกใช้เป็นระบบรักษาความปลอดภัยระดับสูง เนื่องจากบัตร RFID บางประเภทสามารถจำลองหรือคัดลอก UID ได้

---

## 🚀 Future Improvements

สามารถพัฒนาระบบเพิ่มเติมได้ เช่น:

- เพิ่ม Admin Card
- เพิ่มโหมด Add / Delete Card
- บันทึก UID ลง EEPROM
- เพิ่ม RTC สำหรับบันทึกเวลาเข้าออก
- เพิ่ม SD Card สำหรับเก็บ Log
- เพิ่ม ESP32
- เชื่อมต่อ Wi-Fi
- แจ้งเตือนผ่าน LINE / Telegram
- เชื่อมต่อ Home Assistant
- เพิ่ม Keypad
- เพิ่ม Fingerprint Sensor
- เพิ่ม Door Sensor
- เพิ่ม Exit Button
- เพิ่มระบบตรวจสอบว่าประตูปิดจริงหรือไม่
- เพิ่มระบบ Web Dashboard

---

## 👨‍💻 Author

Developed by **MengDIY**

Arduino RFID Door Lock System

---

## 📄 License

This project is intended for educational and DIY purposes.

You are free to study, modify, and improve the project for your own use.

---

⭐ หากโปรเจกต์นี้มีประโยชน์ สามารถกด **Star** Repository เพื่อสนับสนุนโปรเจกต์ได้
