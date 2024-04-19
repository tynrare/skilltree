#include "graph.hpp"
#include <map>
#include <set>

using namespace tyngraph;

/**
 * @returns new node id
 */
nodeid Graph::add_node() {
  const int guid = this->guids++;
  this->nodes[guid] = Node();

  return guid;
}

/**
 * @brief creates directed edge from a to b
 *
 * @param a
 * @param b
 * @param weight , or lenfth
 */
edgeid Graph::add_edge(nodeid a, nodeid b, int weight) {
  const int guid = this->guids++;
	this->edges[guid] = Edge(a, b, weight);

  Node *nodea = &this->nodes[a];
  Node *nodeb = &this->nodes[b];
  nodea->add_edge(guid);
  nodeb->add_edge(guid);

  std::set<nodeid> traversed;
  this->build(a, traversed);

	return guid;
}

/**
 * @brief creates edge from a to b and from b to a
 *
 * @param a
 * @param b
 * @param weight , or lenfth
 */
void Graph::add_edge_bidir(nodeid a, nodeid b, int weight) {
  this->add_edge(a, b, weight);
  this->add_edge(b, a, weight);
}

int Graph::count_nodes() { return this->nodes.size(); }

/**
 * @brief returns next node required to reach b form a
 *
 * @param a
 * @param b
 * @reurns next node in path or -1 if no path found
 */
nodeid Graph::route(nodeid a, nodeid b) {
  const Node *nodea = &this->nodes[a];
  const auto route = nodea->routes.find(b);
  if (route == nodea->routes.end()) {
    return -1;
  }

  return route->second.next();
}

void Graph::build(nodeid a, std::set<nodeid> &traversed) {
  Node *nodea = &this->nodes[a];

  if (traversed.count(a) > 0) {
    return;
  }

  traversed.insert(a);

  // iterate forward
  for (const edgeid &id : nodea->edges) {
    const Edge *edge = &this->edges.at(id);
    if (edge->nodea() != a) {
      continue;
    }

    // add new direct route
    const nodeid b = edge->nodeb();
    Node *nodeb = &this->nodes[b];
    nodea->add_route(edge->nodea(), edge->nodeb(), edge->nodeb(),
                     edge->weight());

    // add dependent routes
    for (const auto &[id, route] : nodeb->routes) {
      // check if route already exists and new one shorter
      const auto existing_route = nodea->routes.find(route.end());
      if (existing_route != nodea->routes.end() &&
          existing_route->second.length() - edge->weight() < route.length()) {
        continue;
      }

      nodea->add_route(a, route.end(), b, route.length() + edge->weight());
    }
  }

  // iterade backward
  for (const edgeid &id : nodea->edges) {
    const Edge *edge = &this->edges.at(id);
    if (edge->nodeb() != a) {
      continue;
    }

    this->build(edge->nodea(), traversed);
  }
}
