#pragma once
#include "graph.hpp"
#include <algorithm>
#include <raylib.h>
#include <string>

using namespace tyngraph;

namespace tynskills {

enum class BranchProgressMode {
  // Branch: Allows progress when root leaf becomes active
  // Leaf: Allows progress when any input branch is active
  ANY,
  // Branch: Allows progress when leaf has one point
  // Leaf: Same as any
  MINIMUM,
  // Branch: Allows progress when leaf upgraded to maximum
  // Lead: Allows progress when all input branches active
  MAXIMUM
};

struct Skillinfo {
  int points;
  int maxpoints;
  bool active;
  BranchProgressMode mode;
	std::string name;
	std::string bind;
};

class Leaf {
  nodeid id;
  Skillinfo info;

public:
  Leaf(nodeid id) {
    this->id = id;
    Leaf();
  }
  Leaf() {
    this->id = -1;
    this->info = {};
    this->setup(0, 0, false);
  }
  Leaf(const Leaf &l) {
    this->id = l.id;
    this->setup(l);
  }

  int get_id() { return this->id; }
  BranchProgressMode get_mode() const { return this->info.mode; }
  bool is_active() const { return this->info.active; }
  int get_points() const { return this->info.points; }
  int get_maxpoints() const { return this->info.maxpoints; }
  const std::string &get_name() const { return this->info.name; }
  const std::string &get_bind() const { return this->info.bind; }
  const bool has_bind() const { return this->get_bind().length() > 0; }

  void setup(int points, int maxpoints, bool active,
             BranchProgressMode mode = BranchProgressMode::ANY) {
    this->info.points = points;
    this->info.maxpoints = maxpoints;
    this->info.active = active;
    this->info.mode = mode;
  }
  void setup(const Leaf &l) { this->info = l.info; }
  void setup(const Skillinfo &info) { this->info = info; }

	/**
	 * @param active
	 *
	 * @returns {int} points discarded if leaf was deactivated.
	 * Negative value
	 */
	int set_active(bool active) {  
		this->info.active = active;
		if (!active) {
			return this->downgrade(this->get_points());
		}

		return 0;
	}
  void activate() { this->set_active(true); }

	/**
	 * Upgrading possible even if leaf not active.
	 * Validate active status before calling this function
	 *
	 * @param points
	 *
	 * @return 
	 */
  int upgrade(int points = 1) {
    int p = this->info.points;
    this->info.points =
        std::clamp(this->info.points + points, 0, this->info.maxpoints);

    return this->info.points - p;
  }

	/**
	 * @param points
	 *
	 * @returns {int} points was discarded. Negative value
	 */
  int downgrade(int points = 1) { return this->upgrade(-points); }
};

class Branch {
  edgeid id;
  BranchProgressMode mode;

public:
  Branch(edgeid id, BranchProgressMode mode = BranchProgressMode::MAXIMUM) {
    this->id = id;
    this->mode = mode;
  }
  Branch() {
    this->id = -1;
    this->mode = BranchProgressMode::ANY;
  }
  Branch(const Branch &b) {
    this->id = b.id;
    this->mode = b.mode;
  }

  BranchProgressMode get_mode() const { return this->mode; }

  bool is_active(const Leaf *leaf) const {
    if (!leaf->is_active()) {
      return false;
    }

    switch (this->mode) {
    case BranchProgressMode::ANY:
      return leaf->is_active();
    case BranchProgressMode::MINIMUM:
      return leaf->is_active() && leaf->get_points() > 0;
    case BranchProgressMode::MAXIMUM:
      return leaf->is_active() && leaf->get_points() >= leaf->get_maxpoints();
    default:
      return false;
    }
  }
};

class Skilltree {
  Graph graph;
  std::map<nodeid, Leaf> leafs;
  std::map<edgeid, Branch> branches;

public:
  void cleanup() {
    this->leafs.clear();
    this->branches.clear();
    this->graph.cleanup();
  }

  nodeid add_leaf() {
    const nodeid id = this->graph.add_node();
    this->leafs[id] = Leaf(id);

    return id;
  }

  Leaf *get_leaf(int id) { return &this->leafs[id]; }

  Node *get_node(int id) { return &this->graph.nodes[id]; }

  void add_branch(nodeid a, nodeid b,
                  BranchProgressMode mode = BranchProgressMode::MAXIMUM) {
    const edgeid id = this->graph.add_edge(a, b);
    this->branches[id] = Branch(id, mode);
  }

  Branch *get_branch(int id) { return &this->branches[id]; }

  Edge *get_edge(int id) { return &this->graph.edges[id]; }

	/**
	 * refreshes all subleafs. Call it after upgrade or downgrade
	 *
	 * @param id
	 * @returns {int} amount of points was discarded on downgrade.
	 * Negative value
	 */
	int refresh_leaf(int id) {
		int points_delta = 0;
    Leaf *leaf = this->get_leaf(id);
    Node *node = this->get_node(id);

		// ---
		// update leaf active status itself
    int active_branches = 0;
    int total_branches = 0;
    for (const auto &eid : node->edges) {
      const Edge *edge = this->get_edge(eid);
      if (edge->nodeb() != id) {
        continue;
      }

      total_branches += 1;
      const Branch *branch = this->get_branch(eid);
      const Leaf *leafb = this->get_leaf(edge->nodea());

      if (branch->is_active(leafb)) {
        active_branches += 1;
      }
    }

		// active status depends on input branches and leaf mode
    bool active = false;
    switch (leaf->get_mode()) {
    case BranchProgressMode::ANY:
    case BranchProgressMode::MINIMUM:
      active = active_branches > 0;
      break;
    case BranchProgressMode::MAXIMUM:
      active = active_branches >= total_branches;
      break;
    default:
      active = false;
    }

		points_delta += leaf->set_active(active);

		// ---
		// refresh all dependent leafs
		for (const auto &eid : node->edges) {
			const Edge *edge = this->get_edge(eid);
			if (edge->nodea() != id) {
				continue;
			}

			Leaf *leafb = this->get_leaf(edge->nodeb());

			points_delta += this->refresh_leaf(leafb->get_id());
		}

		return points_delta;
	}
};
} // namespace tynskills
