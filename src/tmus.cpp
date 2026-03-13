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
#include <algorithm>
#include <random>


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
std::map<std::string, std::string> song_list;


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
            std::cout << RUST << "⏹Stop [S]   ⏮ Previous [P]   ⏭ Next [N]\n" << RESET;
            std::cout << RUST << "Now Playing: " << now_playing << RESET << "\n\n";
        } else {
            std::cout << RUST << "⏹Stop [S]   ⏮ Previous [P]   ⏭ Next [N]\n" << RESET;
            std::cout << RUST << "Now Playing: None" << RESET << "\n\n";
        }
    }

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

class Command {
    public:
        virtual void execute() = 0;
        virtual ~Command() = default;
};

class DownloadCommand : public Command {
public:
    void execute() override {
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
            song_list[song_name] = song_name + ".mp3";
        }


        save_songs();
    }    

};

class PlayCommand : public Command {
public:
    void execute() override {
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
};

class QuitCommand : public Command {
public:
    void execute() override {
        mpon = false;
        std::system("pkill ffplay");
    }
};

class NextCommand : public Command {
public:
    void execute() override {
        std::lock_guard<std::mutex> lock(song_mutex);
		
		if(song_list.empty()) return;
		
		auto it = song_list.find(now_playing);
		
		if (it == song_list.end()) {
			it = song_list.begin();
		} else {
			++it;
			if ( it == song_list.end()) 
				it = song_list.begin();
		}

		std::system("pkill ffplay");

		play_song_bg(it->second);


		{
			std::lock_guard<std::mutex> np_lock(now_playing_mutex);
            now_playing = it->first;
		}
	}
};

class PrevCommand : public Command {
public:
    void execute() override {
        std::lock_guard<std::mutex> lock(song_mutex);
		
		if(song_list.empty()) return;
		
		auto it = song_list.find(now_playing);
		
		if (it == song_list.end()) {
			it = song_list.begin();
		} else {
			--it;
			if ( it == song_list.end()) 
				it = song_list.begin();
		}

		std::system("pkill ffplay");

		play_song_bg(it->second);


		{
			std::lock_guard<std::mutex> np_lock(now_playing_mutex);
            now_playing = it->first;
		}
	}
};

class ShuffCommand : public Command {
public:
	void execute() override {
		std::lock_guard<std::mutex> lock(song_mutex);

		if(song_list.empty()) return;

		std::vector<std::pair<std::string, std::string>> shuffled;

		std::string np_shuff;


		for (auto &song : song_list)
			shuffled.push_back(song);

		std::random_device rd;
		std::mt19937 rng(rd());

		std::shuffle(shuffled.begin(), shuffled.end(), rng);

		for (auto &song : shuffled) {
			std::system("pkill ffplay");
			np_shuff = song.second;
			play_song_bg(np_shuff);
		}

		{
			std::lock_guard<std::mutex> np_lock(now_playing_mutex);
			now_playing = np_shuff;
		}


	}
};




















void main_loop() {
    std::map<char, std::unique_ptr<Command>> command_map;
    command_map['D'] = std::make_unique<DownloadCommand>();
    command_map['I'] = std::make_unique<PlayCommand>();
    command_map['Q'] = std::make_unique<QuitCommand>();
    command_map['N'] = std::make_unique<NextCommand>();
	command_map['B'] = std::make_unique<PrevCommand>();
	command_map['S'] = std::make_unique<ShuffCommand>();

    std::string user_input;
    while (mpon) {
        clear_screen();
        display_songs();
        std::cout.flush();

        char c;
        if (std::cin.get(c)) {
            if (c == '\n') {
                if (!user_input.empty()) {
                    char cmd = std::toupper(user_input[0]);
                    auto it = command_map.find(cmd);
                    if (it != command_map.end()) {
                        it->second->execute();
                    } else {
                        std::cout << "Unknown command\n";
                    }
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
