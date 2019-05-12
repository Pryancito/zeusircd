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

#include <cassert>
#include <algorithm>
#include <iomanip>
#include <queue>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include "Bayes.h"
#include "ScoredWord.h"
#include "user.h"
#include "services.h"

using namespace std;


//==============================================================================
// class HashTable

//------------------------------------------------------------------------------
const char* const HashTable::m_szWhitespaces = " \t\n\r\b\f\v";
BayesClassifier *bayes = new BayesClassifier();

//------------------------------------------------------------------------------
HashTable::HashTable()
	: m_nTotalCount(0)
{
}


//------------------------------------------------------------------------------
size_t HashTable::getWordCount(const std::string& word) const
{
    assert(!hasWhitespaces(word));

    const HashMap::const_iterator tIter = m_tHashMap.find(word);
    return tIter != m_tHashMap.end() ? tIter->second : 0;
}


//------------------------------------------------------------------------------
void HashTable::learnWord(const std::string& word)
{
    assert(!hasWhitespaces(word));

    ++m_tHashMap[word];
}


//------------------------------------------------------------------------------
void HashTable::unlearnWord(const std::string& word)
{
    assert(!hasWhitespaces(word));

    HashMap::iterator tIter = m_tHashMap.find(word);

    if (tIter != m_tHashMap.end()) {
        size_t& nWordCount = tIter->second;
        assert(nWordCount >= 1);

        if (nWordCount > 1) {
            --nWordCount;
        } else {
            m_tHashMap.erase(tIter);
        }
    }
}


//------------------------------------------------------------------------------
bool HashTable::hasWhitespaces(const std::string& word)
{
    return (word.find_first_of(m_szWhitespaces) != string::npos);
}


//------------------------------------------------------------------------------
void HashTable::write(ostream& out)
{
    out << (unsigned int) m_nTotalCount << endl;
	out << (unsigned int) m_tHashMap.size() << endl;

    HashMap::const_iterator iter;

	for (iter = m_tHashMap.begin(); iter != m_tHashMap.end(); ++iter) {
		out << iter->first << " " << (unsigned int) iter->second << endl;
	}
}

//------------------------------------------------------------------------------
void HashTable::read(istream& in)
{
    m_tHashMap.clear();

    m_nTotalCount = 0;

	char Linea[1024];
	
	lin:
	
	in.getline(Linea, 1024);

    while(!in.eof()) {
        string strLine;
        size_t nWordCount = 0;

        strLine = Linea;
        StrVec  x;
		boost::split(x, strLine, boost::is_any_of("@~&\".,;!?:-=|<>()[]{}/\\\t\n\r\b\f\v "), boost::token_compress_on);
		nWordCount = x.size();
		for (unsigned int i = 0; i < nWordCount; i++) {
			m_tHashMap[x[i]]++;
			m_nTotalCount++;
		}
        goto lin;
    }
}

//------------------------------------------------------------------------------
ostream& operator<< (ostream& out, const HashTable& ht)
{
	out << "Gelernte Beispiele: " << (unsigned int) ht.m_nTotalCount << endl;
	out << "Gelernte Woerter: " << (unsigned int) ht.m_tHashMap.size() << endl;

    return out;
}



//==============================================================================
// class Bayes

const char* const BayesClassifier::m_szTokenSeparators = "@~&\".,;!?:-=|<>()[]{}/\\\t\n\r\b\f\v ";
const char* const BayesClassifier::m_szTokenSeparatorsKept = "\03\02\16\1F";

const int BayesClassifier::mScoredItems = 15;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/*
  ; Bayesian score
  ; documentary at http://www.paulgraham.com/spam.html
  ; ---------------------------------------------------------
  ; (let ((g (* 2 (or (gethash word good) 0)))
  ;     (b (or (gethash word bad) 0)))
  ;  (unless (< (+ g b) 5)
  ;    (max .01
  ;         (min .99 (float (/ (min 1 (/ b nbad))
  ;                            (+ (min 1 (/ g ngood))
  ;                               (min 1 (/ b nbad)))))))))
  ; ---------------------------------------------------------
*/
double BayesClassifier::wordScore(const string& word) const
{
    size_t g = 2*m_atHashTables[GOOD].getWordCount(word);
    size_t b = m_atHashTables[BAD].getWordCount(word);

	if (g+b >= 5) {
		double i = min(1.0, double(b)/m_atHashTables[BAD].getTotalWordCount());
		double j = min(1.0, double(g)/m_atHashTables[GOOD].getTotalWordCount());
		
        return max(0.01, min(0.99, i/(i+j)));
	} else {
        return 0.4;
    }
}

//------------------------------------------------------------------------------
void BayesClassifier::save(const boost::filesystem::path file)
{
	ofstream out(file.string().c_str());
	out << "----------GOOD----------\n";
	m_atHashTables[BayesClassifier::GOOD].write(out);
    out	<< endl << endl
		<< "----------BAD----------\n";
	m_atHashTables[BayesClassifier::BAD].write(out);
	out << endl;
	out.close();
}

//------------------------------------------------------------------------------
void BayesClassifier::loadspam(const boost::filesystem::path file)
{
	ifstream inFile(file.string().c_str());

	if (!inFile) {
	    std::cout << "Error opening: " << file.string() << endl;
	} else {
        m_atHashTables[BayesClassifier::BAD].read(inFile);
	}

	inFile.close();
}

//------------------------------------------------------------------------------
void BayesClassifier::loadham(const boost::filesystem::path file)
{
	ifstream inFile(file.string().c_str());

	if (!inFile) {
	    std::cout << "Error opening: " << file.string() << endl;
	} else {
        m_atHashTables[BayesClassifier::GOOD].read(inFile);
	}

	inFile.close();
}

//------------------------------------------------------------------------------
void BayesClassifier::learn(size_t table, const char* const text)
{
	string learn_text(text);
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(m_szTokenSeparators, m_szTokenSeparatorsKept);

    tokenizer tokens(learn_text, sep);

	learn(table, tokens.begin(), tokens.end());
}

//------------------------------------------------------------------------------
void BayesClassifier::reclassify(size_t table, const char *text)
{
	string learn_text(text);
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(m_szTokenSeparators, m_szTokenSeparatorsKept);

    tokenizer tokens(learn_text, sep);

	unlearn(1-table, tokens.begin(), tokens.end());
	learn(table, tokens.begin(), tokens.end());
}

Score BayesClassifier::score(const char* text) const
{
	Score score;
	string score_text(text);
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(m_szTokenSeparators, m_szTokenSeparatorsKept);

    tokenizer tokens(score_text, sep);

	priority_queue<ScoredWord> pqueue;

	tokenizer::const_iterator iter = tokens.begin();

	for (; iter != tokens.end(); ++iter)
	{
		if (OperServ::IsSpammed(*iter)) {
			score.m_dListScore = 1.00;
			return score;
		}
		pqueue.push(ScoredWord(*iter, wordScore(*iter)));
	}

	for (int i = 0; ((i < BayesClassifier::mScoredItems) && !pqueue.empty()); ++i)
	{
		score.m_ptScoredList->push_back(pqueue.top());
		pqueue.pop();
	}

	double prod = 1, negprod = 1;

	list<ScoredWord>::const_iterator ws_iter = score.m_ptScoredList->begin();

	for (; ws_iter != score.m_ptScoredList->end(); ++ws_iter)
	{
		prod *= ws_iter->score;
		negprod *= (1 - ws_iter->score);
	}

	score.m_dListScore = prod/(prod + negprod);

	return score;
}

//------------------------------------------------------------------------------
ostream& operator<< (ostream& out, const BayesClassifier& base)
{
	out << "Ham:" << endl
		<< base.m_atHashTables[BayesClassifier::GOOD]
		<< "Spam:" << endl
		<< base.m_atHashTables[BayesClassifier::BAD];

    return out;
}
