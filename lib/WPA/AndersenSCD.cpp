//===- AndersenSCD.cpp -- SCD based field-sensitive Andersen's analysis-------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2017>  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * AndersenSCD.cpp
 *
 *  Created on: 09, Feb, 2019
 *      Author: Yuxiang Lei
 */

#include "WPA/Andersen.h"
#include "Util/SVFUtil.h"

using namespace SVFUtil;

AndersenSCD* AndersenSCD::scdAndersen = NULL;


// TODO: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*!
 *
 */
void AndersenSCD::solveWorklist() {
    // Initialize the nodeStack via a whole SCC detection
    // Nodes in nodeStack are in topological order by default.
    SCCDetect();
    sccCandidates.clear();
//    while (!topoOrder.empty()) {
//        NodeID nodeId = topoOrder.top();
//        topoOrder.pop();
//        repNodes.push(nodeId);
////        sccCandidates.insert(nodeId);
//    }

    // Process nodeStack and put the changed nodes into workList.
    procRepNodes();

    // New nodes will be inserted into workList during processing.
    procIndirectNodes();

    // reanalysis stops when sccCandidates become empty.
    if(!sccCandidates.empty())
        reanalyze = true;
}

/*!
 *
 */
NodeStack& AndersenSCD::SCCDetect() {
    numOfSCCDetection++;

    double sccStart = stat->getClk();
    WPAConstraintSolver::SCCDetect(sccCandidates);
    double sccEnd = stat->getClk();
    timeOfSCCDetection +=  (sccEnd - sccStart)/TIMEINTERVAL;

    double mergeStart = stat->getClk();
    mergeSccCycle();
    double mergeEnd = stat->getClk();
    timeOfSCCMerges +=  (mergeEnd - mergeStart)/TIMEINTERVAL;

    return getSCCDetector()->topoNodeStack();
}

/*!
 *
 */
bool AndersenSCD::mergeSrcToTgt(NodeID nodeId, NodeID newRepId){
    if(nodeId==newRepId)
        return false;

    // union pts of node to rep
    if (unionPts(newRepId, nodeId))
        pushIntoWorklist(newRepId);

    /// move the edges from node to rep, and remove the node
    ConstraintNode* node = consCG->getConstraintNode(nodeId);
    bool gepInsideScc = consCG->moveEdgesToRepNode(node, consCG->getConstraintNode(newRepId));

    /// set rep and sub relations
    updateNodeRepAndSubs(node->getId(),newRepId);
    consCG->removeConstraintNode(node);

    return gepInsideScc;
}

/*!
 *
 */
void AndersenSCD::procRepNodes() {
    while (!isWorklistEmpty()) {
        NodeID nodeId = popFromWorklist();
        collapsePWCNode(nodeId);
        // process nodes in nodeStack
        if (sccRepNode(nodeId) != nodeId)
            return;

        ConstraintNode* node = consCG->getConstraintNode(nodeId);
        handleCopyGep(node);

        if (!node->getLoadOutEdges().empty() || !node->getStoreInEdges().empty())
            indirectNodes.push(node->getId());
        collapseFields();
    }
}

/*!
 *
 */
void AndersenSCD::procIndirectNodes() {
    while (!indirectNodes.empty()) {
        NodeID nodeId = indirectNodes.pop();
//        if (sccRepNode(nodeId) != nodeId)  // Really omittable??
//            continue;
        // process nodes in worklist
        ConstraintNode* node = consCG->getConstraintNode(nodeId);
        handleLoadStore(node);
    }
}

/*!
 *
 */
void AndersenSCD::handleLoadStore(ConstraintNode *node) {
    double insertStart = stat->getClk();

    NodeID nodeId = node->getId();
    for (PointsTo::iterator piter = getPts(nodeId).begin(), epiter =
            getPts(nodeId).end(); piter != epiter; ++piter) {
        NodeID ptd = *piter;
        // handle load
        for (ConstraintNode::const_iterator it = node->outgoingLoadsBegin(),
                     eit = node->outgoingLoadsEnd(); it != eit; ++it) {
            if (processLoad(ptd, *it)) {
                NodeID dstId = (*it)->getDstID();
                addSccCandidate(ptd);
                if (getPts(ptd) != getPts(dstId))
                    pushIntoWorklist(ptd);
            }
        }

        // handle store
        for (ConstraintNode::const_iterator it = node->incomingStoresBegin(),
                     eit = node->incomingStoresEnd(); it != eit; ++it) {
            if (processStore(ptd, *it)) {
                NodeID srcId = (*it)->getSrcID();
                addSccCandidate(srcId);
                if (getPts(ptd) != getPts(srcId))
                    pushIntoWorklist(srcId);
            }
        }
    }
    double insertEnd = stat->getClk();
    timeOfProcessLoadStore += (insertEnd - insertStart) / TIMEINTERVAL;
}

/*!
 *
 */
void AndersenSCD::processAddr(const AddrCGEdge *addr) {
    numOfProcessedAddr++;

    NodeID dst = addr->getDstID();
    NodeID src = addr->getSrcID();
    if(addPts(dst,src)) {
        pushIntoWorklist(dst);
        addSccCandidate(dst);
    }
}


/*!
 *
 */
//void AndersenSCD::handleCopyGep(ConstraintNode* node) {
//    double propStart = stat->getClk();
//
//    NodeID nodeId = node->getId();
//    for (ConstraintNode::const_iterator it = node->directOutEdgeBegin(), eit = node->directOutEdgeEnd(); it != eit;
//         ++it) {
//        if(CopyCGEdge* copyEdge = SVFUtil::dyn_cast<CopyCGEdge>(*it))
//            processCopy(nodeId,copyEdge);
//        else if(GepCGEdge* gepEdge = SVFUtil::dyn_cast<GepCGEdge>(*it))
//            processGep(nodeId,gepEdge);
//    }
//
//    if (!node->getLoadInEdges().empty() || !node->getStoreOutEdges().empty())
//        indirectNodes.push(node->getId());
//
//    double propEnd = stat->getClk();
//    timeOfProcessCopyGep += (propEnd - propStart) / TIMEINTERVAL;
//}