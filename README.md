# Sistem Kontrol Akses dengan RFID untuk Aktivasi Motor
Proyek ini menyediakan sistem kontrol akses berbasis teknologi RFID. Sistem ini memungkinkan aktivasi mesin berdasarkan kartu RFID yang terdaftar. Terdapat kartu master untuk mengelola pendaftaran dan penghapusan kartu ID, serta memastikan akses aman bagi pengguna yang terotorisasi.

Fitur
  
  •	Kartu Master: Digunakan untuk pemrograman, pendaftaran, dan penghapusan ID kartu RFID.
  
  •	Mode Normal: Memberikan akses saat kartu yang terdaftar dipindai.
  
  •	Mode Pemrograman: Memungkinkan penambahan atau penghapusan kartu saat kartu master dipindai.
  
  •	Kontrol Relay: Mengaktifkan atau menonaktifkan sistem motor berdasarkan kartu yang terdaftar.
  
  •	Umpan Balik Visual dan Audio: Memberikan umpan balik melalui LED dan buzzer untuk tindakan yang berhasil atau gagal.

Cara Kerja
  1.	Inisialisasi:
      o	Menyiapkan konfigurasi pin awal untuk LED, buzzer, dan modul RFID (MFRC522).
      o	Mengonfigurasi variabel untuk mode program, ID kartu, dan lainnya.
  2.	Pendaftaran Kartu Master:
      o	Jika kartu master belum terdaftar, sistem akan meminta pengguna untuk memindai kartu.
      o	UID kartu master kemudian disimpan di EEPROM.
  3.	Loop Utama:
      o	Sistem terus memeriksa kartu RFID yang ditempatkan dekat pembaca.
      o	Jika kartu terdaftar dipindai di mode normal, sistem memberikan akses (mengaktifkan relay).
      o	Di mode pemrograman (diaktifkan dengan memindai kartu master), sistem memungkinkan penambahan/penghapusan kartu yang terdaftar.
  4.	Aktivasi Relay:
      o	Kartu yang terdaftar mengaktifkan motor dengan memicu relay, disertai umpan balik visual dan suara.
      o	Jika kartu yang tidak terdaftar dipindai, akses ditolak dengan sinyal kesalahan (LED merah dan buzzer).
  5.	Manajemen Kartu:
      o	Kartu dapat ditambahkan atau dihapus dengan memindainya di mode pemrograman.

Komponen
  •	Board Arduino
  •	Modul RFID MFRC522
  •	Modul Relay
  •	LED dan Buzzer
  •	EEPROM untuk menyimpan kartu yang terdaftar

Instruksi Pengaturan
  1.	Hubungkan modul RFID ke Arduino sesuai dengan konfigurasi pin yang ditentukan.
  2.	Unggah sketsa Arduino ke board Anda.
  3.	Nyalakan sistem dan ikuti instruksi di layar untuk mendaftarkan kartu master.
  4.	Gunakan kartu master untuk mengelola kartu yang terdaftar, dan pindai kartu yang terotorisasi untuk mengontrol akses.

Prosedur Pengujian
  1.	Setelah mengunggah kode, sistem akan diinisialisasi dan LED akan menunjukkan status perangkat.
  2.	Daftarkan kartu master dengan memindainya sekali.
  3.	Ketika kartu master dipindai, sistem akan masuk ke mode pemrograman, memungkinkan penambahan atau penghapusan kartu lain.
  4.	Di mode normal, tempelkan kartu terdaftar untuk mengaktifkan sistem motor. Relay akan menyala, ditandai dengan indikator relay yang mati dan suara buzzer berbunyi.
  5.	Untuk mematikan, tempelkan kartu terdaftar lagi, dan relay akan mati dengan suara penutupan.
  6.	Jika kartu yang tidak terdaftar dipindai, sistem akan menolak akses dengan LED merah dan suara buzzer.

Fungsi-Fungsi
  •	granted(): Fungsi yang memberikan akses dengan mengaktifkan relay.
  •	denied(): Fungsi yang menolak akses dan memberikan sinyal kesalahan dengan LED dan buzzer.
  •	getID(): Mengambil UID dari kartu yang dipindai.
  •	cycleLeds(): Mengedipkan LED untuk menunjukkan status sistem.
  •	normalModeOn(): Memasuki mode normal setelah memindai kartu yang valid.

