//===- FlowDDA.cpp -- Flow-sensitive demand-driven analysis -------------//
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
 * FlowDDA.cpp
 *
 *  Created on: Jun 30, 2014
 *      Author: Yulei Sui, Sen Ye
 */

#include "Util/Options.h"
#include "DDA/FlowDDA.h"
#include "DDA/DDAClient.h"
#include "MemoryModel/PointsTo.h"

using namespace std;
using namespace SVF;
using namespace SVFUtil;


/*!
 * Compute points-to set for queries
 */
void FlowDDA::computeDDAPts(NodeID id)
{
    resetQuery();
    LocDPItem::setMaxBudget(Options::FlowBudget());

    PAGNode* node = getPAG()->getGNode(id);
    LocDPItem dpm = getDPIm(node->getId(),getDefSVFGNode(node));

    /// start DDA analysis
    DOTIMESTAT(double start = DDAStat::getClk(true));
    const PointsTo& pts = findPT(dpm);
    DOTIMESTAT(ddaStat->_AnaTimePerQuery = DDAStat::getClk(true) - start);
    DOTIMESTAT(ddaStat->_TotalTimeOfQueries += ddaStat->_AnaTimePerQuery);

    if(isOutOfBudgetQuery() == false)
        unionPts(node->getId(),pts);
    else
        handleOutOfBudgetDpm(dpm);

    if(this->printStat())
        DOSTAT(stat->performStatPerQuery(node->getId()));

    DBOUT(DGENERAL,stat->printStatPerQuery(id,getPts(id)));
}


/*!
 * Handle out-of-budget dpm
 */
void FlowDDA::handleOutOfBudgetDpm(const LocDPItem& dpm)
{
    DBOUT(DGENERAL,outs() << "~~~Out of budget query, downgrade to andersen analysis \n");
    const PointsTo& anderPts = getAndersenAnalysis()->getPts(dpm.getCurNodeID());
    updateCachedPointsTo(dpm,anderPts);
    unionPts(dpm.getCurNodeID(),anderPts);
    addOutOfBudgetDpm(dpm);
}

bool FlowDDA::testIndCallReachability(LocDPItem&, const CallGraphNode* callee, CallSiteID csId)
{

    const CallICFGNode* cbn = getSVFG()->getCallSite(csId);

    if(getPAG()->isIndirectCallSites(cbn))
    {
        if(getCallGraph()->hasIndCSCallees(cbn))
        {
            const FunctionSet& funset = getCallGraph()->getIndCSCallees(cbn);
            if(funset.find(callee)!=funset.end())
                return true;
        }

        return false;
    }
    else	// if this is an direct call
        return true;

}

bool FlowDDA::handleBKCondition(LocDPItem& dpm, const SVFGEdge* edge)
{
    _client->handleStatement(edge->getSrcNode(), dpm.getCurNodeID());
    return true;
}

/*!
 * Generate field objects for structs
 */
PointsTo FlowDDA::processGepPts(const GepSVFGNode* gep, const PointsTo& srcPts)
{
    PointsTo tmpDstPts;
    for (PointsTo::iterator piter = srcPts.begin(); piter != srcPts.end(); ++piter)
    {
        NodeID ptd = *piter;
        if (isBlkObjOrConstantObj(ptd))
            tmpDstPts.set(ptd);
        else
        {
            const GepStmt* gepStmt = SVFUtil::cast<GepStmt>(gep->getPAGEdge());
            if (gepStmt->isVariantFieldGep())
            {
                setObjFieldInsensitive(ptd);
                tmpDstPts.set(getFIObjVar(ptd));
            }
            else
            {
                NodeID fieldSrcPtdNode = getGepObjVar(ptd, gepStmt->getAccessPath().getConstantStructFldIdx());
                tmpDstPts.set(fieldSrcPtdNode);
            }
        }
    }
    DBOUT(DDDA, outs() << "\t return created gep objs {");
    DBOUT(DDDA, SVFUtil::dumpSet(srcPts));
    DBOUT(DDDA, outs() << "} --> {");
    DBOUT(DDDA, SVFUtil::dumpSet(tmpDstPts));
    DBOUT(DDDA, outs() << "}\n");
    return tmpDstPts;
}

/// we exclude concrete heap here following the conditions:
/// (1) local allocated heap and
/// (2) not escaped to the scope outside the current function
/// (3) not inside loop
/// (4) not involved in recursion
bool FlowDDA::isHeapCondMemObj(const NodeID& var, const StoreSVFGNode*)
{
    const BaseObjVar* pVar = _pag->getBaseObject(getPtrNodeID(var));
    if(pVar && SVFUtil::isa<HeapObjVar, DummyObjVar>(pVar))
    {
        return true;
    }
    return false;
}
