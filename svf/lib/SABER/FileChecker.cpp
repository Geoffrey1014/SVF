//===- FileChecker.cpp -- Checking incorrect file-open close errors-----------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * FileChecker.cpp
 *
 *  Created on: Apr 24, 2014
 *      Author: Yulei Sui
 */

#include "SABER/FileChecker.h"

using namespace SVF;
using namespace SVFUtil;


bool FileChecker::isSourceLikeFun(const SVFFunction* fun)
{
    CallGraphNode* cgn = PAG::getPAG()->getCallGraph()->getSVFIRCallGraphNode(fun);
    return SaberCheckerAPI::getCheckerAPI()->isFOpen(cgn);
}

bool FileChecker::isSinkLikeFun(const SVFFunction* fun)
{
    CallGraphNode* cgn = PAG::getPAG()->getCallGraph()->getSVFIRCallGraphNode(fun);
    return SaberCheckerAPI::getCheckerAPI()->isFClose(cgn);
}

void FileChecker::reportBug(ProgSlice* slice)
{

    if(isAllPathReachable() == false && isSomePathReachable() == false)
    {
        // full leakage
        GenericBug::EventStack eventStack = { SVFBugEvent(SVFBugEvent::SourceInst, getSrcCSID(slice->getSource())) };
        report.addSaberBug(GenericBug::FILENEVERCLOSE, eventStack);
    }
    else if (isAllPathReachable() == false && isSomePathReachable() == true)
    {
        GenericBug::EventStack eventStack;
        slice->evalFinalCond2Event(eventStack);
        eventStack.push_back(
            SVFBugEvent(SVFBugEvent::SourceInst, getSrcCSID(slice->getSource())));
        report.addSaberBug(GenericBug::FILEPARTIALCLOSE, eventStack);
    }
}
