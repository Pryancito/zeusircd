#ifndef SCORE_HPP__
#define SCORE_HPP__

#include <list>
#include <cassert>
#include <boost/shared_ptr.hpp>

#include "ScoredWord.h"

class Score {
public:
    boost::shared_ptr<std::list<ScoredWord> > m_ptScoredList;
    double m_dListScore;

public:
    Score() : m_ptScoredList(new std::list<ScoredWord>), m_dListScore(0)
	{ assert(m_ptScoredList); }

    Score(const boost::shared_ptr< list<ScoredWord> >& ptScoredList,
	  double dListScore)
	: m_ptScoredList(ptScoredList), m_dListScore(dListScore)
	{ assert(m_ptScoredList); }

    std::string str(bool withScoredWords = false) const;
};

#endif
