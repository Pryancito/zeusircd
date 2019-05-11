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
