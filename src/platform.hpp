#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include <string>

/**
 * @file platform.hpp
 * @brief Platform-specific utility functions for terminal manipulation.
 *
 * This header defines a set of functions encapsulated within the `platform` namespace,
 * providing cross-platform abstractions for common terminal operations such as clearing
 * the screen, reading a single character input, and setting the terminal window title.
 * These utilities are designed to enhance the portability and maintainability of terminal-based
 * applications by isolating platform-dependent code.
 */

namespace platform {

	/**
	 * @brief Clears the terminal screen.
	 *
	 * This function erases all content currently displayed in the terminal window and
	 * resets the cursor position to the top-left corner. The implementation is platform-specific
	 * and ensures consistent behavior across supported operating systems.
	 */
	void clearScreen();

	/**
	 * @brief Reads a single character from the terminal without waiting for a newline.
	 *
	 * This function captures a single character input from the user without requiring the
	 * Enter key to be pressed. It is typically used for interactive command-line applications
	 * where immediate response to keypresses is required.
	 *
	 * @return The ASCII value of the character read from the terminal.
	 */
	int getch();

	/**
	 * @brief Sets the terminal window title.
	 *
	 * This function updates the title of the terminal window to the specified string.
	 * It is useful for providing contextual information or branding within the terminal interface.
	 *
	 * @param title The new title to be displayed in the terminal window.
	 */
	void setTerminalTitle(const std::string& title);

} // namespace platform

#endif