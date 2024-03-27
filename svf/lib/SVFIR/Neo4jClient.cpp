#include <Python.h>
#include "Util/cJSON.h"
#include "SVFIR/Neo4jClient.h"

namespace SVF {
Neo4jClient:: Neo4jClient(const char* uri = "neo4j+s://4291d08d.databases.neo4j.io", const char* username = "neo4j", const char* password = "ghfQKOPyySepbGDVuGgWsJLhhHP2R-ukd3tbvT9QNu8") {
    // Initialize the Python interpreter
    Py_Initialize();

    // Add the path to the Python module
    PyObject* sys = PyImport_ImportModule("sys");
    PyObject* path = PyObject_GetAttrString(sys, "path");
    PyList_Append(path, PyUnicode_FromString("/home/weigang/SVF/svf/lib/SVFIR"));

    // Import the Python module (Neo4jClient.py)
    PyObject* pModule = PyImport_ImportModule("Neo4jClient");
    if (pModule == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to import module");
        PyErr_Print(); // Print error information
        return; // Failed to import module
    }

    // Get the class definition (Neo4jClient)
    PyObject* pClass = PyObject_GetAttrString(pModule, "Neo4jClient");
    if (pClass == nullptr || !PyCallable_Check(pClass)) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get class definition");
        PyErr_Print();
        Py_DECREF(pModule); // Clean up pModule
        return; // Class not found or not callable
    }

    // Create an instance of the class
    pInstance = PyObject_CallObject(pClass, Py_BuildValue("(sss)", uri, username, password));
    if (pInstance == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create class instance");
        PyErr_Print();
        Py_DECREF(pClass); // Clean up pClass
        Py_DECREF(pModule); // Clean up pModule
        return; // Failed to create instance
    }

    // Clean up
    Py_DECREF(pClass);
    Py_DECREF(pModule);
}

    // Destructor
Neo4jClient::~Neo4jClient() {
    // Close the connection to neo4j database
    PyObject_CallMethod(pInstance, "close", nullptr);

    // Clean up
    Py_DECREF(pInstance);

    // Finalize the Python interpreter
    Py_Finalize();
}

DbNode Neo4jClient::createNode(const char* graph_id, const char* node_type, ...) {
    printf("Neo4jClient::createNode\n");
    // Create a new dictionary to store the node properties
    PyObject* properties = PyDict_New();
    if (properties == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create node properties dictionary");
        PyErr_Print();
    }

    // Add graph_id to properties dictionary
    PyDict_SetItemString(properties, "graph_id", PyUnicode_FromString(graph_id));

    // Write the key-value pairs to the properties dictionary
    va_list args; // All variable arguments passed from the function call
    va_start(args, node_type); // Start reading the variable arguments after the node_type
    const char* key;
    PyObject* value;
    while ((key = va_arg(args, const char*)) != nullptr) { // Read the key (use nullptr to mark the ends of the variable arguments list)
        PyObject* pyKey = PyUnicode_FromString(key);
        value = va_arg(args, PyObject*); // Read the value
        PyDict_SetItem(properties, pyKey, value); // Add the key-value pair to the properties dictionary
    }
    va_end(args);

    // Create a new Node object and return it
    return DbNode(node_type, properties);
}

DbEdge Neo4jClient::createEdge(const char* graph_id, const char* edge_type, ...) {
    printf("Neo4jClient::createEdge\n");
    // Create a new dictionary to store the edge properties
    PyObject* properties = PyDict_New();
    if (properties == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create edge properties dictionary");
        PyErr_Print();
    }

    // Add graph_id to properties dictionary
    PyDict_SetItemString(properties, "graph_id", PyUnicode_FromString(graph_id));

    // Write the key-value pairs to the properties dictionary
    va_list args; // All variable arguments passed from the function call
    va_start(args, edge_type); // Start reading the variable arguments after the edge_type
    const char* key;
    PyObject* value;
    while ((key = va_arg(args, const char*)) != nullptr) { // Read the key (use nullptr to mark the ends of the variable arguments list)
        PyObject* pyKey = PyUnicode_FromString(key);
        value = va_arg(args, PyObject*); // Read the value
        PyDict_SetItem(properties, pyKey, value); // Add the key-value pair to the properties dictionary
    }
    va_end(args);

    // Create a new Node object and return it
    return DbEdge(edge_type, properties);
}

DbNode Neo4jClient::createNodeJson(const char* graph_id, const char* node_type, cJSON* jsonNode) {
    // Create a new dictionary to store the node properties
    PyObject* properties = PyDict_New();
    if (properties == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create node properties dictionary");
        PyErr_Print();
    }

    // Add graph_id to properties dictionary
    PyDict_SetItemString(properties, "graph_id", PyUnicode_FromString(graph_id));

    // Write the key-value pairs to the properties dictionary
    cJSON* current_element = nullptr;
    cJSON_ArrayForEach(current_element, jsonNode) {
        PyObject* pyKey = PyUnicode_FromString(current_element->string);
        if (cJSON_IsNumber(current_element)) { // Check if the current element is a number
            PyDict_SetItem(properties, pyKey, PyLong_FromLong(current_element->valueint));
        } else if (cJSON_IsString(current_element)) { // Check if the current element is a string
            PyDict_SetItem(properties, pyKey, PyUnicode_FromString(current_element->valuestring));
        } else if (cJSON_IsArray(current_element)) { // Check if the current element is an array
            // Convert the C++ array to a Python list and adds to properties dictionary
            int size = cJSON_GetArraySize(current_element);
            PyObject* pyList = PyList_New(size);
            for (int i = 0; i < size; i++) {
                cJSON* item = cJSON_GetArrayItem(current_element, i);
                if (cJSON_IsNumber(item)) {
                    PyList_SetItem(pyList, i, PyLong_FromLong(item->valueint));
                } else if (cJSON_IsString(item)) {
                    PyList_SetItem(pyList, i, PyUnicode_FromString(item->valuestring));
                }
            }
            PyDict_SetItem(properties, pyKey, pyList);
        }
    }

    // Create a new Node object and return it
    return DbNode(node_type, properties);
}

DbEdge Neo4jClient::createEdgeJson(const char* graph_id, const char* edge_type, cJSON* edgeJson) {
    // Create a new dictionary to store the edge properties
    PyObject* properties = PyDict_New();
    if (properties == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create edge properties dictionary");
        PyErr_Print();
    }

    // Add graph_id to properties dictionary
    PyDict_SetItemString(properties, "graph_id", PyUnicode_FromString(graph_id));

    // Write the key-value pairs to the properties dictionary
    cJSON* current_element = nullptr;
    cJSON_ArrayForEach(current_element, edgeJson) {
        PyObject* pyKey = PyUnicode_FromString(current_element->string);
        if (cJSON_IsNumber(current_element)) { // Check if the current element is a number
            PyDict_SetItem(properties, pyKey, PyLong_FromLong(current_element->valueint));
        } else if (cJSON_IsString(current_element)) { // Check if the current element is a string
            PyDict_SetItem(properties, pyKey, PyUnicode_FromString(current_element->valuestring));
        } else if (cJSON_IsArray(current_element)) { // Check if the current element is an array
            // Convert the C++ array to a Python list and adds to properties dictionary
            int size = cJSON_GetArraySize(current_element);
            PyObject* pyList = PyList_New(size);
            for (int i = 0; i < size; i++) {
                cJSON* item = cJSON_GetArrayItem(current_element, i);
                if (cJSON_IsNumber(item)) {
                    PyList_SetItem(pyList, i, PyLong_FromLong(item->valueint));
                } else if (cJSON_IsString(item)) {
                    PyList_SetItem(pyList, i, PyUnicode_FromString(item->valuestring));
                }
            }
            PyDict_SetItem(properties, pyKey, pyList);
        }
    }

    // Create a new Node object and return it
    return DbEdge(edge_type, properties);
}

void Neo4jClient:: writeNode(const DbNode& node) {
    // Call create_node method
    PyObject* pResult = PyObject_CallMethod(pInstance, "create_node", "(sO)",
                                            node.getNodetype(), node.getNodeProperties());
    // Check if the call succeeded
    if (pResult == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to call create_node method in python module");
        PyErr_Print();
        return;
    }
}

void Neo4jClient::writeEdge(const DbNode& node1, const DbNode& node2, const DbEdge& edge) {
    // Call create_edge method
    PyObject* pResult = PyObject_CallMethod(pInstance, "create_edge", "(sOsOsO)",
                                            node1.getNodetype(), node1.getNodeProperties(),
                                            node2.getNodetype(), node2.getNodeProperties(),
                                            edge.getEdgeType(), edge.getEdgeProperties());
    // Check if the call succeeded
    if (pResult == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to call create_edge method");
        PyErr_Print();
        return;
    }
}

void Neo4jClient::clearDatabase() {
    // Clear neo4j database
    PyObject_CallMethod(pInstance, "clear_database", nullptr);
}

Neo4jItemManager::Neo4jItemManager(Neo4jClient * bd) {
    this->bd = bd;
}

DbItem*  Neo4jItemManager::itemCreateArray(){
    DbItem *item= new DbItem();
    if(item)
    {
        item->type=dbITEM_Array;
    }
    return item;
}

DbItem* Neo4jItemManager::itemCreateObject(){
    DbItem *item= new DbItem();
    if(item)
    {
        item->type=dbITEM_Object;
    }
    return item;

}

}