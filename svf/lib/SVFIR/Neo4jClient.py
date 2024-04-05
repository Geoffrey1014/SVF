# -*- coding: utf-8 -*-
"""
Created on Mon Mar 11 12:41:50 2024

@author: Ruijun Feng
"""

from neo4j import GraphDatabase

class Neo4jClient:
    def __init__(self, uri, username, password):
        self._driver = GraphDatabase.driver(uri, auth=(username, password))

    def close(self):
        '''
        Closing the neo4j driver.
        '''
        self._driver.close()
    
    def clear_database(self):
        '''
        Clearing the neo4j database.
        '''
        with self._driver.session() as session:
            session.run("MATCH (n) DETACH DELETE n")

    def create_node(self, nodetype, properties):
        '''
        Creating a node.
        nodetype specifies the type of node and properties specifies the attributes of the node.
        '''
        with self._driver.session() as session:
            session.execute_write(self._create_node, 
                                  nodetype, 
                                  properties)

    @staticmethod
    def _create_node(tx, nodetype, properties):
        '''
        Formatting the query for creating a node.
        '''
        query = (
            f"CREATE (n:{nodetype} {Neo4jClient._format_properties(properties)})"
        )
        tx.run(query)

    def create_edge(self, nodetype1, properties1, nodetype2, properties2, edge_type, edge_properties):
        '''
        Creating a edge between two nodes.
        '''
        with self._driver.session() as session:
            session.execute_write(
                self._create_edge,
                nodetype1,
                properties1,
                nodetype2,
                properties2,
                edge_type,
                edge_properties
            )

    @staticmethod
    def _create_edge(tx, nodetype1, properties1, nodetype2, properties2, edge_type, edge_properties):
        '''
        Formatting a query for creating a edge between two nodes.
        nodetype and properties are used to identify the nodes and edge_type is used to identify the relationship between the nodes.
        '''
        query = (
            f"MATCH (node1:{nodetype1} {Neo4jClient._format_properties(properties1)}), "
            f"(node2:{nodetype2} {Neo4jClient._format_properties(properties2)}) "
            f"CREATE (node1)-[:{edge_type} {Neo4jClient._format_properties(edge_properties)}]->(node2)"
        )
        tx.run(query)

    @staticmethod
    def _format_properties(properties):
        '''
        Formatting a query for all properties of a node or edge.
        '''
        formatted_properties = []
        for key, value in properties.items():
            if isinstance(value, str):
                formatted_value = f"'{value}'"
            else:
                formatted_value = str(value)
            formatted_properties.append(f"{key}: {formatted_value}")
        return "{" + ", ".join(formatted_properties) + "}"

    def run_query(self, query):
        '''
        Querying the neo4j database.
        '''
        with self._driver.session() as session:
            result = session.run(query)
            return result.data()
    
    @staticmethod
    def generate_node_query(graph_id):
        query = f"MATCH (n) WHERE n.graph_id = '{graph_id}' RETURN n"
        return query

    @staticmethod
    def generate_edge_query(graph_id):
        query = f"MATCH (a)-[r]->(b) WHERE r.graph_id = '{graph_id}' RETURN r, properties(r) as props, a, b"
        return query

if __name__ == "__main__":
    # Define connection parameters
    uri = "neo4j+s://4291d08d.databases.neo4j.io"
    username = "neo4j"
    password = "ghfQKOPyySepbGDVuGgWsJLhhHP2R-ukd3tbvT9QNu8"
    
    # Start a client
    client = Neo4jClient(uri, username, password)

    # Clear the database
    client.clear_database()
    
    # Create a node
    graph_id = "1"
    client.create_node(nodetype = "Person", 
                       properties = {"name": "Alice", "age": 30, "graph_id": graph_id})
    
    # Create another node
    client.create_node(nodetype = "Person", 
                       properties = {"name": "Bob", "age": 35, "graph_id": graph_id})
    
    # Create a edge between nodes
    client.create_edge(nodetype1 = "Person", 
                       properties1 = {"name": "Alice", "graph_id": graph_id}, 
                       nodetype2 = "Person", 
                       properties2 = {"name": "Bob", "graph_id": graph_id}, 
                       edge_type = "KNOWS", 
                       edge_properties = {"graph_id": graph_id, "since": "2022-01-01"})

    # Run the query
    node_result = client.run_query(client.generate_node_query(graph_id))
    edge_result = client.run_query(client.generate_edge_query(graph_id))

    # Print the results
    print("Nodes:")
    for i, record in enumerate(node_result):
        print(f"Node {i+1}: {record}")

    print("Edges:")
    for record in edge_result:
        print(f"Edge: {record['r']}")
        print(f"Properties: {record['props']}")
        print(f"From: {record['a']}")
        print(f"To: {record['b']}")

    # Close the session
    client.close()