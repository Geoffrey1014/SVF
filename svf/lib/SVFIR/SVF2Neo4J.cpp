#include "SVFIR/SVF2Neo4J.h"
#include "Graphs/CHG.h"
#include "SVFIR/SVFIR.h"
#include "Util/CommandLine.h"
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <Python.h>


// static const Option<bool> humanReadableOption(
//     "human-readable", "Whether to output human-readable JSON", true);

namespace SVF
{
// TODO: 找个小的数据结构写进database

// DbItem* SVFIRDbWriter::toItem(const SVFModule* module)
// {
//     DbItem* root = neo4jItemManager->itemCreateArray();
//     DbItem* allSVFType = neo4jItemManager->itemCreateArray();
//     DbItem* allStInfo = neo4jItemManager->itemCreateArray();
//     DbItem* allSVFValue = neo4jItemManager->itemCreateArray();

//     for (const SVFType* svfType : svfModuleWriter.svfTypePool)
//     {
//         DbItem* svfTypeObj = virtToItem(svfType);
//         neo4jItemManager->itemAddItemToArray(allSVFType, svfTypeObj);
//     }

//     for (const StInfo* stInfo : svfModuleWriter.stInfoPool)
//     {
//         DbItem* stInfoObj = contentToItem(stInfo);
//         neo4jItemManager->itemAddItemToArray(allStInfo, stInfoObj);
//     }

//     #define F(field) DB_WRITE_FIELD(root, module, field)
//         neo4jItemManager->itemAddItemToObject(root, FIELD_NAME_ITEM(allSVFType));  // Meta field
//         neo4jItemManager->itemAddItemToObject(root, FIELD_NAME_ITEM(allStInfo));   // Meta field
//         neo4jItemManager->itemAddItemToObject(root, FIELD_NAME_ITEM(allSVFValue)); // Meta field
//         F(pagReadFromTxt);
//         F(moduleIdentifier);

//         F(FunctionSet);
//         F(GlobalSet);
//         F(AliasSet);
//         F(ConstantSet);
//         F(OtherValueSet);
//     #undef F

//     for (size_t i = 1; i <= svfModuleWriter.sizeSVFValuePool(); ++i)
//     {
//         DbItem* value = virtToItem(svfModuleWriter.getSVFValuePtr(i));
//         neo4jItemManager->itemAddItemToArray(allSVFValue, value);
//     }

//     return root;
// }


DbItem* SVFIRDbWriter::toItem(const SVFType* type)
{
    return neo4jItemManager->itemCreateIndex(svfModuleWriter.getSVFTypeID(type));
}

DbItem* SVFIRDbWriter::toItem(const SVFValue* value)
{
    return neo4jItemManager->itemCreateIndex(svfModuleWriter.getSVFValueID(value));
}

// cITEM* SVFIRDbWriter::toItem(const IRGraph* graph)
// {
//     ENSURE_NOT_VISITED(graph);

//     cJSON* root = genericGraphToJson(graph, irGraphWriter.edgePool.getPool());
// #define F(field) JSON_WRITE_FIELD(root, graph, field)
//     F(KindToSVFStmtSetMap);
//     F(KindToPTASVFStmtSetMap);
//     F(fromFile);
//     F(nodeNumAfterPAGBuild);
//     F(totalPTAPAGEdge);
//     F(valueToEdgeMap);
// #undef F
//     return root;
// }

DbItem* SVFIRDbWriter::toItem(const SVFVar* var)
{
    return var ? neo4jItemManager->itemCreateIndex(var->getId()) : neo4jItemManager->itemCreateNullId();
}

DbItem* SVFIRDbWriter::toItem(const SVFStmt* stmt)
{
    return neo4jItemManager->itemCreateIndex(irGraphWriter.getEdgeID(stmt));
}

DbItem* SVFIRDbWriter::virtToItem(const ICFGNode* node)
{

    switch (node->getNodeKind())
    {
    default:
        assert(false && "Unknown ICFGNode kind");
        break;
    case ICFGNode::IntraBlock:
        return contentToItem(static_cast<const IntraICFGNode*>(node));
    case ICFGNode::FunEntryBlock:
        return contentToItem(static_cast<const FunEntryICFGNode*>(node));
    case ICFGNode::FunExitBlock:
        return contentToItem(static_cast<const FunExitICFGNode*>(node));
    case ICFGNode::FunCallBlock:
        return contentToItem(static_cast<const CallICFGNode*>(node));
    case ICFGNode::FunRetBlock:
        return contentToItem(static_cast<const RetICFGNode*>(node));
    case ICFGNode::GlobalBlock:
        return contentToItem(static_cast<const GlobalICFGNode*>(node));
    }
    
}

DbItem* SVFIRDbWriter::toItem(const ICFG* icfg)
{
    DbItem* allSvfLoop = neo4jItemManager->itemCreateArray(); // all indices seen in constructor
    printf("allSvfLoop: %d\n", allSvfLoop->type);
    printf("icfgWriter.svfLoopPool.size(): %zu\n", icfgWriter.svfLoopPool.size());
    for (const SVFLoop* svfLoop : icfgWriter.svfLoopPool)
    {
        DbItem* svfLoopObj = contentToItem(svfLoop);
        printf("svfLoopObj: %d\n", svfLoopObj->type);
        neo4jItemManager->itemAddItemToArray(allSvfLoop, svfLoopObj);
    }
    DbItem* root = genericGraphToDb(icfg, icfgWriter.edgePool.getPool());
    neo4jItemManager->itemAddItemToObject(root, FIELD_NAME_ITEM(allSvfLoop));

// #define F(field) DB_WRITE_FIELD(root, icfg, field)
//     DbItem* root = genericGraphToDb(icfg, icfgWriter.edgePool.getPool());
//     neo4jItemManager->itemAddItemToObject(root, FIELD_NAME_ITEM(allSvfLoop)); // Meta field
//     F(totalICFGNode);
//     F(FunToFunEntryNodeMap);
//     F(FunToFunExitNodeMap);
//     F(CSToCallNodeMap);
//     F(CSToRetNodeMap);
//     F(InstToBlockNodeMap);
//     F(globalBlockNode);
//     F(icfgNodeToSVFLoopVec);
// #undef F
    return root;
}


DbItem* SVFIRDbWriter::contentToItem(const GlobalICFGNode* node)
{
    return contentToItem(static_cast<const ICFGNode*>(node));
}

DbItem* SVFIRDbWriter::contentToItem(const IntraICFGNode* node)
{
    DbItem* root = contentToItem(static_cast<const ICFGNode*>(node));
    ITEM_WRITE_FIELD(root, node, inst);
    return root;
}

DbItem* SVFIRDbWriter::contentToItem(const InterICFGNode* node)
{
    return contentToItem(static_cast<const ICFGNode*>(node));
}

DbItem* SVFIRDbWriter::contentToItem(const FunEntryICFGNode* node)
{
    DbItem* root = contentToItem(static_cast<const ICFGNode*>(node));
    ITEM_WRITE_FIELD(root, node, FPNodes);
    return root;
}

DbItem* SVFIRDbWriter::contentToItem(const FunExitICFGNode* node)
{
    DbItem* root = contentToItem(static_cast<const ICFGNode*>(node));
    ITEM_WRITE_FIELD(root, node, formalRet);
    return root;
}

DbItem* SVFIRDbWriter::contentToItem(const CallICFGNode* node)
{
    DbItem* root = contentToItem(static_cast<const ICFGNode*>(node));
    ITEM_WRITE_FIELD(root, node, cs);
    ITEM_WRITE_FIELD(root, node, ret);
    ITEM_WRITE_FIELD(root, node, APNodes);
    return root;
}

DbItem* SVFIRDbWriter::contentToItem(const RetICFGNode* node)
{
    DbItem* root = contentToItem(static_cast<const ICFGNode*>(node));
    ITEM_WRITE_FIELD(root, node, cs);
    ITEM_WRITE_FIELD(root, node, actualRet);
    ITEM_WRITE_FIELD(root, node, callBlockNode);
    return root;
}

DbItem* SVFIRDbWriter::contentToItem(const ICFGNode* node)
{
    DbItem* root = genericNodeToDb(node);
    ITEM_WRITE_FIELD(root, node, fun);
    ITEM_WRITE_FIELD(root, node, bb);
    // TODO: Ensure this?
    assert(node->VFGNodes.empty() && "VFGNodes list not empty?");
    ITEM_WRITE_FIELD(root, node, pagEdges);
    return root;
}


DbItem* SVFIRDbWriter::toItem(const ICFGNode* node)
{
    assert(node && "ICFGNode is null!");
    return neo4jItemManager->itemCreateIndex(node->getId());
}

DbItem* SVFIRDbWriter::toItem(const ICFGEdge* edge)
{
    assert(edge && "ICFGNode is null!");
    return neo4jItemManager->itemCreateIndex(icfgWriter.getEdgeID(edge));
}


DbItem* SVFIRDbWriter::toItem(bool flag)
{
    return neo4jItemManager->itemCreateBool(flag);
}

DbItem* SVFIRDbWriter::toItem(unsigned number)
{
    // OK, double precision enough
    return neo4jItemManager->itemCreateNumber(number);
}

DbItem* SVFIRDbWriter::toItem(int number)
{
    // OK, double precision enough
    return neo4jItemManager->itemCreateNumber(number);
}


DbItem* SVFIRDbWriter::toItem(const std::string& str)
{
    return neo4jItemManager->itemCreateString(str.c_str());
}

DbItem* SVFIRDbWriter::toItem(float number)
{
    return neo4jItemManager->itemCreateNumber(number);
}

DbItem* SVFIRDbWriter::toItem(unsigned long number)
{
    // unsigned long is subset of unsigned long long
    return toItem(static_cast<unsigned long long>(number));
}

DbItem* SVFIRDbWriter::toItem(long long number)
{
    return toItem(static_cast<unsigned long long>(number));
}

DbItem* SVFIRDbWriter::toItem(unsigned long long number)
{
    return neo4jItemManager->itemCreateString(numToStr(number));
}

const char* SVFIRDbWriter::numToStr(size_t n)
{
    auto it = numToStrMap.find(n);
    if (it != numToStrMap.end() && !(numToStrMap.key_comp()(n, it->first)))
    {
        return it->second.c_str();
    }
    return numToStrMap.emplace_hint(it, n, std::to_string(n))->second.c_str();
}


DbItem* SVFIRDbWriter::contentToItem(const SVFLoop* loop)
{
    DbItem* root = neo4jItemManager->itemCreateArray();
    #define F(field) DB_WRITE_FIELD(root, loop, field)
    F(entryICFGEdges);
    F(backICFGEdges);
    F(inICFGEdges);
    F(outICFGEdges);
    F(icfgNodes);
    F(loopBound);
    #undef F

    return root;
}

const char*  SVFIRDbWriter::generateDataBaseItems()
{
    // const IRGraph* const irGraph = svfIR;
    NodeIDAllocator* nodeIDAllocator = NodeIDAllocator::allocator;
    assert(nodeIDAllocator && "NodeIDAllocator is not initialized?");

    // autoITEM object = generateItems();

    // try icfgWriter 
    
    DbItem* root = neo4jItemManager->itemCreateObject();
    #define F(field) DB_WRITE_FIELD(root, svfIR, field)
        // F(svfModule);
        // F(symInfo);
        F(icfg);
        // F(chgraph);
        // jsonAddJsonableToObject(root, FIELD_NAME_ITEM(irGraph));
        // F(icfgNode2SVFStmtsMap);
        // F(icfgNode2PTASVFStmtsMap);
        // F(GepValObjMap);
        // F(typeLocSetsMap);
        // F(GepObjVarMap);
        // F(memToFieldsMap);
        // F(globSVFStmtSet);
        // F(phiNodeMap);
        // F(funArgsListMap);
        // F(callSiteArgsListMap);
        // F(callSiteRetMap);
        // F(funRetMap);
        // F(indCallSiteToFunPtrMap);
        // F(funPtrToCallSitesMap);
        // F(candidatePointers);
        // F(callSiteSet);
        // jsonAddJsonableToObject(root, FIELD_NAME_ITEM(nodeIDAllocator));
    #undef F

    const char* str =  "generateDataBaseItems!\n";
    return str;
}



void SVFIRDbWriter::writeToDatabase(const SVFIR* svfir, const std::string& path)
{

    SVFIRDbWriter writer(svfir);
    writer.generateDataBaseItems();
}

SVFIRDbWriter::SVFIRDbWriter(const SVFIR* svfir)
    : svfIR(svfir), svfModuleWriter(svfir->svfModule), icfgWriter(svfir->icfg),
      chgWriter(SVFUtil::dyn_cast<CHGraph>(svfir->chgraph)),
      irGraphWriter(svfir)
{
    const char* uri = "neo4j+s://4291d08d.databases.neo4j.io";
    const char* username = "neo4j";
    const char* password = "ghfQKOPyySepbGDVuGgWsJLhhHP2R-ukd3tbvT9QNu8";
    db = new Neo4jClient(uri, username, password);
    neo4jItemManager = new Neo4jItemManager(db);
}

ICFGDbWriter::ICFGDbWriter(const ICFG* icfg) : GenericICFGDbWriter(icfg)
{
    for (const auto& pair : icfg->getIcfgNodeToSVFLoopVec())
    {
        for (const SVFLoop* loop : pair.second)
        {
            svfLoopPool.saveID(loop);
        }
    }
}

SVFModuleDbWriter::SVFModuleDbWriter(const SVFModule* svfModule)
{
    // TODO: SVFType & StInfo are managed by SymbolTableInfo. Refactor it?
    auto symTab = SymbolTableInfo::SymbolInfo();

    const auto& svfTypes = symTab->getSVFTypes();
    svfTypePool.reserve(svfTypes.size());
    for (const SVFType* type : svfTypes)
    {
        svfTypePool.saveID(type);
    }

    const auto& stInfos = symTab->getStInfos();
    stInfoPool.reserve(stInfos.size());
    for (const StInfo* stInfo : stInfos)
    {
        stInfoPool.saveID(stInfo);
    }

    svfValuePool.reserve(svfModule->getFunctionSet().size() +
                         svfModule->getConstantSet().size() +
                         svfModule->getOtherValueSet().size());
}



}