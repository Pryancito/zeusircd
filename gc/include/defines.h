#ifndef DEFINES_H
#define DEFINES_H

#include "config.h"
#include <string>
#include <sstream>

template<typename T>
std::string ToString( const T & Value ) {
    std::ostringstream oss;
    oss << Value;
    return oss.str();
}

#endif
