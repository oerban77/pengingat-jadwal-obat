# Pengingat Jadwal Obat — ESP32 + RTC DS3231 + LCD

Jam minum obat tiba, buzzer berbunyi, nama obat tampil di LCD, dan kamu
tinggal tekan tombol untuk menandai sudah diminum. Kalau 60 detik tidak
ditekan, alat mengirim pengingat ke Telegram.

## Cara kerja

| Waktu | Yang terjadi |
|---|---|
| 07:00 / 13:00 / 19:00 | Buzzer bunyi (1 detik sekali), LCD menampilkan "WAKTUNYA MINUM!" + nama obat, LED hijau menyala |
| Tombol ditekan | Alarm berhenti, LCD "Sudah diminum :)", Telegram: "Sudah minum OBAT PAGI" |
| 60 detik tidak ditekan | Alarm berhenti, LCD "Belum diminum!", Telegram: "Pengingat: waktunya minum OBAT PAGI" |
| 00:00 | Penanda jadwal di-reset supaya besok bunyi lagi |

Jadwal obat bisa diubah di array `jadwal[]` di sketch (jam, menit, nama).
RTC DS3231 memakai baterai CR2032, jadi jam tetap jalan walau listrik mati.
Kalau baterai RTC habis, jam di-set otomatis dari waktu compile sketch.

## Wiring

| Komponen | Pin modul | ESP32 |
|---|---|---|
| RTC DS3231 SDA | SDA | GPIO21 |
| RTC DS3231 SCL | SCL | GPIO22 |
| RTC DS3231 VCC | VCC | 3V3 |
| LCD 16x2 I2C SDA | SDA | GPIO21 (bus I2C sama) |
| LCD 16x2 I2C SCL | SCL | GPIO22 (bus I2C sama) |
| LCD 16x2 I2C VCC | VCC | VIN 5V |
| Buzzer (aktif HIGH) | + | GPIO26 |
| LED hijau (220 Ω) | anoda | GPIO32 |
| Tombol akui | satu kaki | GPIO14 (INPUT_PULLUP, kaki lain ke GND) |
| Semua GND | GND | GND (digabung) |

Skema lengkap: [schematic.png](schematic.png)

## Library

Ada di folder `libraries/` (tinggal ekstrak ke folder libraries Arduino):

| Library | Versi | Keterangan |
|---|---|---|
| RTClib (Adafruit) | 2.1.4 | Jam DS3231 |
| LiquidCrystal_I2C | 1.1.2 | LCD 16x2 I2C (alamat 0x27) |
| UniversalTelegramBot | 1.3.0 | Hanya bila USE_TELEGRAM = 1 |
| ArduinoJson | 6.21.5 | WAJIB v6 — v7 gagal compile dengan UniversalTelegramBot |

## Telegram (opsional)

Set `USE_TELEGRAM` ke 1 di sketch, lalu isi `WIFI_SSID`, `WIFI_PASSWORD`,
`BOT_TOKEN` (buat lewat @BotFather), dan `CHAT_ID` (dari @userinfobot).
Bila 0, alat tetap bekerja penuh tanpa WiFi — cuma tanpa notifikasi HP.

## Compile

Terverifikasi 20 Nov 2026 — arduino-cli 1.6.0, ESP32 core 4.0.0:

```
Sketch uses 1092873 bytes (83%) of program storage space.
Global variables use 52032 bytes (15%) of dynamic memory.
```

## Lisensi

MIT — lihat [LICENSE](LICENSE).
