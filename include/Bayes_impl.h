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

#ifndef BAYES_HPP__
#  error Do not include this file!
#endif

#include <cassert>


//==============================================================================
// class HashTable

//------------------------------------------------------------------------------
template<typename Iter>
void HashTable::learn(Iter begin, Iter end)
{
    for ( ; begin != end; ++begin) {
        learnWord(*begin);
	}
	++m_nTotalCount;
}


//------------------------------------------------------------------------------
template<typename Iter>
void HashTable::unlearn(Iter begin, Iter end)
{
    for ( ; begin != end; ++begin)
        unlearnWord(*begin);
    assert(m_nTotalCount >= 1);
    --m_nTotalCount;
}




//==============================================================================
// class BayesClassifier

//------------------------------------------------------------------------------
template<typename Iter> inline
void BayesClassifier::learn(size_t table, Iter begin, Iter end)
{
    assert(table < 2);
    m_atHashTables[table].learn(begin, end);
}


//------------------------------------------------------------------------------
template<typename Iter> inline
void BayesClassifier::unlearn(size_t table, Iter begin, Iter end)
{
    assert(table < 2);
    m_atHashTables[table].unlearn(begin, end);
}
