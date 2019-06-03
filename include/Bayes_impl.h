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

#include <cassert>
#include <boost/iterator/iterator_adaptor.hpp>

//==============================================================================
// class HashTable

//------------------------------------------------------------------------------
template<class Iterator>
void HashTable::learn(Iterator &begin, Iterator &end)
{
    for ( ; begin != end; ++begin) {
        learnWord(*begin);
	}
	++m_nTotalCount;
}


//------------------------------------------------------------------------------
template<class Iterator>
void HashTable::unlearn(Iterator &begin, Iterator &end)
{
    for ( ; begin != end; ++begin)
        unlearnWord(*begin);
    assert(m_nTotalCount >= 1);
    --m_nTotalCount;
}




//==============================================================================
// class BayesClassifier

//------------------------------------------------------------------------------
template<class Iterator>
void BayesClassifier::learn(size_t table, Iterator begin, Iterator end)
{
    assert(table < 2);
    m_atHashTables[table].learn(begin, end);
}


//------------------------------------------------------------------------------
template<class Iterator>
void BayesClassifier::unlearn(size_t table, Iterator begin, Iterator end)
{
    assert(table < 2);
    m_atHashTables[table].unlearn(begin, end);
}
