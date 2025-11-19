// Debug.hpp

#pragma once
/**
 * @file Debug.hpp
 * @brief Header file for debugging utilities.
 */

#include <iostream>
#include "Config.hpp" // Include the config file to access the flags

#ifndef DEBUG_PRINT
#  ifdef DEBUG_MODE
#    define DEBUG_PRINT(x) std::cout << x << std::endl;
#  else
#    define DEBUG_PRINT(x)
#  endif
#endif
