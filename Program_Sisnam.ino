#include <EEPROM.h>     // Kita akan membaca dan menulis UID PICC dari/ke EEPROM
#include <SPI.h>        // Modul RC522 menggunakan protokol SPI
#include <MFRC522.h>    // Perpustakaan untuk Perangkat Mifare RC522

#define COMMON_ANODE

#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif

#define redLed 7        // Tetapkan Pin Led
#define whiteLed 6
#define buzzer 5
const int starter = 8;
#define kontak 4        // Tetapkan Pin kontak

boolean match = false;           // Inisialisasi pencocokan kartu menjadi false
boolean programMode = false;     // Inisialisasi mode pemrograman menjadi false
boolean replaceMaster = false;

uint8_t successRead;             // Variabel integer untuk menyimpan apakah kita berhasil membaca dari pembaca
byte storedCard[4];              // Menyimpan ID yang dibaca dari EEPROM
byte readCard[4];                // Menyimpan ID yang dibaca dari Modul RFID
byte masterCard[4];              // Menyimpan ID kartu master dari EEPROM

// Buat instance MFRC522.
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN); 

///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  // Konfigurasi Pin Arduino
  pinMode(redLed, OUTPUT);
  pinMode(whiteLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  // pinMode(wipeB, INPUT_PULLUP);   // Aktifkan resistor pull-up pin
  pinMode(kontak, OUTPUT);
  pinMode(starter, OUTPUT);
  // Hati-hati dengan cara rangkaian kontak saat mereset atau mematikan daya Arduino Anda
  digitalWrite(kontak, HIGH);    // Pastikan pintu terkunci
  digitalWrite(starter, HIGH);
  digitalWrite(redLed, LED_OFF);  // Pastikan led mati
  digitalWrite(whiteLed, LED_OFF);  // Pastikan led mati
  digitalWrite(buzzer, LED_OFF); // Pastikan led mati

  // Konfigurasi Protokol
  Serial.begin(9600);  // Inisialisasi komunikasi serial dengan PC
  SPI.begin();          // Hardware MFRC522 menggunakan protokol SPI
  mfrc522.PCD_Init();   // Inisialisasi Hardware MFRC522

  // Jika Anda mengatur Antenna Gain ke Max, ini akan meningkatkan jarak baca
  // mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  Serial.println(F("Contoh Kontrol Akses v0.1"));   // Untuk tujuan debugging
  ShowReaderDetails();  // Tampilkan detail PCD - Rincian Pembaca Kartu MFRC522

  //Periksa apakah kartu master sudah ditentukan, jika tidak biarkan pengguna memilih kartu master
  // Ini juga berguna untuk hanya mendefinisikan Kartu Master ulang
  // Anda dapat menyimpan rekaman EEPROM lainnya hanya menulis selain 143 ke alamat EEPROM 1
  // EEPROM alamat 1 harus menyimpan nomor ajaib yang adalah '143'
  if (EEPROM.read(1) != 143) {
    Serial.println(F("Kartu Master Belum Ditentukan"));
    Serial.println(F("Pindai PICC untuk Ditentukan sebagai Kartu Master"));
    do {
      successRead = getID();            // mengatur successRead menjadi 1 saat kita mendapatkan pembacaan dari pembaca jika tidak 0
      digitalWrite(buzzer, LED_ON);    // Visualisasikan Kartu Master yang perlu ditentukan
      delay(200);
      digitalWrite(buzzer, LED_OFF);
      delay(200);
    }
    while (!successRead);                  // Program tidak akan melanjutkan selama Anda belum mendapatkan pembacaan yang berhasil
    for ( uint8_t j = 0; j < 4; j++ ) {        // Loop 4 kali
      EEPROM.write( 2 + j, readCard[j] );  // Tulis UID PICC yang dipindai ke EEPROM, mulai dari alamat 3
    }
    EEPROM.write(1, 143);                  // Tulis ke EEPROM bahwa kita telah mendefinisikan Kartu Master
    Serial.println(F("Kartu Master Telah Ditentukan"));
  }
  Serial.println(F("-------------------"));
  Serial.println(F("UID Kartu Master"));
  for ( uint8_t i = 0; i < 4; i++ ) {          // Baca UID Kartu Master dari EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Tuliskan ke masterCard
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Semuanya Siap"));
  Serial.println(F("Menunggu PICC untuk dipindai"));
  cycleLeds();    // Semuanya siap mari berikan umpan balik pengguna dengan menggulirkan led
}

///////////////////////////////////////// Loop Utama ///////////////////////////////////
void loop () {
  do {
    successRead = getID();  // mengatur successRead menjadi 1 saat kita mendapatkan pembacaan dari pembaca jika tidak 0
    if (programMode) {
      cycleLeds();              // Mode Program bergulir melalui Merah Hijau Biru menunggu membaca kartu baru
    }
    else {
      normalModeOn();     // Mode normal, buzzer Power menyala, yang lainnya mati
    }
  }
  while (!successRead);   // program tidak akan melanjutkan selama Anda tidak mendapatkan pembacaan yang berhasil
  if (programMode) {
    if ( isMaster(readCard) ) { // Saat dalam mode program, cek Terlebih Dahulu Jika kartu master dipindai lagi untuk keluar dari mode program
      Serial.println(F("Kartu Master Dipindai"));
      Serial.println(F("Daftar ID Kartu yang Ditambahkan:"));
      listAddedIDs();
      Serial.println(F("Keluar dari Mode Program"));
      Serial.println(F("-----------------------------"));
      programMode = false;
      return;
    }
    else {
      if ( findID(readCard) ) { // Jika kartu yang dipindai dikenal, hapus kartu tersebut
        Serial.println(F("Saya tahu PICC ini, menghapus..."));
        deleteID(readCard);
        Serial.println("-----------------------------");
        Serial.println(F("Pindai PICC untuk MENAMBAH atau MENGHAPUS dari EEPROM"));
      }
      else {                    // Jika kartu yang dipindai tidak dikenal, tambahkan kartu tersebut
        Serial.println(F("Saya tidak tahu PICC ini, menambahkan..."));
        writeID(readCard);
        Serial.println(F("-----------------------------"));
        Serial.println(F("Pindai PICC untuk MENAMBAH atau MENGHAPUS dari EEPROM"));
      }
    }
  }
  else {
    if ( isMaster(readCard)) {    // Jika ID kartu yang dipindai cocok dengan ID Kartu Master - masuk mode program
      programMode = true;
      Serial.println(F("Halo - Masuk Mode Program"));
      uint8_t count = EEPROM.read(0);   // Baca Byte pertama EEPROM yang
      Serial.print(F("Saya punya "));     // menyimpan jumlah ID di EEPROM
      Serial.print(count);
      Serial.print(F(" rekaman di EEPROM"));
      Serial.println("");
      Serial.println(F("Pindai PICC untuk MENAMBAH atau MENGHAPUS dari EEPROM"));
      Serial.println(F("Pindai Kartu Master lagi untuk Keluar dari Mode Program"));
      Serial.println(F("-----------------------------"));
    }
    else {
      if ( findID(readCard) ) { // Jika tidak, lihat apakah kartu ada di EEPROM
        Serial.println(F("Selamat, kartu Anda terdaftar"));
        granted(5000);         // Buka kunci untuk 300 ms
      }
      else {      // Jika tidak, tunjukkan bahwa ID tidak valid
        Serial.println(F("Maaf, kartu Anda belum terdaftar"));
        denied();
      }
    }
  }
}

/////////////////////////////////////////  Akses Diberikan  ///////////////////////////////////
void granted ( uint16_t setDelay) {
  digitalWrite(buzzer, LED_OFF);   // Pastikan buzzer mati
  delay(100);
  digitalWrite(buzzer, LED_ON);  // Pastikan buzzer menyala
  delay(200);
  digitalWrite(buzzer, LED_OFF);   // Pastikan buzzer mati
  delay(100);
  digitalWrite(buzzer, LED_ON);  // Pastikan buzzer menyala
  delay(200);
  if(digitalRead(kontak)==HIGH) 
  { 
    digitalWrite(kontak,LOW); //hidupkan kontak
    delay(2000); //jeda 2 detik
    digitalWrite(starter,LOW); //starter
    delay(1000); //jeda starter
    digitalWrite(starter,HIGH); //starter mati
  }
  else
  { digitalWrite(kontak,HIGH); //matikan kontak
  }
}

///////////////////////////////////////// Akses Ditolak  ///////////////////////////////////
void denied() {
  digitalWrite(whiteLed, LED_OFF);  // Pastikan LED putih mati
  digitalWrite(buzzer, LED_OFF);   // Pastikan buzzer mati
  digitalWrite(redLed, LED_ON);   // Nyalakan led merah
  delay(1000);
}

///////////////////////////////////////// Dapatkan UID PICC ///////////////////////////////////
uint8_t getID() {
  // Persiapan untuk Membaca PICCs
  if (!mfrc522.PICC_IsNewCardPresent()) { // Jika ada PICC baru ditempatkan pada pembaca RFID, lanjutkan
    return 0;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {   // Karena ada PICC ditempatkan, dapatkan Serial dan lanjutkan
    return 0;
  }
  // Ada Mifare PICCs yang memiliki UID 4 byte atau 7 byte, berhati-hatilah jika menggunakan PICC 7 byte
  // Saya pikir kita seharusnya menganggap setiap PICC memiliki UID 4 byte
  // Sampai kita mendukung PICC 7 byte
  Serial.println(F("UID PICC yang Dipindai:"));
  for (uint8_t i = 0; i < 4; i++) {
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Berhenti membaca
  return 1;
}

void ShowReaderDetails() {
  // Dapatkan versi perangkat lunak MFRC522
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("Versi Perangkat Lunak MFRC522: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (tidak diketahui)"));
  Serial.println("");
  // Ketika 0x00 atau 0xFF dikembalikan, komunikasi mungkin gagal
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("PERINGATAN: Kegagalan komunikasi, apakah MFRC522 terhubung dengan benar?"));
    Serial.println(F("SISTEM DIHENTIKAN: Periksa koneksi."));
    // Visualisasikan bahwa sistem dihentikan
    digitalWrite(whiteLed, LED_OFF);  // Pastikan LED putih mati
    digitalWrite(buzzer, LED_OFF);   // Pastikan buzzer mati
    digitalWrite(redLed, LED_ON);   // Nyalakan LED merah
    while (true); // jangan lanjut
  }
}

///////////////////////////////////////// Siklus Leds (Mode Program) ///////////////////////////////////
void cycleLeds() {
  digitalWrite(buzzer, LED_OFF);   // Pastikan buzzer mati
  delay(100);
  digitalWrite(buzzer, LED_ON);  // Pastikan buzzer menyala
  delay(100);
  delay(200);
}

//////////////////////////////////////// Mode Normal Led  ///////////////////////////////////
void normalModeOn() {
  digitalWrite(buzzer, LED_OFF);   // Pastikan buzzer mati
  delay(2000);
  digitalWrite(buzzer, LED_ON);  // Pastikan buzzer menyala
  delay(1000);
  delay(1000);
  // digitalWrite(buzzer, LED_ON);  // buzzer ON dan siap membaca kartu
  // digitalWrite(redLed, LED_OFF);  // Pastikan LED merah mati
  // digitalWrite(whiteLed, LED_OFF);  // Pastikan LED putih mati
  // digitalWrite(kontak, HIGH);    // Pastikan pintu terkunci
}

//////////////////////////////////////// Baca ID dari EEPROM //////////////////////////////
void readID(uint8_t nomor) {
  uint8_t awal = (nomor * 4) + 2;    // Tentukan posisi awal
  for (uint8_t i = 0; i < 4; i++) {     // Ulangi 4 kali untuk mendapatkan 4 Byte
    storedCard[i] = EEPROM.read(awal + i);   // Berikan nilai yang dibaca dari EEPROM ke array
  }
}

///////////////////////////////////////// Tambahkan ID ke EEPROM   ///////////////////////////////////
void writeID(byte a[]) {
  if (!findID(a)) {     // Sebelum kita menulis ke EEPROM, periksa apakah kami pernah melihat kartu ini sebelumnya!
    uint8_t num = EEPROM.read(0);     // Dapatkan jumlah ruang yang digunakan, posisi 0 menyimpan jumlah kartu ID
    uint8_t awal = (num * 4) + 6;  // Tentukan di mana slot berikutnya dimulai
    num++;                // Tambahkan nilai penghitung satu
    EEPROM.write(0, num);     // Tulis hitungan baru ke penghitung
    for (uint8_t j = 0; j < 4; j++) {   // Ulangi 4 kali
      EEPROM.write(awal + j, a[j]);  // Tulis nilai array ke EEPROM pada posisi yang benar
    }
    suksesTulis();
    Serial.println(F("Berhasil menambahkan rekaman ID ke EEPROM"));
  } else {
    failedWrite();
    Serial.println(F("Gagal! Ada yang salah dengan ID atau EEPROM rusak"));
  }
}

///////////////////////////////////////// Hapus ID dari EEPROM   ///////////////////////////////////
void deleteID(byte a[]) {
  if (!findID(a)) {     // Sebelum kita menghapus dari EEPROM, periksa apakah kami memiliki kartu ini!
    failedWrite();      // Jika tidak
    Serial.println(F("Gagal! Ada yang salah dengan ID atau EEPROM rusak"));
  } else {
    uint8_t num = EEPROM.read(0);   // Dapatkan jumlah ruang yang digunakan, posisi 0 menyimpan jumlah kartu ID
    uint8_t slot;       // Tentukan nomor slot kartu
    uint8_t awal;      // = ( num * 4 ) + 6; // Tentukan di mana slot berikutnya dimulai
    uint8_t looping;    // Jumlah kali loop diulang
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Baca Byte pertama EEPROM yang menyimpan jumlah kartu
    slot = findIDSLOT(a);   // Tentukan nomor slot kartu untuk dihapus
    awal = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Kurangi penghitung satu
    EEPROM.write(0, num);   // Tulis hitungan baru ke penghitung
    for (j = 0; j < looping; j++) {         // Ulangi kartu bergeser kali
      EEPROM.write(awal + j, EEPROM.read(awal + 4 + j));   // Geser nilai array ke 4 tempat sebelumnya di EEPROM
    }
    for (uint8_t k = 0; k < 4; k++) {         // Loop pergeseran
      EEPROM.write(awal + j + k, 0);
    }
    successDelete();
    Serial.println(F("Berhasil menghapus rekaman ID dari EEPROM"));
  }
}

///////////////////////////////////////// Tampilkan Daftar ID yang Ditambahkan ///////////////////////////////////
void listAddedIDs() {
  uint8_t count = EEPROM.read(0);
  Serial.print(F("Jumlah ID yang Ditambahkan: "));
  Serial.println(count);
  Serial.println(F("ID Kartu:"));
  for (uint8_t i = 1; i <= count; i++) {    // Ulangi satu kali untuk setiap entri EEPROM
    readID(i);          // Baca ID dari EEPROM, disimpan di storedCard[4]
    Serial.print(F("UID: "));
    for (uint8_t j = 0; j < 4; j++) {   // Ulangi 4 kali
      Serial.print(storedCard[j], HEX);
    }
    Serial.println("");
  }
}

///////////////////////////////////////// Periksa Byte   ///////////////////////////////////
boolean checkTwo(byte a[], byte b[]) {
  if (a[0] != 0)      // Pastikan ada sesuatu dalam array pertama
    match = true;       // Anggap cocok pada awalnya
  for (uint8_t k = 0; k < 4; k++) {   // Ulangi 4 kali
    if (a[k] != b[k])     // JIKA a != b maka atur cocok = false, satu gagal, semua gagal
      match = false;
  }
  if (match) {      // Periksa apakah cocok masih benar
    return true;      // Kembalikan benar
  } else  {
    return false;       // Kembalikan salah
  }
}

///////////////////////////////////////// Temukan Slot   ///////////////////////////////////
uint8_t findIDSLOT(byte find[]) {
  uint8_t count = EEPROM.read(0);       // Baca Byte pertama EEPROM
  for (uint8_t i = 1; i <= count; i++) {    // Ulangi satu kali untuk setiap entri EEPROM
    readID(i);                // Baca ID dari EEPROM, disimpan di storedCard[4]
    if (checkTwo(find, storedCard)) {   // Periksa apakah storedCard yang dibaca dari EEPROM
      // sama dengan ID yang dilewatkan find[]
      return i;         // Nomor slot kartu
      break;          // Berhenti mencari kami menemukannya
    }
  }
}

///////////////////////////////////////// Temukan ID Dari EEPROM   ///////////////////////////////////
boolean findID(byte find[]) {
  uint8_t count = EEPROM.read(0);     // Baca Byte pertama EEPROM
  for (uint8_t i = 1; i <= count; i++) {    // Ulangi satu kali untuk setiap entri EEPROM
    readID(i);          // Baca ID dari EEPROM, disimpan di storedCard[4]
    if (checkTwo(find, storedCard)) {   // Periksa apakah storedCard yang dibaca dari EEPROM
      return true;
      break;  // Berhenti mencari kami menemukannya
    } else {    // Jika tidak, kembalikan false
    }
  }
  return false;
}

///////////////////////////////////////// Tulis Sukses ke EEPROM   ///////////////////////////////////
// Berkedip LED putih 3 kali untuk menunjukkan penulisan berhasil ke EEPROM
void suksesTulis() {
  digitalWrite(buzzer, LED_OFF);   // Pastikan buzzer mati
  digitalWrite(redLed, LED_OFF);  // Pastikan LED merah mati
  digitalWrite(whiteLed, LED_OFF);  // Pastikan LED putih menyala
  delay(200);
  digitalWrite(whiteLed, LED_ON);   // Pastikan LED putih menyala
  delay(200);
  digitalWrite(whiteLed, LED_OFF);  // Pastikan LED putih mati
  delay(200);
  digitalWrite(whiteLed, LED_ON);   // Pastikan LED putih menyala
  delay(200);
  digitalWrite(whiteLed, LED_OFF);  // Pastikan LED putih mati
  delay(200);
  digitalWrite(whiteLed, LED_ON);   // Pastikan LED putih menyala
  delay(200);
}

///////////////////////////////////////// Tulis Gagal ke EEPROM   ///////////////////////////////////
// Berkedip LED merah 3 kali untuk menunjukkan penulisan gagal ke EEPROM
void failedWrite() {
  digitalWrite(buzzer, LED_OFF);   // Pastikan buzzer mati
  digitalWrite(redLed, LED_OFF);  // Pastikan LED merah mati
  digitalWrite(whiteLed, LED_OFF);  // Pastikan LED putih mati
  delay(200);
  digitalWrite(redLed, LED_ON);   // Pastikan LED merah menyala
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Pastikan LED merah mati
  delay(200);
  digitalWrite(redLed, LED_ON);   // Pastikan LED merah menyala
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Pastikan LED merah mati
  delay(200);
  digitalWrite(redLed, LED_ON);   // Pastikan LED merah menyala
  delay(200);
}

///////////////////////////////////////// Sukses Hapus UID Dari EEPROM  ///////////////////////////////////
// Berkedip buzzer 3 kali untuk menunjukkan penghapusan berhasil ke EEPROM
void successDelete() {
  digitalWrite(buzzer, LED_OFF);   // Pastikan buzzer mati
  digitalWrite(redLed, LED_OFF);  // Pastikan LED merah mati
  digitalWrite(whiteLed, LED_OFF);  // Pastikan LED putih menyala
  delay(200);
  digitalWrite(whiteLed, LED_ON);   // Pastikan LED putih menyala
  delay(200);
  digitalWrite(whiteLed, LED_OFF);  // Pastikan LED putih mati
  delay(200);
  digitalWrite(whiteLed, LED_ON);   // Pastikan LED putih menyala
  delay(200);
  digitalWrite(whiteLed, LED_OFF);  // Pastikan LED putih mati
  delay(200);
  digitalWrite(whiteLed, LED_ON);   // Pastikan LED putih menyala
  delay(200);
}

////////////////////// Periksa readCard JIKA adalah Kartu masterCard   ///////////////////////////////////
// Periksa apakah ID yang dilewatkan adalah kartu pemrograman master
boolean isMaster(byte test[]) {
  if (checkTwo(test, masterCard))
    return true;
  else
    return false;
}

