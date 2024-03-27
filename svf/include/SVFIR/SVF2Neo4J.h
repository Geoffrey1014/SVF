//===- SVF2Neo4J.h -- SVF IR Reader and Writer ------------------------===//
//
//  SVF - Static Value-Flow Analysis Framework
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2024>  <Yulei Sui>
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

#ifndef INCLUDE_SVF2NEO4J_H_
#define INCLUDE_SVF2NEO4J_H_

#include "Graphs/GenericGraph.h"
#include "Util/SVFUtil.h"
// #include "Util/cJSON.h"
#include <type_traits>
#include "SVFIR/Neo4jClient.h"


namespace SVF
{
/// @brief Forward declarations.
///@{
class NodeIDAllocator;
// Classes created upon SVFMoudle construction
class SVFType;
class SVFPointerType;
class SVFIntegerType;
class SVFFunctionType;
class SVFStructType;
class SVFArrayType;
class SVFOtherType;

class StInfo; // Every SVFType is linked to a StInfo. It also references SVFType

class SVFValue;
class SVFFunction;
class SVFBasicBlock;
class SVFInstruction;
class SVFCallInst;
class SVFVirtualCallInst;
class SVFConstant;
class SVFGlobalValue;
class SVFArgument;
class SVFConstantData;
class SVFConstantInt;
class SVFConstantFP;
class SVFConstantNullPtr;
class SVFBlackHoleValue;
class SVFOtherValue;
class SVFMetadataAsValue;

class SVFLoopAndDomInfo; // Part of SVFFunction

// Classes created upon buildSymbolTableInfo
class MemObj;

// Classes created upon ICFG construction
class ICFGNode;
class GlobalICFGNode;
class IntraICFGNode;
class InterICFGNode;
class FunEntryICFGNode;
class FunExitICFGNode;
class CallICFGNode;
class RetICFGNode;

class ICFGEdge;
class IntraCFGEdge;
class CallCFGEdge;
class RetCFGEdge;

class SVFIR;
class SVFIRWriter;
class SVFLoop;
class ICFG;
class IRGraph;
class CHGraph;
class CommonCHGraph;
class SymbolTableInfo;

class SVFModule;

class AccessPath;
class ObjTypeInfo; // Need SVFType

class SVFVar;
class ValVar;
class ObjVar;
class GepValVar;
class GepObjVar;
class FIObjVar;
class RetPN;
class VarArgPN;
class DummyValVar;
class DummyObjVar;

class SVFStmt;
class AssignStmt;
class AddrStmt;
class CopyStmt;
class StoreStmt;
class LoadStmt;
class GepStmt;
class CallPE;
class RetPE;
class MultiOpndStmt;
class PhiStmt;
class SelectStmt;
class CmpStmt;
class BinaryOPStmt;
class UnaryOPStmt;
class BranchStmt;
class TDForkPE;
class TDJoinPE;

class CHNode;
class CHEdge;
class CHGraph;
// End of forward declarations
///@}

/// @brief Bookkeeping class to keep track of the IDs of objects that doesn't
/// have any ID. E.g., SVFValue, XXXEdge.
/// @tparam T
template <typename T> 
class WriterDbPtrPool
{
private:
    Map<const T*, size_t> ptrToId;
    std::vector<const T*> ptrPool;

public:
    inline size_t getID(const T* ptr)
    {
        if (!ptr)
            return 0;

        typename decltype(ptrToId)::iterator it;
        bool inserted;
        std::tie(it, inserted) = ptrToId.emplace(ptr, 1 + ptrPool.size());
        if (inserted)
            ptrPool.push_back(ptr);
        return it->second;
    }

    inline void saveID(const T* ptr)
    {
        getID(ptr);
    }

    inline const T* getPtr(size_t id) const
    {
        assert(id <= ptrPool.size() && "Invalid ID");
        return id ? ptrPool[id - 1] : nullptr;
    }

    inline const std::vector<const T*>& getPool() const
    {
        return ptrPool;
    }

    inline size_t size() const
    {
        return ptrPool.size();
    }

    inline void reserve(size_t size)
    {
        ptrPool.reserve(size);
    }

    inline auto begin() const
    {
        return ptrPool.cbegin();
    }

    inline auto end() const
    {
        return ptrPool.cend();
    }
};

template <typename NodeTy, typename EdgeTy> class GenericGraphDbWriter
{
    friend class SVFIRDbWriter;

private:
    using NodeType = NodeTy;
    using EdgeType = EdgeTy;
    using GraphType = GenericGraph<NodeType, EdgeType>;

    // const GraphType* graph;
    WriterDbPtrPool<EdgeType> edgePool;

public:
    GenericGraphDbWriter(const GraphType* graph)
    {
        assert(graph && "Graph pointer should never be null");
        edgePool.reserve(graph->getTotalEdgeNum());


        for (const auto& pair : graph->IDToNodeMap)
        {
            const NodeType* node = pair.second;

            for (const EdgeType* edge : node->getOutEdges())
            {
                edgePool.saveID(edge);
            }
        }
    }

    inline size_t getEdgeID(const EdgeType* edge)
    {
        return edgePool.getID(edge);
    }
};

using GenericICFGDbWriter = GenericGraphDbWriter<ICFGNode, ICFGEdge>;

class ICFGDbWriter : public GenericICFGDbWriter
{
    friend class SVFIRDbWriter;

private:
    WriterDbPtrPool<SVFLoop> svfLoopPool;

public:
    ICFGDbWriter(const ICFG* icfg);

    inline size_t getSvfLoopID(const SVFLoop* loop)
    {
        return svfLoopPool.getID(loop);
    }
};

using IRGraphDbWriter = GenericGraphDbWriter<SVFVar, SVFStmt>;
using CHGraphDbWriter = GenericGraphDbWriter<CHNode, CHEdge>;

class SVFModuleDbWriter
{
    friend class SVFIRDbWriter;

private:
    WriterDbPtrPool<SVFType> svfTypePool;
    WriterDbPtrPool<StInfo> stInfoPool;
    WriterDbPtrPool<SVFValue> svfValuePool;

public:
    SVFModuleDbWriter(const SVFModule* svfModule);

    inline size_t getSVFValueID(const SVFValue* value)
    {
        return svfValuePool.getID(value);
    }
    inline const SVFValue* getSVFValuePtr(size_t id) const
    {
        return svfValuePool.getPtr(id);
    }
    inline size_t getSVFTypeID(const SVFType* type)
    {
        return svfTypePool.getID(type);
    }
    inline size_t getStInfoID(const StInfo* stInfo)
    {
        return stInfoPool.getID(stInfo);
    }
    inline size_t sizeSVFValuePool() const
    {
        return svfValuePool.size();
    }
};


class SVFIRDbWriter
{
private:
    const SVFIR* svfIR;

    SVFModuleDbWriter svfModuleWriter;
    ICFGDbWriter icfgWriter;
    CHGraphDbWriter chgWriter;
    IRGraphDbWriter irGraphWriter;
    Neo4jClient* db;
    Neo4jItemManager* neo4jItemManager;

    OrderedMap<size_t, std::string> numToStrMap;

public:
    using autoITEM = std::unique_ptr<DbItem>;
    // using autoCStr = std::unique_ptr<char, decltype(&cITEM_free)>;

    /// @brief Constructor.
    SVFIRDbWriter(const SVFIR* svfir);

    static void writeToDatabase(const SVFIR* svfir, const std::string& path);

private:
    /// @brief Main logic to dump a SVFIR to a Database Items object.
    autoITEM generateItems();
    const char * generateDataBaseItems();

    const char* numToStr(size_t n);

    DbItem* toItem(const NodeIDAllocator* nodeIDAllocator);
    DbItem* toItem(const SymbolTableInfo* symTable);
    DbItem* toItem(const SVFModule* module);
    DbItem* toItem(const SVFType* type);
    DbItem* toItem(const SVFValue* value);
    DbItem* toItem(const IRGraph* graph);       // IRGraph Graph
    DbItem* toItem(const SVFVar* var);          // IRGraph Node
    DbItem* toItem(const SVFStmt* stmt);        // IRGraph Edge
    DbItem* toItem(const ICFG* icfg);           // ICFG Graph
    DbItem* toItem(const ICFGNode* node);       // ICFG Node
    DbItem* toItem(const ICFGEdge* edge);       // ICFG Edge
    DbItem* toItem(const CommonCHGraph* graph); // CHGraph Graph
    DbItem* toItem(const CHGraph* graph);       // CHGraph Graph
    DbItem* toItem(const CHNode* node);         // CHGraph Node
    DbItem* toItem(const CHEdge* edge);         // CHGraph Edge

    DbItem* toItem(const CallSite& cs);
    DbItem* toItem(const AccessPath& ap);
    DbItem* toItem(const SVFLoop* loop);
    DbItem* toItem(const MemObj* memObj);
    DbItem* toItem(const ObjTypeInfo* objTypeInfo);  // Only owned by MemObj
    DbItem* toItem(const SVFLoopAndDomInfo* ldInfo); // Only owned by SVFFunction
    DbItem* toItem(const StInfo* stInfo);

    // No need for 'toItem(short)' because of promotion to int
    DbItem* toItem(bool flag);
    DbItem* toItem(unsigned number);
    DbItem* toItem(int number);
    DbItem* toItem(float number);
    DbItem* toItem(const std::string& str);
    DbItem* toItem(unsigned long number);
    DbItem* toItem(long long number);
    DbItem* toItem(unsigned long long number);

    /// \brief Parameter types of these functions are all pointers.
    /// When they are used as arguments of toItem(), they will be
    /// dumped as an index. `contentToItem()` will dump the actual content.
    ///@{
    DbItem* virtToItem(const SVFType* type);
    DbItem* virtToItem(const SVFValue* value);
    DbItem* virtToItem(const SVFVar* var);
    DbItem* virtToItem(const SVFStmt* stmt);
    DbItem* virtToItem(const ICFGNode* node);
    DbItem* virtToItem(const ICFGEdge* edge);
    DbItem* virtToItem(const CHNode* node);
    DbItem* virtToItem(const CHEdge* edge);

    // Classes inherited from SVFVar
    DbItem* contentToItem(const SVFVar* var);
    DbItem* contentToItem(const ValVar* var);
    DbItem* contentToItem(const ObjVar* var);
    DbItem* contentToItem(const GepValVar* var);
    DbItem* contentToItem(const GepObjVar* var);
    DbItem* contentToItem(const FIObjVar* var);
    DbItem* contentToItem(const RetPN* var);
    DbItem* contentToItem(const VarArgPN* var);
    DbItem* contentToItem(const DummyValVar* var);
    DbItem* contentToItem(const DummyObjVar* var);

    // Classes inherited from SVFStmt
    DbItem* contentToItem(const SVFStmt* edge);
    DbItem* contentToItem(const AssignStmt* edge);
    DbItem* contentToItem(const AddrStmt* edge);
    DbItem* contentToItem(const CopyStmt* edge);
    DbItem* contentToItem(const StoreStmt* edge);
    DbItem* contentToItem(const LoadStmt* edge);
    DbItem* contentToItem(const GepStmt* edge);
    DbItem* contentToItem(const CallPE* edge);
    DbItem* contentToItem(const RetPE* edge);
    DbItem* contentToItem(const MultiOpndStmt* edge);
    DbItem* contentToItem(const PhiStmt* edge);
    DbItem* contentToItem(const SelectStmt* edge);
    DbItem* contentToItem(const CmpStmt* edge);
    DbItem* contentToItem(const BinaryOPStmt* edge);
    DbItem* contentToItem(const UnaryOPStmt* edge);
    DbItem* contentToItem(const BranchStmt* edge);
    DbItem* contentToItem(const TDForkPE* edge);
    DbItem* contentToItem(const TDJoinPE* edge);

    // Classes inherited from ICFGNode
    DbItem* contentToItem(const ICFGNode* node);
    DbItem* contentToItem(const GlobalICFGNode* node);
    DbItem* contentToItem(const IntraICFGNode* node);
    DbItem* contentToItem(const InterICFGNode* node);
    DbItem* contentToItem(const FunEntryICFGNode* node);
    DbItem* contentToItem(const FunExitICFGNode* node);
    DbItem* contentToItem(const CallICFGNode* node);
    DbItem* contentToItem(const RetICFGNode* node);

    // Classes inherited from ICFGEdge
    DbItem* contentToItem(const ICFGEdge* edge);
    DbItem* contentToItem(const IntraCFGEdge* edge);
    DbItem* contentToItem(const CallCFGEdge* edge);
    DbItem* contentToItem(const RetCFGEdge* edge);

    // CHNode & CHEdge
    DbItem* contentToItem(const CHNode* node);
    DbItem* contentToItem(const CHEdge* edge);

    DbItem* contentToItem(const SVFType* type);
    DbItem* contentToItem(const SVFPointerType* type);
    DbItem* contentToItem(const SVFIntegerType* type);
    DbItem* contentToItem(const SVFFunctionType* type);
    DbItem* contentToItem(const SVFStructType* type);
    DbItem* contentToItem(const SVFArrayType* type);
    DbItem* contentToItem(const SVFOtherType* type);

    DbItem* contentToItem(const SVFValue* value);
    DbItem* contentToItem(const SVFFunction* value);
    DbItem* contentToItem(const SVFBasicBlock* value);
    DbItem* contentToItem(const SVFInstruction* value);
    DbItem* contentToItem(const SVFCallInst* value);
    DbItem* contentToItem(const SVFVirtualCallInst* value);
    DbItem* contentToItem(const SVFConstant* value);
    DbItem* contentToItem(const SVFGlobalValue* value);
    DbItem* contentToItem(const SVFArgument* value);
    DbItem* contentToItem(const SVFConstantData* value);
    DbItem* contentToItem(const SVFConstantInt* value);
    DbItem* contentToItem(const SVFConstantFP* value);
    DbItem* contentToItem(const SVFConstantNullPtr* value);
    DbItem* contentToItem(const SVFBlackHoleValue* value);
    DbItem* contentToItem(const SVFOtherValue* value);
    DbItem* contentToItem(const SVFMetadataAsValue* value);

    // Other classes
    DbItem* contentToItem(const SVFLoop* loop);
    DbItem* contentToItem(const MemObj* memObj); // Owned by SymbolTable->objMap
    DbItem* contentToItem(const StInfo* stInfo);
    ///@}

    template <typename NodeTy, typename EdgeTy>
    DbItem* genericNodeToDb(const GenericNode<NodeTy, EdgeTy>* node)
    {
        DbItem* root = neo4jItemManager->itemCreateObject();
        ITEM_WRITE_FIELD(root, node, id);
        ITEM_WRITE_FIELD(root, node, nodeKind);
        ITEM_WRITE_FIELD(root, node, InEdges);
        ITEM_WRITE_FIELD(root, node, OutEdges);
        return root;
    }

    template <typename NodeTy>
    DbItem* genericEdgeToDb(const GenericEdge<NodeTy>* edge)
    {
        DbItem* root = neo4jItemManager->itemCreateObject();
        ITEM_WRITE_FIELD(root, edge, edgeFlag);
        ITEM_WRITE_FIELD(root, edge, src);
        ITEM_WRITE_FIELD(root, edge, dst);
        return root;
    }

    template <typename NodeTy, typename EdgeTy>
    DbItem* genericGraphToDb(const GenericGraph<NodeTy, EdgeTy>* graph,
                              const std::vector<const EdgeTy*>& edgePool)
    {
        DbItem* root = neo4jItemManager->itemCreateObject();

        DbItem* allNode = neo4jItemManager->itemCreateArray();
        for (const auto& pair : graph->IDToNodeMap)
        {
            NodeTy* node = pair.second;
            
            DbItem* itemNode = virtToItem(node);
            itemNode->printItem();
            // neo4jItemManager->itemAddItemToArray(allNode, itemNode);
        }

        DbItem* allEdge = neo4jItemManager->itemCreateArray();
        for (const EdgeTy* edge : edgePool)
        {   printf("EdgeTy: %p\n", edge);
            // DbItem* edgeDb = virtToItem(edge);
            // neo4jItemManager->itemAddItemToArray(allEdge, edgeDb);
        }

        ITEM_WRITE_FIELD(root, graph, nodeNum);
        neo4jItemManager->itemAddItemToObject(root, FIELD_NAME_ITEM(allNode));
        ITEM_WRITE_FIELD(root, graph, edgeNum);
        neo4jItemManager->itemAddItemToObject(root, FIELD_NAME_ITEM(allEdge));

        return root;
    }

    /** The following 2 functions are intended to convert SparseBitVectors
     * to JSON. But they're buggy. Commenting them out would enable the
     * toItem(T) where is_iterable_v<T> is true. But that implementation is less
     * space-efficient if the bitvector contains many elements.
     * It is observed that upon construction, SVF IR bitvectors contain at most
     * 1 element. In that case, we can just use the toItem(T) for iterable T
     * without much space overhead.

    template <unsigned ElementSize>
    DbItem* toItem(const SparseBitVectorElement<ElementSize>& element)
    {
        DbItem* array = itemCreateArray();
        for (const auto v : element.Bits)
        {
            itemAddItemToArray(array, toItem(v));
        }
        return array;
    }

    template <unsigned ElementSize>
    DbItem* toItem(const SparseBitVector<ElementSize>& bv)
    {
        return toItem(bv.Elements);
    }
    */

    template <typename T, typename U> DbItem* toItem(const std::pair<T, U>& pair)
    {
        DbItem* obj = neo4jItemManager->itemCreateArray();
        neo4jItemManager->itemAddItemToArray(obj, toItem(pair.first));
        neo4jItemManager->itemAddItemToArray(obj, toItem(pair.second));
        return obj;
    }

    template <typename T,
              typename = std::enable_if_t<SVFUtil::is_iterable_v<T>>>
                                          DbItem* toItem(const T& container)
    {
        DbItem* array = neo4jItemManager->itemCreateArray();
        for (const auto& item : container)
        {
            DbItem* itemObj = toItem(item);
            neo4jItemManager->itemAddItemToArray(array, itemObj);
        }
        return array;
    }

    template <typename T>
    bool itemAddItemableToObject(DbItem* obj, const char* name, const T& item)
    {
        DbItem* itemObj = toItem(item);
        return neo4jItemManager->itemAddItemToObject(obj, name, itemObj);
    }

    template <typename T>
    bool itemAddContentToObject(DbItem* obj, const char* name, const T& item)
    {
        DbItem* itemObj = contentToItem(item);
        return neo4jItemManager->itemAddItemToObject(obj, name, itemObj);
    }
};

/*
 * Reader Part
 */





} // namespace SVF

#endif // !INCLUDE_SVF2NEO4J_H_
