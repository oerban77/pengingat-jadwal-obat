/*
  Pengingat Jadwal Obat — ESP32 + RTC DS3231 + LCD I2C + buzzer
  ==============================================================
  Jam minum obat tiba -> buzzer berbunyi, nama obat tampil di LCD,
  tekan tombol untuk menandai sudah diminum. Kalau 60 detik tidak
  ditekan, alat kirim pengingat ke Telegram (opsional).

  Wiring:
  - RTC DS3231 (I2C): SDA -> GPIO21, SCL -> GPIO22, VCC 3V3, GND -> GND
  - LCD 16x2 I2C (0x27): SDA -> GPIO21, SCL -> GPIO22, VCC 5V, GND -> GND
  - Buzzer aktif HIGH: GPIO26
  - LED hijau (220 ohm): GPIO32
  - Tombol akui: GPIO14 ke GND (INPUT_PULLUP, tekan = LOW)

  Library (wajib, ada di folder libraries/ repo ini):
  - RTClib (Adafruit) — jam DS3231
  - LiquidCrystal_I2C — LCD 16x2 I2C
  - UniversalTelegramBot 1.3.0 + ArduinoJson 6.21.5 — hanya bila USE_TELEGRAM = 1
    (ArduinoJson WAJIB v6, v7 gagal compile dengan UniversalTelegramBot)

  Isi WIFI_SSID, WIFI_PASSWORD, BOT_TOKEN dan CHAT_ID di bawah bila
  USE_TELEGRAM = 1. Bila 0, alat tetap bekerja penuh tanpa WiFi.
  Bot Telegram: buat lewat @BotFather, CHAT_ID dari @userinfobot.
*/

#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

#define USE_TELEGRAM 1

#if USE_TELEGRAM
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

const char *WIFI_SSID     = "NAMA_WIFI_KAMU";
const char *WIFI_PASSWORD = "PASSWORD_WIFI_KAMU";
const String BOT_TOKEN    = "TOKEN_BOT_TELEGRAM_KAMU";
const String CHAT_ID      = "CHAT_ID_KAMU";
#endif

// ---- Pin ----
const int PIN_BUZZER   = 26;  // buzzer aktif HIGH
const int PIN_LED      = 32;  // LED hijau, 220 ohm ke GND
const int PIN_TOMBOL   = 14;  // tombol ke GND, INPUT_PULLUP

// ---- Jadwal obat (jam, menit, nama) — ubah sesuai kebutuhan ----
struct Jadwal {
  uint8_t jam;
  uint8_t menit;
  const char *nama;
};

const Jadwal jadwal[] = {
  {  7,  0, "OBAT PAGI"  },
  { 13,  0, "OBAT SIANG" },
  { 19,  0, "OBAT MALAM" },
};
const int JUMLAH_JADWAL = sizeof(jadwal) / sizeof(jadwal[0]);
bool sudahBunyi[JUMLAH_JADWAL] = { false };

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

#if USE_TELEGRAM
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
#endif

void kirimTelegram(const String &pesan) {
#if USE_TELEGRAM
  if (WiFi.status() != WL_CONNECTED) return;
  bot.sendMessage(CHAT_ID, pesan, "");
#endif
}

void tampilkanJam(const DateTime &t) {
  char buf[9];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.hour(), t.minute(), t.second());
  lcd.setCursor(0, 0);
  lcd.print("Jam ");
  lcd.print(buf);
  lcd.print("    ");
  // Jadwal berikutnya
  lcd.setCursor(0, 1);
  for (int i = 0; i < JUMLAH_JADWAL; i++) {
    int menitSekarang = t.hour() * 60 + t.minute();
    int menitJadwal = jadwal[i].jam * 60 + jadwal[i].menit;
    if (menitJadwal > menitSekarang) {
      char b2[17];
      snprintf(b2, sizeof(b2), "Next %02d:%02d", jadwal[i].jam, jadwal[i].menit);
      lcd.print(b2);
      lcd.print("        ");
      return;
    }
  }
  lcd.print("Selesai hari ini");
}

void bunyikanAlarm(int idx) {
  const Jadwal &j = jadwal[idx];
  sudahBunyi[idx] = true;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WAKTUNYA MINUM!");
  lcd.setCursor(0, 1);
  lcd.print(j.nama);
  digitalWrite(PIN_LED, HIGH);

  // Bunyi 60 detik atau sampai tombol ditekan
  unsigned long mulai = millis();
  bool ditekan = false;
  while (millis() - mulai < 60000UL) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(250);
    digitalWrite(PIN_BUZZER, LOW);
    delay(750);
    if (digitalRead(PIN_TOMBOL) == LOW) {
      delay(50);
      if (digitalRead(PIN_TOMBOL) == LOW) { ditekan = true; break; }
    }
  }
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(j.nama);
  lcd.setCursor(0, 1);
  if (ditekan) {
    lcd.print("Sudah diminum :)");
    kirimTelegram(String("Sudah minum ") + j.nama);
  } else {
    lcd.print("Belum diminum!");
    kirimTelegram(String("Pengingat: waktunya minum ") + j.nama);
  }
  delay(3000);
  lcd.clear();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TOMBOL, INPUT_PULLUP);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED, LOW);

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Pengingat Obat");
  lcd.setCursor(0, 1);
  lcd.print("Siap...");

  if (!rtc.begin()) {
    Serial.println("RTC tidak ditemukan!");
    lcd.clear();
    lcd.print("RTC ERROR");
    while (1) delay(1000);
  }
  if (rtc.lostPower()) {
    // RTC kehabisan baterai: set jam dari waktu compile sketch
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

#if USE_TELEGRAM
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000UL) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " OK" : " gagal (lanjut tanpa WiFi)");
  client.setInsecure();
#endif

  delay(1500);
  lcd.clear();
}

void loop() {
  DateTime sekarang = rtc.now();
  for (int i = 0; i < JUMLAH_JADWAL; i++) {
    if (sekarang.hour() == jadwal[i].jam &&
        sekarang.minute() == jadwal[i].menit &&
        !sudahBunyi[i]) {
      bunyikanAlarm(i);
    }
  }
  // Reset penanda tiap tengah malam supaya jadwal besok bunyi lagi
  if (sekarang.hour() == 0 && sekarang.minute() == 0) {
    for (int i = 0; i < JUMLAH_JADWAL; i++) sudahBunyi[i] = false;
  }
  tampilkanJam(sekarang);
  delay(1000);
}
