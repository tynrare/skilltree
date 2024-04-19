#pragma once
#include "graph.hpp"
#include <raylib.h>

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

class Leaf {
  nodeid id;
  int points;
  int maxpoints;
  bool active;
	BranchProgressMode mode;

public:
  Leaf(nodeid id) { 
		this->id = id; 
		this->setup(0, 0, false);
	}
  Leaf() { 
		this->id = -1; 
		this->mode = BranchProgressMode::ANY;
		this->setup(0, 0, false);
	}
  Leaf(const Leaf &l) {
    this->id = l.id;
    this->setup(l);
  }

  int get_id() { return this->id; }

	BranchProgressMode get_mode() const { return this->mode; }

  bool is_active() const { return this->active; }

  int get_points() const { return this->points; }

  int get_maxpoints() const { return this->maxpoints; }

  void setup(int points, int maxpoints, bool active, BranchProgressMode mode = BranchProgressMode::ANY) {
    this->points = points;
    this->maxpoints = maxpoints;
    this->active = active;
		this->mode = mode;
  }
  void setup(const Leaf &l) { this->setup(l.points, l.maxpoints, l.active, l.mode); }

  void activate() { this->active = true; }

  void upgrade() {
    if (!this->active) {
      return;
    }

    this->points += 1;
    if (this->points >= this->maxpoints) {
      this->points = this->maxpoints;
    }
  }
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

	bool is_active(const Leaf *leaf) const {
		if (!leaf->is_active()) {
			return false;
		}

		switch(this->mode) {
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
  nodeid add_leaf(BranchProgressMode mode = BranchProgressMode::ANY) {
    const nodeid id = this->graph.add_node();
    this->leafs[id] = Leaf(id);

    return id;
  }

  Leaf *get_leaf(int id) { return &this->leafs[id]; }

  Node *get_node(int id) { return &this->graph.nodes[id]; }

  void add_branch(nodeid a, nodeid b, BranchProgressMode mode = BranchProgressMode::MAXIMUM) {
    const edgeid id = this->graph.add_edge(a, b);
    this->branches[id] = Branch(id, mode);
  }

  Branch *get_branch(int id) { return &this->branches[id]; }

  Edge *get_edge(int id) { return &this->graph.edges[id]; }

	bool activate_leaf(int id) {
		Leaf *leaf = this->get_leaf(id);

		if (leaf->is_active()) {
			return true;
		}

		Node *node = this->get_node(id);

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

	  bool active = false;

		switch(leaf->get_mode()) {
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

		if (active) {
			leaf->activate();
		}

		return leaf->is_active();
	}
};
} // namespace tynskills
