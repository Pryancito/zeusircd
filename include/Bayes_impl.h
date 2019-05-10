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
