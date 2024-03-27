//===- Neo4jClient.h -----------------------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->2024>  <Yulei Sui>
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


#include <Python.h>
#include <string>
#include <vector>
#include "Util/cJSON.h"

#ifndef INCLUDE_NEO4JCLIENT_H_
#define INCLUDE_NEO4JCLIENT_H_

#define ABORT_MSG(reason)                                                      \
    do                                                                         \
    {                                                                          \
        SVFUtil::errs() << __FILE__ << ':' << __LINE__ << ": " << reason       \
                        << '\n';                                               \
        abort();                                                               \
    } while (0)
#define ABORT_IFNOT(condition, reason)                                         \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
            ABORT_MSG(reason);                                                 \
    } while (0)


#define FIELD_NAME_ITEM(field) #field, (field)

#define ITEM_FIELD_OR(item, field, default) ((item) ? (item)->field : default)
#define ITEM_KEY(item) ITEM_FIELD_OR(item, string, "NULL")
#define ITEM_CHILD(item) JSON_FIELD_OR(item, child, nullptr)

#define ITEM_WRITE_FIELD(root, objptr, field)                                  \
    itemAddItemableToObject(root, #field, (objptr)->field)

#define CITEM_PUBLIC(type) type

#define itemForEach(field, array)                                              \
        for (const DbItem* field = ITEM_CHILD(array); field; field = field->next)

#define DB_WRITE_FIELD(root, objptr, field)                                  \
        itemAddItemableToObject(root, #field, (objptr)->field)

namespace SVF{
/* dbITEM Types: */
#define dbITEM_Invalid (0)
#define dbITEM_False  (1 << 0)
#define dbITEM_True   (1 << 1)
#define dbITEM_NULL   (1 << 2)
#define dbITEM_Number (1 << 3)
#define dbITEM_String (1 << 4)
#define dbITEM_Array  (1 << 5)
#define dbITEM_Object (1 << 6)
#define dbITEM_Raw    (1 << 7) /* raw item */

class DbItem{
    public:
    /* next/prev allow you to walk array/object chains. */
    DbItem *next;
    DbItem *prev;

    std::vector<DbItem*> children;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==cITEM_String and type == cITEM_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cITEM_SetNumberValue instead */
    int valueint;
    /* The item's number, if type==cITEM_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;

    public:
    void printItem(){
        printf("DbItem details:\n");
        printf("type: %d\n", type);
        if (type & dbITEM_String){
            printf("valuestring: %s\n", valuestring);
        }
        if (type & dbITEM_Number){
            printf("valuedouble: %f\n", valuedouble);
        }
        if (type & dbITEM_Array){
            printf("Array size %ld: \n", children.size());   
        }
        if (type & dbITEM_Object){
            printf("It is an Object \n");
            
        }
    }
    DbItem(){
        next = prev = nullptr;
        type = dbITEM_Invalid;
        valuestring = nullptr;
        valueint = 0;
        valuedouble = 0.0;
        string = nullptr;
    }
    ~DbItem(){
        if (type & dbITEM_String){
            free(valuestring);
        }
        if (type & dbITEM_Array){
            for (auto child : children){
                delete child;
            }
        }
        if (type & dbITEM_Object){
            delete next;
        }
    }
    bool itemIsMap(const DbItem* item);
    inline bool itemIsObject(){
        return (type & dbITEM_Object) ? true : false;
    }
    inline bool itemIsArray(const DbItem* item){
        return (type & dbITEM_Array) ? true : false;
    }
    
    bool addItemToArray(DbItem *item)
    {

        if ((item == NULL) || (this == item))
        {
            return false;
        }

        this->children.push_back(item);

        return true;
    }


};

class DbNode : public DbItem{
    
private:
    const char* node_type;
    PyObject* node_properties;

public:
    // Constructor
    DbNode(const char* node_type, PyObject* node_properties) {
        // Set the node_type
        this->node_type = node_type;
        // Set the node_properties
        this->node_properties = node_properties;
    }

    // Destructor
    ~DbNode() {
        Py_DECREF(node_properties);
    }

    // Get the node_type
    const char* getNodetype() const {
        return node_type;
    }

    // Get the node_properties
    PyObject* getNodeProperties() const {
        return node_properties;
    }
};

class DbEdge : public DbItem{

private:
    const char* edge_type;
    PyObject* edge_properties;

public:
    // Constructor
    DbEdge(const char* edge_type, PyObject* edge_properties) {
        // Set the edge_type
        this->edge_type = edge_type;
        // Set the edge_properties
        this->edge_properties = edge_properties;
    }

    // Destructor
    ~DbEdge() {
        Py_DECREF(edge_properties);
    }

    // Get the edge_type
    const char* getEdgeType() const {
        return edge_type;
    }

    // Get the edge_properties
    PyObject* getEdgeProperties() const {
        return edge_properties;
    }
};

class Neo4jClient {

private:
    PyObject* pInstance;

public:
    // Constructor
    Neo4jClient(const char* uri, const char* username, const char* password);
    // Destructor
    ~Neo4jClient();

    DbNode createNodeJson(const char* graph_id, const char* node_type, cJSON* jsonNode);

    DbNode createNode(const char* graph_id, const char* node_type, ...);

    DbEdge createEdgeJson(const char* graph_id, const char* edge_type, cJSON* edgeJson);

    DbEdge createEdge(const char* graph_id, const char* edge_type, ...);

    void writeNode(const DbNode& node);

    void writeEdge(const DbNode& node1, const DbNode& node2, const DbEdge& edge);

    void clearDatabase();
};

class Neo4jItemManager {
private:
    Neo4jClient * bd;

public:
    Neo4jItemManager(Neo4jClient * bd);
    ~Neo4jItemManager();

    DbItem* dbItem_New_Item();
    /* Delete a DbItem entity and all subentities. */
    void dbItem_Delete(DbItem *item);

    /* Render a DbItem entity to text for transfer/storage. */
    char * dbItem_Print(const DbItem *item);

    /* raw item */
    DbItem * dbItem_CreateRaw(const char *raw);
    DbItem * dbItem_CreateArray(void);
    DbItem * dbItem_CreateObject(void);
    bool itemIsBool(const DbItem* item);
    bool itemIsBool(const DbItem* item, bool& flag);
    bool itemIsNumber(const DbItem* item);
    bool itemIsString(const DbItem* item);
    bool itemIsNullId(const DbItem* item);
    bool itemIsArray(const DbItem* item);
    bool itemIsMap(const DbItem* item);
    bool itemIsObject(const DbItem* item);
    bool itemKeyEquals(const DbItem* item, const char* key);
    std::pair<const DbItem*, const DbItem*> itemUnpackPair(const DbItem* item);
    double itemGetNumber(const DbItem* item);
    DbItem* itemCreateNullId()
    {
        // TODO: optimize
        return itemCreateNull();
    }
    DbItem* itemCreateObject();
    DbItem* itemCreateArray();
    bool itemAddPairToMap(DbItem* obj, DbItem* key, DbItem* value);
    bool itemAddItemToObject(DbItem* obj, const char* name, DbItem* item){
        printf("itemAddItemToObject: %s\n", name);
        return obj->addItemToArray(item);
    }
    bool itemAddItemToArray(DbItem* array, DbItem* item){
        return array->addItemToArray(item);
    }
    /// @brief Helper function to write a number to a ITEM object.
    bool itemAddNumberToObject(DbItem* obj, const char* name, double number);
    bool itemAddStringToObject(DbItem* obj, const char* name, const char* str);
    bool itemAddStringToObject(DbItem* obj, const char* name, const std::string& s);


    /* Create basic types: */
    DbItem *  itemCreateNull(void)
    {
        DbItem *item = new DbItem();
        if(item)
        {
            item->type = dbITEM_NULL;
        }

        return item;
    }

    DbItem *  itemCreateTrue(void)
    {
        DbItem *item = new DbItem();
        if(item)
        {
            item->type = dbITEM_True;
        }

        return item;
    }

    DbItem * itemCreateFalse(void)
    {
        DbItem *item = new DbItem();
        if(item)
        {
            item->type = dbITEM_False;
        }

        return item;
    }

    DbItem * itemCreateBool(bool boolean)
    {
        DbItem *item = new DbItem();
        if(item)
        {
            item->type = boolean ? dbITEM_True : dbITEM_False;
        }

        return item;
    }

    DbItem * itemCreateString(const char *string)
    {
        DbItem *item = new DbItem();
        if(item)
        {
            item->type = dbITEM_String;
            size_t length = strlen(string);

            item->valuestring = new char[length + 1];  
            strcpy(item->valuestring, string);
            if(!item->valuestring)
            {
                delete item;
                return NULL;
            }
        }

        return item;
    }

    DbItem * itemCreateNumber(double num)
    {
        DbItem *item= new DbItem();
        if(item)
        {
            item->type = dbITEM_Number;
            item->valuedouble = num;

            /* use saturation in case of overflow */
            if (num >= INT_MAX)
            {
                item->valueint = INT_MAX;
            }
            else if (num <= (double)INT_MIN)
            {
                item->valueint = INT_MIN;
            }
            else
            {
                item->valueint = (int)num;
            }
        }

        return item;
    }

    DbItem* itemCreateIndex(size_t index)
    {
        constexpr size_t maxPreciseIntInDouble = (1ull << 53);
        (void)maxPreciseIntInDouble; // silence unused warning
        assert(index <= maxPreciseIntInDouble);
        return itemCreateNumber(index);
    }
};

} // namespace SVF

#endif // INCLUDE_NEO4JCLIENT_H_