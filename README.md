# TCLI - Tactical Command-Line Interface

**TCLI** is a highly interactive, multi-threaded command-line interface for local and remote directory enumeration, scanning, session management, and more. It is designed for power users, sysadmins, and security testers who need a fast, extensible, and visually rich CLI toolkit.

---

## Features

- **Local & Global Directory Listing:**  
    List and enumerate local directories or remote HTTP(S) directories with recursion and parallelization.

- **Parallelized Scanning:**  
    Fast directory and port scanning using multi-threading.

- **Command History & Syntax Highlighting:**  
    Navigate history with arrow keys, use tab-completion, and enjoy rich syntax highlighting for commands, paths, URLs, and more.

- **Session Management:**  
    List, kill, and resume sessions for advanced workflows.

- **Simulated Security Testing:**  
    Simulate scans, injections, spoofing, and authentication bypasses for educational or testing purposes.

- **Configurable & Persistent:**  
    All options are configurable at runtime and can be saved to a persistent config file.

- **ANSI Color Output & Banners:**  
    Customizable banners and color schemes for a modern CLI experience.

- **Highly Modular:**  
    Easily extensible command structure for adding new features.

---

## Quick Start

1. **Build:**  
     Requires C++17, `clang`, `curl`, a POSIX-compatible terminal, and `make`.
     ```sh
     make
     ```

2. **Run:**  
     ```sh
     ./tcli
     ```

3. **Setup Config (optional):**  
     ```sh
     tcli setup
     ```

---

## Example Commands

- `help` — Show help and command list
- `connect local /path/to/dir` — Connect to a local directory
- `connect global https example.com` — Connect to a remote HTTP(S) directory
- `ld local` — List local directory contents
- `ld global` — List global (remote) directory contents recursively
- `enum` — Enumerate directories on the connected global URL
- `scan 192.168.1.1` — Scan for open ports/services
- `inject target payload --sql` — Simulate SQL injection
- `spoof mac --randomize` — Simulate MAC address spoofing
- `session list` — List active sessions
- `config show` — Show current configuration
- `set user "newuser" true` — Change config in realtime (persist if `true`)

---

## Keyboard Shortcuts

- **Tab:** Auto-complete commands and arguments
- **Up/Down:** Navigate command history
- **Syntax Highlighting:**  
    - Commands: **purple bold**
    - Paths: **yellow bold**
    - URLs: **cyan underline**
    - Numbers: **green**
    - Strings: **yellow on blue**
    - Flags: **blue on yellow**
    - Local/Global: **green/cyan background**

---

## Configuration

All options are stored in a config file (`TCLI` by default).  
Change settings with `config set <key> <value>` or `set <key> <value> <true|false>`.

Example config keys:
- `user`, `lc_path`, `gl_path`, `prompt_color`, `banner_color`, `history_file`, etc.

---

## Author

- **Initalize**

---

## License

MIT License (see `LICENSE` file).

---