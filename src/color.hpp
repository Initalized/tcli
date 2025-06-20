#ifndef COLOR_HPP
#define COLOR_HPP

#include <string>

// -------------------------------------------------------------------------
// ANSI Color Codes (for rich CLI output)
// -------------------------------------------------------------------------

const std::string COLOR_GRAY    = "\033[90m";
const std::string COLOR_YELLOW  = "\033[93m";
const std::string COLOR_PURPLE  = "\033[95m";
const std::string COLOR_CYAN    = "\033[96m";
const std::string COLOR_GREEN   = "\e[38;5;42m";
const std::string COLOR_RESET   = "\033[0m";
const std::string COLOR_RED     = "\033[91m";
const std::string COLOR_BLUE    = "\033[94m";
const std::string COLOR_BOLD    = "\033[1m";
const std::string COLOR_UNDER   = "\033[4m";
const std::string COLOR_BG_YEL  = "\033[43m";
const std::string COLOR_BG_CYAN = "\033[46m";
const std::string COLOR_BG_RED  = "\033[41m";
const std::string COLOR_BG_GRN  = "\e[48;5;42m";
const std::string COLOR_BG_MAG  = "\033[45m";
const std::string COLOR_BG_BLU  = "\033[44m";
const std::string COLOR_BG_WHT  = "\033[47m";
const std::string COLOR_BG_BLK  = "\033[40m";
const std::string COLOR_ORANGE  = "\033[38;5;208m";
const std::string COLOR_PINK    = "\033[38;5;213m";

#endif