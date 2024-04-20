#pragma once
#include "graph.hpp"
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

  void setup(int points, int maxpoints, bool active, BranchProgressMode mode = BranchProgressMode::ANY) {
    this->info.points = points;
    this->info.maxpoints = maxpoints;
    this->info.active = active;
		this->info.mode = mode;
  }
  void setup(const Leaf &l) { this->info = l.info; }
	void setup(const Skillinfo &info) {
		this->info = info;
	}

  void activate() { this->info.active = true; }

  void upgrade() {
    if (!this->info.active) {
      return;
    }

    this->info.points += 1;
    if (this->info.points >= this->info.maxpoints) {
      this->info.points = this->info.maxpoints;
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
