#include <sstream>
#include <string>
#include <iomanip>
#include <cassert>

#include "Score.h"

using namespace std;

string Score::str(bool withScoredWords) const
{
    assert(m_ptScoredList);
    stringstream out;
    if (withScoredWords)
    {
	list<ScoredWord>::const_iterator iter = m_ptScoredList->begin();
	for  (; iter != m_ptScoredList->end(); ++iter)
	{
	    ScoredWord word = *iter;
	    out << setprecision(2) << word.word << "\t" << word.score << endl;
	}
    }
    out << setprecision(2) << m_dListScore << endl;
    return out.str();
}

