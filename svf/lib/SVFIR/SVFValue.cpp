#include "SVFIR/SVFValue.h"
#include "Util/SVFUtil.h"
#include "Graphs/GenericGraph.h"

using namespace SVF;
using namespace SVFUtil;

/// Add field (index and offset) with its corresponding type
void StInfo::addFldWithType(u32_t fldIdx, const SVFType* type, u32_t elemIdx)
{
    fldIdxVec.push_back(fldIdx);
    elemIdxVec.push_back(elemIdx);
    fldIdx2TypeMap[fldIdx] = type;
}

///  struct A { int id; int salary; }; struct B { char name[20]; struct A a;}   B b;
///  OriginalFieldType of b with field_idx 1 : Struct A
///  FlatternedFieldType of b with field_idx 1 : int
//{@
const SVFType* StInfo::getOriginalElemType(u32_t fldIdx) const
{
    Map<u32_t, const SVFType*>::const_iterator it = fldIdx2TypeMap.find(fldIdx);
    if(it!=fldIdx2TypeMap.end())
        return it->second;
    return nullptr;
}

const SVFLoopAndDomInfo::LoopBBs& SVFLoopAndDomInfo::getLoopInfo(const SVFBasicBlock* bb) const
{
    assert(hasLoopInfo(bb) && "loopinfo does not exist (bb not in a loop)");
    Map<const SVFBasicBlock*, LoopBBs>::const_iterator mapIter = bb2LoopMap.find(bb);
    return mapIter->second;
}

void SVFLoopAndDomInfo::getExitBlocksOfLoop(const SVFBasicBlock* bb, BBList& exitbbs) const
{
    if (hasLoopInfo(bb))
    {
        const LoopBBs blocks = getLoopInfo(bb);
        if (!blocks.empty())
        {
            for (const SVFBasicBlock* block : blocks)
            {
                for (const SVFBasicBlock* succ : block->getSuccessors())
                {
                    if ((std::find(blocks.begin(), blocks.end(), succ)==blocks.end()))
                        exitbbs.push_back(succ);
                }
            }
        }
    }
}

bool SVFLoopAndDomInfo::dominate(const SVFBasicBlock* bbKey, const SVFBasicBlock* bbValue) const
{
    if (bbKey == bbValue)
        return true;

    // An unreachable node is dominated by anything.
    if (isUnreachable(bbValue))
    {
        return true;
    }

    // And dominates nothing.
    if (isUnreachable(bbKey))
    {
        return false;
    }

    const Map<const SVFBasicBlock*,BBSet>& dtBBsMap = getDomTreeMap();
    Map<const SVFBasicBlock*,BBSet>::const_iterator mapIter = dtBBsMap.find(bbKey);
    if (mapIter != dtBBsMap.end())
    {
        const BBSet & dtBBs = mapIter->second;
        if (dtBBs.find(bbValue) != dtBBs.end())
        {
            return true;
        }
    }

    return false;
}

bool SVFLoopAndDomInfo::postDominate(const SVFBasicBlock* bbKey, const SVFBasicBlock* bbValue) const
{
    if (bbKey == bbValue)
        return true;

    // An unreachable node is dominated by anything.
    if (isUnreachable(bbValue))
    {
        return true;
    }

    // And dominates nothing.
    if (isUnreachable(bbKey))
    {
        return false;
    }

    const Map<const SVFBasicBlock*,BBSet>& dtBBsMap = getPostDomTreeMap();
    Map<const SVFBasicBlock*,BBSet>::const_iterator mapIter = dtBBsMap.find(bbKey);
    if (mapIter != dtBBsMap.end())
    {
        const BBSet & dtBBs = mapIter->second;
        if (dtBBs.find(bbValue) != dtBBs.end())
        {
            return true;
        }
    }
    return false;
}

const SVFBasicBlock* SVFLoopAndDomInfo::findNearestCommonPDominator(const SVFBasicBlock* A, const SVFBasicBlock* B) const
{
    assert(A && B && "Pointers are not valid");
    assert(A->getParent() == B->getParent() &&
           "Two blocks are not in same function");

    // Use level information to go up the tree until the levels match. Then
    // continue going up til we arrive at the same node.
    while (A != B)
    {
        // no common PDominator
        if(A == NULL) return NULL;
        const auto lvA = getBBPDomLevel().find(A);
        const auto lvB = getBBPDomLevel().find(B);
        assert(lvA != getBBPDomLevel().end() && lvB != getBBPDomLevel().end());

        if (lvA->second < lvB->second) std::swap(A, B);

        const auto lvAIdom = getBB2PIdom().find(A);
        assert(lvAIdom != getBB2PIdom().end());
        A = lvAIdom->second;
    }

    return A;
}

bool SVFLoopAndDomInfo::isLoopHeader(const SVFBasicBlock* bb) const
{
    if (hasLoopInfo(bb))
    {
        const LoopBBs& blocks = getLoopInfo(bb);
        assert(!blocks.empty() && "no available loop info?");
        return blocks.front() == bb;
    }
    return false;
}

SVFFunction::SVFFunction(const SVFType* ty, const SVFFunctionType* ft,
                         bool declare, bool intrinsic, bool adt, bool varg,
                         SVFLoopAndDomInfo* ld)
    : SVFValue(ty, SVFValue::SVFFunc), isDecl(declare), intrinsic(intrinsic),
      addrTaken(adt), isUncalled(false), isNotRet(false), varArg(varg),
      funcType(ft), loopAndDom(ld), realDefFun(nullptr), exitBlock(nullptr)
{
}

SVFFunction::~SVFFunction()
{
    for(const SVFArgument* arg : allArgs)
        delete arg;
    delete loopAndDom;
    delete bbGraph;
}

u32_t SVFFunction::arg_size() const
{
    return allArgs.size();
}

const SVFArgument* SVFFunction::getArg(u32_t idx) const
{
    assert (idx < allArgs.size() && "getArg() out of range!");
    return allArgs[idx];
}

bool SVFFunction::isVarArg() const
{
    return varArg;
}

const SVFBasicBlock *SVFFunction::getExitBB() const
{
    assert(hasBasicBlock() && "function does not have any Basicblock, external function?");
    assert(exitBlock && "must have an exitBlock");
    return exitBlock;
}

void SVFFunction::setExitBlock(SVFBasicBlock *bb)
{
    assert(!exitBlock && "have already set exit Basicblock!");
    exitBlock = bb;
}


SVFInstruction::SVFInstruction(const SVFType* ty, const SVFBasicBlock* b,
                               bool tm, bool isRet, SVFValKind k)
    : SVFValue(ty, k), bb(b), terminator(tm), ret(isRet)
{
}

__attribute__((weak))
std::string SVFValue::toString() const
{
    assert("SVFValue::toString should be implemented or supported by fronted" && false);
    abort();
}

__attribute__((weak))
const std::string SVFBaseNode::valueOnlyToString() const
{
    assert("SVFBaseNode::valueOnlyToString should be implemented or supported by fronted" && false);
    abort();
}