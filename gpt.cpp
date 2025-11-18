// 32 171 264
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <regex>
#include <csignal>

// Library pihak ketiga untuk HTTP requests dan JSON
// Pastikan Anda menginstalnya sesuai petunjuk di README.md
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// --- Deklarasi Global ---

bool saveLog = false;
std::string logPath;
std::vector<std::string> contentHistory; // Untuk redraw saat resize
json konteks;
json token = {
    {"prompt_tokens", 0},
    {"completion_tokens", 0},
    {"model", "gpt-4o"}
};
std::string apiKey;

// --- Definisi Warna ANSI ---
namespace Color {
    const std::string red = "\x1b[31;1m";
    const std::string green = "\x1b[32;1m";
    const std::string yellow = "\x1b[33;1m";
    const std::string blue = "\x1b[34;1m";
    const std::string magenta = "\x1b[35m";
    const std::string cyan_bold = "\x1b[1;36m";
    const std::string white_bold = "\x1b[1;37m";
    const std::string blue_bg = "\x1b[94;1m";
    const std::string magenta_bg = "\x1b[45;1m";
    const std::string reset = "\x1b[0m";
}

// --- Prototipe Fungsi ---
void gpt(const std::string& txt);
void redrawTerminal();
void handle_resize(int sig);
std::string exec(const char* cmd);


// --- Implementasi Fungsi ---

// Fungsi untuk menjalankan perintah shell dan mendapatkan outputnya
std::string exec(const char* cmd) {
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            result += buffer;
    }
    return result;
}

// Fungsi untuk menyimpan log ke file
void log(const std::string& text) {
    contentHistory.push_back(text);
    if (saveLog) {
        std::ofstream logFile(logPath, std::ios_base::app);
        if (logFile.is_open()) {
            logFile << text;
        } else {
            std::cerr << "\n" << Color::red << "[Gagal menyimpan log]" << Color::reset << std::endl;
        }
    }
}

// Fungsi untuk menjalankan perintah shell dari input pengguna
void sh(const std::string& command) {
    std::cout << Color::blue << "$ " << command << Color::reset << std::endl;
    std::string output = exec(command.c_str());
    // Hapus newline di akhir jika ada
    if (!output.empty() && output[output.length()-1] == '\n') {
        output.erase(output.length()-1);
    }
    std::cout << output << std::endl;
}

// Fungsi untuk memberikan efek mengetik
void typo(const std::string& text) {
    for (char c : text) {
        std::cout << c << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

// Fungsi untuk menghitung biaya penggunaan API
double gptUsage() {
    std::string model = token["model"];
    json prices = {
        {"gpt-4o", {{"input", 5.00}, {"output", 15.00}}},
        {"gpt-4", {{"input", 30.00}, {"output", 60.00}}},
        {"gpt-3.5-turbo", {{"input", 0.50}, {"output", 1.50}}}
    };

    std::string modelPriceKey = "gpt-4o";
    for (auto const& [key, val] : prices.items()) {
        if (model.find(key) != std::string::npos) {
            modelPriceKey = key;
            break;
        }
    }

    double inputPrice = prices[modelPriceKey]["input"];
    double outputPrice = prices[modelPriceKey]["output"];

    double inputCost = (token["prompt_tokens"].get<double>() / 1000000.0) * inputPrice;
    double outputCost = (token["completion_tokens"].get<double>() / 1000000.0) * outputPrice;

    return inputCost + outputCost;
}

// Fungsi untuk memformat output dari bot dengan warna
std::string pre(std::string x) {
    // Memberi warna pada blok kode ```bahasa ... ```
    x = std::regex_replace(x, std::regex("```(\\w*)\\n([\\s\\S]+?)```"),
        "\n" + Color::magenta_bg + " $1 " + Color::reset + "\n" + Color::magenta + "$2" + Color::reset + "\n");

    // Memberi warna pada kode inline `...`
    x = std::regex_replace(x, std::regex("`([^`]+?)`"), Color::yellow + "$1" + Color::reset);

    // Memberi warna pada teks tebal **...**
    x = std::regex_replace(x, std::regex("\\*\\*([^\\*]+?)\\*\\*"), Color::cyan_bold + "$1" + Color::reset);

    // Memberi warna pada heading ### ...
    x = std::regex_replace(x, std::regex("^###\\s*(.*)", std::regex_constants::multiline), Color::green + "--- $1 ---" + Color::reset);

    return x;
}

// Handler untuk sinyal resize terminal
void handle_resize(int sig) {
    if (sig == SIGWINCH) {
        redrawTerminal();
    }
}

// Fungsi untuk menggambar ulang seluruh riwayat chat
void redrawTerminal() {
    // Bersihkan layar
    std::cout << "\033[2J\033[1;1H";
    // Tulis ulang riwayat
    for (const auto& line : contentHistory) {
        std::cout << line;
    }
    // Tampilkan prompt lagi
    std::cout << Color::green << "\nKamu" << Color::reset << ": " << std::flush;
}

// Fungsi utama untuk berinteraksi dengan OpenAI API
void gpt(const std::string& txt) {
    konteks.push_back({{"role", "user"}, {"content", txt}});

    json data = {
        {"model", token["model"]},
        {"messages", konteks},
        {"max_tokens", 2048},
        {"temperature", 0.8}
    };

    auto startTime = std::chrono::high_resolution_clock::now();

    cpr::Response r = cpr::Post(cpr::Url{"https://api.openai.com/v1/chat/completions"},
                                cpr::Body{data.dump()},
                                cpr::Header{{"Content-Type", "application/json"},
                                            {"Authorization", "Bearer " + apiKey}});

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;

    if (r.status_code != 200) {
        std::cerr << "\n" << Color::red << "[Request Error] " << Color::reset << "Status code: " << r.status_code << std::endl;
        std::cerr << r.text << std::endl;
        konteks.erase(konteks.size() - 1); // Hapus pertanyaan terakhir jika gagal
        return;
    }

    try {
        json response = json::parse(r.text);

        if (response.contains("error")) {
            std::cerr << "\n" << Color::red << "[API Error] " << Color::reset << response["error"]["message"].get<std::string>() << std::endl;
            konteks.erase(konteks.size() - 1);
            return;
        }

        json message = response["choices"][0]["message"];
        konteks.push_back(message);

        // Batasi konteks agar tidak terlalu panjang
        if (konteks.size() > 10) {
            json new_konteks;
            new_konteks.push_back(konteks[0]); // Simpan system prompt
            for(size_t i = konteks.size() - 9; i < konteks.size(); ++i) {
                new_konteks.push_back(konteks[i]);
            }
            konteks = new_konteks;
        }

        token["prompt_tokens"] = token["prompt_tokens"].get<int>() + response["usage"]["prompt_tokens"].get<int>();
        token["completion_tokens"] = token["completion_tokens"].get<int>() + response["usage"]["completion_tokens"].get<int>();

        std::string hasil = message["content"];
        // Hapus spasi/newline di awal
        hasil.erase(0, hasil.find_first_not_of(" \n\r\t"));

        std::string formatted_hasil = Color::green + "\nBot" + Color::reset + ": " + pre(hasil);

        std::stringstream ttd_ss;
        ttd_ss.precision(1);
        ttd_ss << std::fixed;
        ttd_ss << "\n=== " << Color::blue_bg << elapsed.count() << " detik ~ ($" ;
        ttd_ss.precision(5);
        ttd_ss << gptUsage() << ")" << Color::reset << " ===\n";
        std::string ttd = ttd_ss.str();

        log(formatted_hasil + ttd);
        typo(formatted_hasil + ttd);

    } catch (json::parse_error& e) {
        std::cerr << "\n" << Color::red << "[JSON Parse Error] " << Color::reset << e.what() << std::endl;
        konteks.erase(konteks.size() - 1);
    }
}

int main() {
    // Menangani sinyal resize terminal
    signal(SIGWINCH, handle_resize);

    // Menggunakan dialog Termux untuk konfirmasi penyimpanan log
    try {
        std::string dialogOutput = exec("termux-dialog confirm -t 'Simpan riwayat chat?'");
        json dialogResult = json::parse(dialogOutput);
        if (dialogResult["text"] == "yes") {
            saveLog = true;
        }
    } catch (...) {
        std::cout << "Tidak dapat menampilkan dialog Termux, penyimpanan log dinonaktifkan." << std::endl;
        saveLog = false;
    }

    // Mendapatkan API key dari environment variable
    const char* env_api_key = getenv("OPENAI_API_KEY");
    if (env_api_key == nullptr) {
        std::cerr << Color::red << "[PERINGATAN] Environment variable OPENAI_API_KEY tidak diatur." << Color::reset << std::endl;
        // Anda bisa set default key di sini jika perlu, tapi tidak disarankan
        // apiKey = "";
    } else {
        apiKey = env_api_key;
    }
    if (apiKey.empty() || apiKey.rfind("sk-", 0) != 0) {
       std::cerr << Color::red << "[PERINGATAN] API Key OpenAI tidak valid atau tidak diatur." << Color::reset << std::endl;
    }


    // Menentukan path log
    const char* home_dir = getenv("HOME");
    if (home_dir != nullptr) {
        logPath = std::string(home_dir) + "/cpp/.log";
    }

    // Inisialisasi konteks percakapan
    konteks = json::array();
    konteks.push_back({{"role", "system"}, {"content", "kamu adalah asisten AI yang berjalan di terminal Termux."}});
    konteks.push_back({{"role", "assistant"}, {"content", "Halo! Saya siap membantu. Anda bisa bertanya apa saja atau menjalankan perintah shell dengan mengetik `. <perintah>`."}});

    // Memuat riwayat jika ada
    if (saveLog) {
        std::ifstream logFile(logPath);
        if (logFile.is_open()) {
            std::stringstream buffer;
            buffer << logFile.rdbuf();
            std::string history = buffer.str();
            if (!history.empty()) {
                contentHistory.push_back(history);
                std::cout << history;
            }
        }
    }

    std::cout << Color::yellow << "Berhasil terhubung dengan Asisten" << Color::reset << std::endl;
    std::cout << "\n" << Color::green << "Bot" << Color::reset << ": " << konteks[1]["content"].get<std::string>() << std::endl;

    std::string input;
    while (true) {
        std::cout << Color::green << "\nKamu" << Color::reset << ": " << std::flush;
        if (!std::getline(std::cin, input)) {
            break; // Keluar jika Ctrl+D (EOF)
        }

        // Hapus spasi di awal dan akhir
        input.erase(0, input.find_first_not_of(" \t\n\r"));
        input.erase(input.find_last_not_of(" \t\n\r") + 1);

        if (input == ".") {
            break;
        }
        if (input == ".c" || input == ".clear") {
            std::cout << "\033[2J\033[1;1H"; // Kode ANSI untuk clear screen
            continue;
        }
        if (input.empty()) {
            continue;
        }

        if (input[0] == '.') {
            sh(input.substr(1));
            continue;
        }

        std::string logInput = "\n" + Color::green + "\nKamu" + Color::reset + ": " + input + "\n";
        log(logInput);

        gpt(input);
    }

    if (token["prompt_tokens"].get<int>() > 0) {
        std::cout << "\n" << Color::green << "Bot" << Color::reset << ": " << Color::white_bold << "Menutup koneksi..." << Color::reset << std::endl;
    }
    std::cout << Color::green << "Bot" << Color::reset << ": " << Color::white_bold << "Oke, Konsol ditutup." << Color::reset << std::endl;

    return 0;
}
