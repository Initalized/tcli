//------------------------------------------------------------------------------
// @file common.cppm
// @brief Exports the standard <condition_variable> header for use in this module.
//
// This export statement makes the facilities provided by the C++ Standard Library's
// <condition_variable> header available to any translation unit that imports this module.
// The <condition_variable> header provides synchronization primitives, such as
// std::condition_variable and std::condition_variable_any, which are essential for
// thread communication and coordination in concurrent programming.
//
// Usage:
//   Import this module in other parts of the project to utilize condition variables
//   without directly including the standard header.
//
// @note Exporting standard library headers through modules can improve build times
//       and encapsulate dependencies, promoting modular and maintainable code.
//
// @see https://en.cppreference.com/w/cpp/thread/condition_variable
//------------------------------------------------------------------------------

export module common;

export import <iostream>;
export import <fstream>;
export import <string>;
export import <map>;
export import <functional>;
export import <filesystem>;
export import <thread>;
export import <chrono>;
export import <random>;
export import <vector>;
export import <regex>;
export import <cstdio>;
export import <set>;
export import <cctype>;
export import <algorithm>;
export import <atomic>;
export import <mutex>;
export import <future>;
export import <sstream>;
export import <array>;
export import <shared_mutex>;
export import <condition_variable>;
