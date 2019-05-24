/* 
 * This file is part of the ZeusiRCd distribution (https://github.com/Pryancito/zeusircd).
 * Copyright (c) 2019 Rodrigo Santidrian AKA Pryan.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <utility>
#include <cmath>
#include <string>

using namespace std;

class ScoredWord {
public:
    ScoredWord(void) {};
    ScoredWord(const char* text, double s = 0.0) : word(text), score(s) {};
    ScoredWord(const string& text, double s = 0.0) : word(text), score(s) {};

    inline bool operator<(const ScoredWord& w) const { return (abs(0.5 - score) < abs(0.5 - w.score)); }

public:
    string word;
    double score;
};
