#pragma once
#include <map>
#include <vector>
#include <set>

namespace tyngraph {

typedef int nodeid;
typedef int edgeid;
typedef int routeid;

class Edge {
  int _weight;
  nodeid _nodea;
  nodeid _nodeb;

public:
  Edge(nodeid a, nodeid b, int weight) {
    this->_nodea = a;
    this->_nodeb = b;
    this->_weight = weight;
  }
	Edge() {
    this->_nodea = -1;
    this->_nodeb = -1;
    this->_weight = 1;
	}
	Edge(const Edge &e) {
    this->_nodea = e._nodea;
    this->_nodeb = e._nodeb;
    this->_weight = e._weight;
	}
  nodeid nodea() const { return this->_nodea; }
  nodeid nodeb() const { return this->_nodeb; }
  int weight() const { return this->_weight; }
};

class Route {
  int _length;
  nodeid _start;
  nodeid _end;
  nodeid _next;

public:
  Route(nodeid start, nodeid end, nodeid next, int length) {
    this->_start = start;
    this->_end = end;
    this->_next = next;
    this->_length = length;
  }
  nodeid start() const { return this->_start; }
  nodeid end() const { return this->_end; }
  nodeid next() const { return this->_next; }
  int length() const { return this->_length; }
};

class Node {
public:
  std::vector<edgeid> edges;
  std::map<nodeid, Route> routes;

  void add_edge(edgeid edge) { this->edges.push_back(edge); }

  void add_route(nodeid start, nodeid end, nodeid next, int length) {
    this->routes.insert_or_assign(end, Route(start, end, next, length));
  }

  int count_edges() const { return this->edges.size(); }
};

class Graph {
  int guids;

public:
  std::map<edgeid, Edge> edges;
  std::map<routeid, Route> routes;
  std::map<nodeid, Node> nodes;

  Graph() { 
		this->guids = 0; 
		this->edges = {};
		this->routes = {};
		this->nodes = {};
	}

  /**
   * @returns new node id
   */
  nodeid add_node();

  /**
   * @brief creates directed edge from a to b
   *
   * @param a
   * @param b
   * @param weight , or lenfth
   */
  edgeid add_edge(nodeid a, nodeid b, int weight = 1);

  /**
   * @brief creates edge from a to b and from b to a
   *
   * @param a
   * @param b
   * @param weight , or lenfth
   */
  void add_edge_bidir(nodeid a, nodeid b, int weight = 1);

  int count_nodes();

  /**
   * @brief returns next node required to reach b form a
   *
   * @param a
   * @param b
   * @reurns next node in path or -1 if no path found
   */
  nodeid route(nodeid a, nodeid b);

private:
  void build(nodeid a, std::set<nodeid> &traversed);
};

} // namespace tyngraph
