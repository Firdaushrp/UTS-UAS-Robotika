# Ekosistem Eksperimen Kit iMCLab ESP32

Dokumen ini berisi dokumentasi dan riset eksperimen praktikum robotika menggunakan **Kit iMCLab** berbasis **ESP32**. Eksperimen ini mencakup sistem kontrol dasar motor DC, modulasi kecepatan PWM, integrasi platform Blynk IoT, hingga pembaruan _firmware_ nirkabel secara _Over-The-Air_ (OTA).

---

## Arsitektur Kit iMCLab

Pemrograman dasar robotika pada Kit iMCLab berfokus pada pengkonfigurasian mikrokontroler ESP32 untuk mengendalikan komponen aktuator dan indikator secara langsung melalui sinyal digital maupun modulasi analog.

Terdapat tiga komponen utama yang saling terhubung:

1. **Mikrokontroler (ESP32)**: Otak pemroses sinyal kendali, PWM, dan protokol komunikasi.
2. **Driver Motor DC**: Penggerak daya motor DC H-Bridge (`GPIO 27`, `GPIO 26`, dan `GPIO 12`).
3. **Indikator LED**: Indikator visual aktivitas sistem (`GPIO 2`).

---

## Eksperimen 1 — Kontrol On/Off Motor DC & Indikator LED via Serial

Eksperimen pertama mengimplementasikan komunikasi serial _asynchronous_ dua arah antara Python (sebagai pengendali/UI) dan ESP32 (sebagai mikrokontroler) pada _baud rate_ `115200 bps`.

### 1. Auto-Detect Port Serial

Skrip Python melakukan pemindaian port serial aktif secara otomatis menggunakan algoritma skor kriteria pembobotan _Vendor ID_ (`VID:PID 10C4:EA60` - CP210x) serta pencocokan kata kunci _driver_. Sistem secara presisi memilih port ESP32 (`COM3`) dan mengeliminasi port Bluetooth aktif (`COM4` dan `COM6`).

### 2. Logika Perintah & Analisis Respon

- **Input `'1'`**: Mengaktifkan LED (`GPIO 2 = HIGH`) dan menjalankan motor dengan frekuensi PWM `30 kHz` pada _duty cycle_ konstan `205`.
- **Input `'0'`**: Mematikan LED (`GPIO 2 = LOW`) dan menghentikan motor (_duty cycle_ = `0`).

**Analisis Hasil**:
Eksekusi otomatis memberikan transisi delay `3 detik` (ON) dan `1 detik` (OFF) tanpa adanya _garbage character_. Pembacaan data per karakter via `Serial.read()` memiliki karakteristik _low latency_, namun terbatas pada perintah biner tunggal tanpa parameter dinamis.

---

## Eksperimen 2 — Kontrol PWM & Kecepatan Motor Variable via Protocol Framing

Eksperimen ini meningkatkan skema komunikasi serial dari karakter tunggal menjadi _string buffer parsing_ berbasis _newline separator_ (`\n`), sehingga memungkinkan pengiriman perintah dinamis berparameter.

### 1. Protokol Perintah

- **`1` / `0`**: Motor ON (_default duty_ = 200) / Motor OFF.
- **`+`**: Menambah kecepatan/duty cycle sebesar `10` poin (rentang aman `0–255`).
- **`-`**: Mengurangi kecepatan/duty cycle sebesar `10` poin.
- **`PWM:xxx` / `pxxx`**: Menentukan nilai PWM secara langsung (contoh: `p150` atau `p200`).

### 2. Analisis Respon PWM

Transisi kecepatan berjalan bertahap ($200 \rightarrow 210 \rightarrow 220 \rightarrow 230 \rightarrow 220 \rightarrow 210 \rightarrow 200$) maupun perubahan langsung instan (`p150`, `p100`). Penerapan logika _clamping_ (`DUTY_MIN` dan `DUTY_MAX`) di sisi C++ menjamin nilai PWM tidak mengalami _integer overflow/underflow_.

---

## Eksperimen 3 — iMCLab Berbasis Blynk IoT

Mengintegrasikan Kit iMCLab dengan platform **Blynk IoT** untuk kontrol daya motor DC jarak jauh serta pemantauan kecepatan putar (**RPM**) secara _real-time_ via Wi-Fi.

### 1. Sensor RPM & Interrupts

Pembacaan pulsa menggunakan sensor _rotary encoder_ berbasis _Interrupt Service Routine_ (`IRAM_ATTR isr()`) pada `GPIO 13`. Teknik _software debouncing_ mikrodetik diterapkan untuk mengeliminasi _bounce noise_ saat piringan berputar cepat. Nilai RPM dihitung setiap 1 detik secara _non-blocking_ dan dikirimkan ke cloud.

### 2. Pemetaan Virtual Pins (Datastreams)

| Virtual Pin | Nama Datastream | Tipe Data | Fungsi Utama                                              |
| :---------: | :-------------: | :-------: | :-------------------------------------------------------- |
|  **`V0`**   |       RPM       | _Double_  | Display pembacaan kecepatan putar motor pada Widget Gauge |
|  **`V1`**   |       PWM       | _Integer_ | Indikator _duty cycle_ aktif saat ini pada Widget Gauge   |
|  **`V2`**   |    PWM Motor    | _Integer_ | Kontrol _duty cycle_ dinamis menggunakan Widget Slider    |
|  **`V3`**   |     Switch      | _Integer_ | Tombol sakelar digital untuk perintah ON/OFF              |

### 3. Analisis Dual-Control Sync

Sistem menerapkan sinkronisasi dua arah (_bi-directional control_). Setiap perubahan variabel baik melalui Dashboard Blynk maupun Serial Monitor USB akan saling memperbarui status secara otomatis.

---

## Eksperimen 4 — Pemrograman Over-The-Air (OTA) Berbasis Blynk Edgent

Eksperimen 4 menerapkan metode pembaruan _firmware_ nirkabel secara _Over-The-Air_ (OTA) menggunakan fitur **Blynk.Air** dan **Blynk.Inject** yang terintegrasi pada arsitektur Blynk Edgent.

### 1. Wi-Fi Provisioning (Blynk.Inject)

Pengunggahan _firmware_ dasar (v`0.1.0`) mengaktifkan mode Access Point sementara (`Blynk OTABlynk-HW2R`). Pengguna dapat mengonfigurasi kredensial Wi-Fi lokal melalui aplikasi Blynk di smartphone tanpa perlu _hardcoding_ SSID/Password pada kode program.

### 2. OTA Firmware Update (Blynk.Air)

Pengembangan program utama dilakukan dengan menambahkan logika kontrol PWM dan dikompilasi menjadi berkas biner (`Edgent_ESP32.ino.bin`) berukuran `957 KB` dengan nomor versi `0.1.1`. Berkas diunggah melalui _Blynk Console (Developer Zone / Shipping)_.

### 3. Analisis Keandalan OTA

- **Keamanan Data**: Proses update diverifikasi secara otomatis menggunakan _MD5 Checksum_ (`7FBC59FC12A6E7E073828358B8B6A0E4`).
- **Optimasi Sistem**: Penggunaan variabel _flag_ (`motorStateChanged`), penanganan kompatibilitas versi _core_ ESP32 LEDC (`ledcAttach` vs `ledcSetup`), serta pewaktu _non-blocking_ (`millis()`) memastikan eksekusi PWM dan koneksi `BlynkEdgent.run()` tetap stabil selama proses pengunduhan _firmware_ nirkabel.
