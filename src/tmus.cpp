#include <iostream>
#include <vector>
#include <memory>
#include <cstdlib>
#include <thread>
#include <map>
#include <functional>
#include <string>
#include <utility>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <atomic>

// Color Schemes
const std::string RUST = "\033[38;2;222;159;104m";
const std::string RUST_BOLD = "\033[1;38;2;222;159;104m";
const std::string LIGHT_GRAY = "\033[38;2;191;191;191m";
const std::string LIGHT_GRAY_BOLD = "\033[1;38;2;191;191;191m";
const std::string RESET = "\033[0m";

std::atomic<bool> mpon{true};

// mutex for song operations
std::mutex song_mutex;
std::mutex now_playing_mutex;
std::string now_playing;

// clear screen
void clear_screen() {
    std::cout << "\033[H\033[2J";
}



// Store Songs
std::unordered_map<std::string, std::string> song_list;


// Save
void save_songs() {
    std::lock_guard<std::mutex> lock(song_mutex);
    
    
    std::ofstream file("songs.txt");

    for (auto it=song_list.begin(); it != song_list.end(); it++) {
        file << it->first << "|" << it->second << "\n";
    }
};

void load_songs() {
    std::lock_guard<std::mutex> lock(song_mutex);
    std::ifstream file("songs.txt");

    std::string line;

    while(std::getline(file, line)) {
        size_t sep = line.find('|');
        if (sep != std::string::npos) {
            std::string song_name = line.substr(0, sep);
            std::string output = line.substr(sep + 1);
            song_list[song_name]  = output;
        }
    }
}

void display_songs() {
    std::lock_guard<std::mutex> lock(song_mutex);
    int tracknum = 1;
    std::cout << RUST_BOLD << "--- Available Songs ---\n" << RESET;
    std::cout << RUST_BOLD << "Track\tTitle\n" << RESET;
    std::cout << RUST << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;
    for (auto& [name, path] : song_list) {
        std::cout << LIGHT_GRAY << tracknum++ << "\t" << name << RESET << "\n";
    }
    std::cout << RUST << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;
    
    {
        std::lock_guard<std::mutex> np_lock(now_playing_mutex);
        if(!now_playing.empty()){
            std::cout << RUST << "Now Playing: " << now_playing << RESET << "\n\n";
        } else {
            std::cout << RUST << "Now Playing: None" << RESET << "\n\n";
        }
    }

}



void quit() {
    mpon = false;
    std::system("pkill ffplay");
}

void next() {
    std::cout << "Next function called\n";
}

void download() {
    std::cout << "Download function called\n";
    std::string url;
    std::cout << "Enter URL\n";
    std::getline(std::cin, url);
    std::cout << "Save Song as\n";
    std::string song_name;
    std::getline(std::cin, song_name);
    std::string output_path = std::string(getenv("HOME")) + "/Music/" + song_name + ".mp3";
    std::string command = "yt-dlp -x --audio-format mp3 -o \"" + output_path + "\" --quiet --no-warnings  \"" + url + "\"";
    std::system(command.c_str());
    
    {
        std::lock_guard<std::mutex> lock(song_mutex);
        song_list[song_name] = output_path;
    }


    save_songs();    

}

void pauseM() {
}

void play_song_bg(std::string& song_file) {
    std::thread t2([song_file]() {
        std::string music_dir = std::string(getenv("HOME")) + "/Music";
        std::string final_song = music_dir + "/" + song_file;
        std::string command = "ffplay -nodisp -autoexit -loglevel quiet \"" + final_song + "\"";
        std::system(command.c_str());
    });
    t2.detach();
}


void selectM() {
    std::string song_name;
    std::cout << "Enter Track to Play\n";
    std::getline(std::cin, song_name);
   
    std::lock_guard<std::mutex> lock(song_mutex);
    auto it = song_list.find(song_name);
    if(it != song_list.end()) {
        play_song_bg(it->second);

        {
            std::lock_guard<std::mutex> np_lock(now_playing_mutex);
            now_playing = song_name;
        }
        std::cout << RUST << "PLaying: " << song_name << "\n" << RESET;
    } else {
        std::cout << "Song Not avaliable\n";
    }

}


void main_loop() {
    std::string user_input;
    while (mpon) {
        clear_screen();
        display_songs();
        std::cout.flush();

        char c;
        if (std::cin.get(c)) {
            if (c == '\n') {
                if (!user_input.empty()) {
                    char cmd = toupper(user_input[0]);
                    if (cmd == 'D') download();
                    else if (cmd == 'S') selectM();
                    else if (cmd == 'Q') quit();
                    else std::cout << "Unknown command\n";
                    user_input.clear();
                }
            } else {
                user_input += c;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main() {
    load_songs();
    main_loop();
    std::cout << "Exiting tmus...\n";
    return 0;
}
