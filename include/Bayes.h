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

#include <iostream>
#include <string>

#include <map>
#include <set>
#include <functional>

#include "Score.h"
#include <boost/filesystem/path.hpp>

// for some reason there is no hash-function for std::string in gcc
struct StringHash
{
	size_t operator()(const std::string& s) const
        { return std::hash<const char*>()(s.c_str()); }
};

typedef std::map<std::string, size_t> HashMap;

//==============================================================================
// class HashTable

class HashTable
{
public:
    HashTable();

    size_t getTotalWordCount() const { return m_nTotalCount; }
    size_t getWordCount(const std::string& word) const;

    template<typename Iter>
    void learn(Iter begin, Iter end);

    template<typename Iter>
    void unlearn(Iter begin, Iter end);

    static bool hasWhitespaces(const std::string& word);

	void read (std::istream& out);
	void write (std::ostream& in);

    void learnWord(const std::string& word);
	void unlearnWord(const std::string& word);

protected:
	HashMap m_tHashMap;
    size_t m_nTotalCount;

    static const char* const m_szWhitespaces;

	friend std::ostream & operator<< (std::ostream& out, const HashTable& ht);
};


//------------------------------------------------------------------------------
std::ostream& operator<< (std::ostream& out, const HashTable& ht);




//==============================================================================
// class Bayes

class BayesClassifier
{
public:
    enum { GOOD, BAD };
	BayesClassifier() {};
private:
	double wordScore(const std::string& word) const;

    template<typename Iter>
	void unlearn(size_t table, Iter begin, Iter end);

	template<typename Iter>
	void learn(size_t table, Iter begin, Iter end);

public:

	Score score(const char* const text) const;

	void learn(size_t table, const char* const text);

	void reclassify(size_t table, const char* const text);

	void loadham(const boost::filesystem::path file);
	void loadspam(const boost::filesystem::path file);
	
	void save(const boost::filesystem::path file);

private:
    // use an array so we can use learn(size_t table, ...)
    HashTable m_atHashTables[2];

	static const int mScoredItems;

	static const char* const m_szTokenSeparators;
	static const char* const m_szTokenSeparatorsKept;

	friend std::ostream & operator<< (std::ostream& out, const BayesClassifier& base);
};


//------------------------------------------------------------------------------
std::ostream& operator<< (std::ostream& out, const BayesClassifier& base);
extern BayesClassifier *bayes;

#include "Bayes_impl.h"
