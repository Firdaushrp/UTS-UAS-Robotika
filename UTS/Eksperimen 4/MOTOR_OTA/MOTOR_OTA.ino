/*
  TEMPLATE DASAR: ESP32 + Blynk Edgent + OTA (Blynk.Air) + Kontrol Motor DC (PWM)
*/

// ============================ BAGIAN TETAP (identitas) ============================
#define BLYNK_TEMPLATE_ID "TMPL6HfqaAmxf"
#define BLYNK_TEMPLATE_NAME "OTABlynk"

// Naikkan nomor ini SETIAP kali membuat versi baru untuk OTA via Blynk.Air (0.1.1, 0.1.2, dst.)
#define BLYNK_FIRMWARE_VERSION "0.1.1"

#define BLYNK_PRINT Serial
#define APP_DEBUG


#include "BlynkEdgent.h"


// ====================== AREA ISI 1: PIN DAN VARIABEL ======================
const int ledPin     = 2;     // LED Indikator Onboard
const int motor1Pin1 = 27;    // Motor Direction 1
const int motor1Pin2 = 26;    // Motor Direction 2
const int enable1Pin = 12;    // Enable PWM Pin

// Konfigurasi PWM
const int freq       = 30000; // Frekuensi PWM 30kHz
const int resolution = 8;     // 8-bit (0 - 255)

int dutyCycle        = 200;   // Duty cycle default
bool isMotorOn       = false; // Status ON/OFF motor
bool motorStateChanged = true; // Flag pembaharuan status motor


// ====================== AREA ISI 2: LOGIKA UTAMA ======================
// Penerapan perintah motor ke hardware
void applyMotor() {
  if (isMotorOn && dutyCycle > 0) {
    digitalWrite(motor1Pin1, HIGH);
    digitalWrite(motor1Pin2, LOW);

    // Kompatibilitas ESP32 Arduino Core v2.x & v3.x
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcWrite(enable1Pin, dutyCycle);
#else
    ledcWrite(0, dutyCycle);
#endif

    digitalWrite(ledPin, HIGH); // LED Indikator Nyala
  } else {
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcWrite(enable1Pin, 0);
#else
    ledcWrite(0, 0);
#endif

    digitalWrite(ledPin, LOW);  // LED Indikator Mati
  }
}

void updateDevice() {
  // Hanya mengeksekusi jika terjadi perubahan dari Blynk untuk menghemat pemrosesan
  if (motorStateChanged) {
    applyMotor();
    motorStateChanged = false;
  }
}


// ====================== AREA ISI 3: TERIMA PERINTAH DARI BLYNK ======================
// V0: Button Switch ON/OFF Motor
BLYNK_WRITE(V0) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    isMotorOn = true;
    if (dutyCycle == 0) dutyCycle = 200;
    Serial.println("Blynk: Motor ON");
  } else {
    isMotorOn = false;
    Serial.println("Blynk: Motor OFF");
  }
  motorStateChanged = true;
}

// V1: Slider Kecepatan Motor (0 - 255)
BLYNK_WRITE(V1) {
  dutyCycle = param.asInt();
  if (dutyCycle < 0) dutyCycle = 0;
  if (dutyCycle > 255) dutyCycle = 255;

  Serial.print("Blynk: Kecepatan Motor Duty Cycle = ");
  Serial.println(dutyCycle);

  motorStateChanged = true;
}


// ====================== AREA ISI 4: KIRIM DATA KE DASHBOARD ======================
const unsigned long SEND_INTERVAL_MS = 1000;
unsigned long lastSend = 0;

void sendData() {
  // Mengirim umpan balik status duty cycle ke Datastream Virtual V2 (Opsional / Value Display)
  if (isMotorOn) {
    Blynk.virtualWrite(V2, dutyCycle);
  } else {
    Blynk.virtualWrite(V2, 0);
  }
}


// ============================ BAGIAN TETAP (koneksi) ============================
BLYNK_CONNECTED() {
  Blynk.syncAll();
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // ====================== AREA ISI 5: INISIALISASI TAMBAHAN ======================
  pinMode(ledPin, OUTPUT);
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);

  // Inisialisasi PWM LEDC ESP32 (Universal)
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcAttach(enable1Pin, freq, resolution);
#else
  ledcSetup(0, freq, resolution);
  ledcAttachPin(enable1Pin, 0);
#endif

  applyMotor(); // Pastikan motor mati saat booting

  Serial.println("Mulai - versi " BLYNK_FIRMWARE_VERSION);
  BlynkEdgent.begin();                         // TETAP
}

void loop() {
  BlynkEdgent.run();                           // TETAP: menjaga koneksi dan OTA (Blynk.Air)
  updateDevice();                              // logika utama motor (AREA ISI 2)

  // Kirim data berkala tanpa delay() (TETAP)
  if (millis() - lastSend >= SEND_INTERVAL_MS) {
    lastSend = millis();
    if (Blynk.connected()) sendData();
  }
}
