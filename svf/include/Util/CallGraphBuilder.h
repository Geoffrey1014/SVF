//===- CallGraphBuilder.h ----------------------------------------------------------------//
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
 * CallGraphBuilder.h
 *
 *  Created on: 13Mar.,2020
 *      Author: Yulei Sui
 */

#ifndef INCLUDE_SVF_FE_CALLGRAPHBUILDER_H_
#define INCLUDE_SVF_FE_CALLGRAPHBUILDER_H_

#include "Graphs/CallGraph.h"
#include "Graphs/ThreadCallGraph.h"

namespace SVF
{

class ICFG;

class CallGraphBuilder
{

protected:
    CallGraph* callgraph;
    ICFG* icfg;
public:
    CallGraphBuilder(CallGraph* cg, ICFG* i): callgraph(cg),icfg(i)
    {
    }

    /// Build normal callgraph
    CallGraph* buildCallGraph();

    /// Buidl SVFIR callgraoh
    CallGraph* buildSVFIRCallGraph();

};

class ThreadCallGraphBuilder : public CallGraphBuilder
{

public:
    ThreadCallGraphBuilder(ThreadCallGraph* cg, ICFG* i): CallGraphBuilder(cg,i)
    {
    }

    /// Build thread-aware callgraph
    CallGraph* buildThreadCallGraph();

};

} // End namespace SVF


#endif /* INCLUDE_UTIL_CALLGRAPHBUILDER_H_ */