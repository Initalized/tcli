/**
 * @file platform.cpp
 * @brief Cross-platform terminal utilities for TCLI
 *
 * This file provides a set of platform-agnostic utility functions to manage
 * terminal behavior and user input in a consistent manner across Windows and
 * POSIX-compliant systems. The utilities abstract away OS-specific details,
 * enabling seamless terminal control for the Tactical Command-Line Interface (TCLI).
 *
 * Features include:
 *   - Clearing the terminal screen on both Windows and UNIX-like systems
 *   - Reading single keypresses without requiring Enter (getch), with echo suppression
 *   - Dynamically setting the terminal window title for enhanced user experience
 *
 * All functions are encapsulated within the `platform` namespace to ensure
 * modularity and prevent naming conflicts.
 *
 * @author
 *   Initalize
 * @date
 *   2025-06-17
 */

#include "platform.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#include <iostream>

namespace platform {
    /**
     * @brief Clears the terminal screen.
     *
     * Uses the appropriate system command to clear the terminal screen
     * on both Windows and UNIX-like systems.
     */
    void clearScreen() {
        #ifdef _WIN32
        system("cls");
        #else
        system("clear");
        #endif
    }

    /**
     * @brief Reads a single character from the terminal without echo.
     *
     * Captures a single keypress from the user without requiring Enter,
     * and suppresses the character from being echoed to the terminal.
     * Handles both Windows and POSIX systems.
     *
     * @return The character code of the key pressed.
     */
    int getch() {
        #ifdef _WIN32
        return _getch();
        #else
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        int ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
        #endif
    }

    /**
     * @brief Sets the terminal window title.
     *
     * Dynamically updates the terminal or console window title to the
     * specified string, supporting both Windows and UNIX-like systems.
     *
     * @param title The new title for the terminal window.
     */
    void setTerminalTitle(const std::string& title) {
        #ifdef _WIN32
        SetConsoleTitleA(title.c_str());
        #else
        std::cout << "\033]0;" << title << "\007";
        std::cout.flush();
        #endif
    }
}