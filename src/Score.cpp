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
	    out << setprecision(8) << word.word << "\t" << word.score << endl;
	}
    }
    out << setprecision(8) << m_dListScore << endl;
    return out.str();
}

