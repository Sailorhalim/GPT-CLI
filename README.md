GPT-Termux C++
Ini adalah versi C++ dari skrip klien OpenAI yang dirancang untuk berjalan secara efisien di Termux. Program ini menawarkan fungsionalitas yang sama dengan skrip Node.js aslinya tetapi dengan performa yang lebih baik dan penggunaan sumber daya yang lebih rendah.

Persyaratan
Anda perlu menginstal beberapa paket di Termux sebelum dapat mengompilasi program ini.

Clang: Kompiler C++ modern.

libcurl: Pustaka untuk melakukan permintaan HTTP.

openssl: Pustaka untuk enkripsi SSL/TLS yang dibutuhkan oleh libcurl.

nlohmann-json: Pustaka untuk mem-parsing dan membuat data JSON.

Git: Untuk mengkloning pustaka cpr.

Buka Termux dan jalankan perintah berikut:

pkg update && pkg upgrade
pkg install clang libcurl openssl nlohmann-json git

Instalasi Pustaka Tambahan (CPR)
Program ini menggunakan cpr, sebuah wrapper C++ modern untuk libcurl. Anda perlu mengunduhnya secara manual.

Kloning repositori cpr dari GitHub:

git clone [https://github.com/libcpr/cpr.git](https://github.com/libcpr/cpr.git)
cd cpr

Bangun dan instal cpr. Perintah berikut akan mengompilasi dan menginstal header serta file pustaka ke lokasi standar Termux ($PREFIX):

mkdir build && cd build
cmake .. -DCPR_USE_SYSTEM_CURL=ON -DCMAKE_INSTALL_PREFIX=$PREFIX
make -j$(nproc)
make install
cd ../..

Catatan: Proses ini mungkin memakan waktu beberapa menit.

Kompilasi
Setelah semua dependensi terinstal, Anda dapat mengompilasi file gpt.cpp.

Pastikan Anda berada di direktori yang sama dengan gpt.cpp.

Gunakan perintah clang++ berikut untuk mengompilasi:

clang++ gpt.cpp -o gpt-cpp -std=c++17 -lcurl -lcpr -lssl -lcrypto

Penjelasan flag:

gpt.cpp: File sumber Anda.

-o gpt-cpp: Menentukan nama file output yang dapat dieksekusi (executable).

-std=c++17: Menggunakan standar C++17.

-lcurl -lcpr: Menautkan pustaka libcurl dan cpr.

-lssl -lcrypto: (Penting) Menautkan pustaka OpenSSL yang dibutuhkan untuk koneksi HTTPS.

Pengaturan API Key
Program ini mengambil API Key OpenAI dari environment variable bernama OPENAI_API_KEY. Ini adalah cara yang lebih aman daripada menuliskannya langsung di kode.

Jalankan perintah ini di Termux untuk mengatur API key Anda (ganti sk-xxxx dengan key Anda yang sebenarnya):

export OPENAI_API_KEY="sk-xxxx"

Penting: Perintah export ini hanya berlaku untuk sesi terminal saat ini. Untuk membuatnya permanen, tambahkan baris di atas ke file konfigurasi shell Anda (~/.bashrc atau ~/.zshrc) dan mulai ulang Termux.

echo 'export OPENAI_API_KEY="sk-xxxx"' >> ~/.bashrc
source ~/.bashrc

Penggunaan
Setelah kompilasi berhasil, Anda akan memiliki file bernama gpt-cpp di direktori Anda. Untuk menjalankannya, cukup ketik:

./gpt-cpp

Program akan dimulai, dan Anda bisa langsung berinteraksi dengan asisten AI, sama seperti versi Node.js.
