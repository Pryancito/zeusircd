#ifndef SCOREDWORD_HPP__
#define SCOREDWORD_HPP__

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

#endif
