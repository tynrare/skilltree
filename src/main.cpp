#include "raylib.h"
#include <map>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#ifdef __DEBUG__
#define RES_PATH "../res/"
#else
#define RES_PATH "res/"
#endif

// #include <iostream>
// #include <ostream>
#include "skillicon.hpp"
#include "skilltree.hpp"

void UpdateDrawFrame(void);

using namespace tynskills;

int screenWidth = 800;
int screenHeight = 450;

Skilltree *skilltree;
std::map<nodeid, Skillicon> skillicons;

Leaf *make_skill(const char *name, Vector2 pos) {
  nodeid leafid = skilltree->add_leaf();
  Texture texture = LoadTexture(TextFormat(RES_PATH "icons/%s.png", name));
	skillicons[leafid] = Skillicon(texture, pos, leafid);

  return skilltree->get_leaf(leafid);
}

void init() {
  skilltree = new Skilltree();

  Leaf *skill1 = make_skill("SGI_01", (Vector2){0.0, 0.0});
  Leaf *skill2 = make_skill("SGI_02", (Vector2){(128.0 + 16.0) * 1, 0.0});
  Leaf *skill3 = make_skill("SGI_03", (Vector2){(128.0 + 16.0) * 2, 0.0});
  Leaf *skill4 = make_skill("SGI_04", (Vector2){(128.0 + 16.0) * 2, (128.0 + 16.0) * 1});
  Leaf *skill5 = make_skill("SGI_05", (Vector2){(128.0 + 16.0) * 1, (128.0 + 16.0) * 1});
  Leaf *skill6 = make_skill("SGI_06", (Vector2){(128.0 + 16.0) * 1, (128.0 + 16.0) * 2});
  Leaf *skill7 = make_skill("SGI_07", (Vector2){(128.0 + 16.0) * 3, (128.0 + 16.0) * 2});
  Leaf *skill8 = make_skill("SGI_08", (Vector2){(128.0 + 16.0) * 3, (128.0 + 16.0) * 0});
  Leaf *skill9 = make_skill("SGI_09", (Vector2){(128.0 + 16.0) * 4, (128.0 + 16.0) * 1});
	skill1->setup(0, 5, true);
	skill2->setup(0, 5, true);
	skill3->setup(0, 5, false);
	skill4->setup(0, 5, false);
	skill5->setup(0, 5, false);
	skill6->setup(0, 5, false, BranchProgressMode::MAXIMUM);
	skill7->setup(0, 5, false);
	skill8->setup(0, 5, false);
	skill9->setup(0, 5, false, BranchProgressMode::MAXIMUM);

  skilltree->add_branch(skill2->get_id(), skill3->get_id(), BranchProgressMode::MINIMUM);
  skilltree->add_branch(skill3->get_id(), skill4->get_id());
  skilltree->add_branch(skill2->get_id(), skill5->get_id());
  skilltree->add_branch(skill5->get_id(), skill6->get_id());
  skilltree->add_branch(skill4->get_id(), skill7->get_id());
  skilltree->add_branch(skill4->get_id(), skill6->get_id());
  skilltree->add_branch(skill3->get_id(), skill8->get_id(), BranchProgressMode::ANY);

  skilltree->add_branch(skill7->get_id(), skill9->get_id());
  skilltree->add_branch(skill8->get_id(), skill9->get_id(), BranchProgressMode::MINIMUM);
}

void draw() {
  Vector2 mouse = GetMousePosition();
  bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

  Vector2 pad = {16.0, 16.0};
  for (auto &[id, icon] : skillicons) {
		Leaf *leaf = skilltree->get_leaf(id);
		Node *node = skilltree->get_node(id);
    const Rectangle rect = icon.get_rect(pad);
		const Vector2 center = icon.get_center(pad);

		// user interact
    bool collision = CheckCollisionPointRec(mouse, rect);
    if (collision && clicked) {
      leaf->upgrade();
    }

		// draw edges
		for (const auto &eid : node->edges) {
			const Edge *edge = skilltree->get_edge(eid);
			if (edge->nodea() != id) {
				continue;
			}

			const Branch *branch = skilltree->get_branch(eid);

			Skillicon *iconb = &skillicons[edge->nodeb()];
			Leaf *leafb = skilltree->get_leaf(edge->nodeb());
			const Vector2 centerb = iconb->get_center(pad);

			bool active = branch->is_active(leaf);
			Color color = active ? RED : GRAY;
			DrawLineEx(center, centerb, 4.0, color);

			// activate branched skills
			if (active) {
				skilltree->activate_leaf(leafb->get_id());
				//leafb->activate();
			}
		}

		// draw icon
    icon.draw(leaf, rect, collision);
  }
}

void dispose() { delete skilltree; }

//----------------------------------------------------------------------------------
// Main Entry Point
//----------------------------------------------------------------------------------
int main() {

  // Initialization
  //--------------------------------------------------------------------------------------
  InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

  init();

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
  SetTargetFPS(60); // Set our game to run at 60 frames-per-second
  //--------------------------------------------------------------------------------------


  // Main game loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    UpdateDrawFrame();
  }
#endif

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
	dispose();
  //--------------------------------------------------------------------------------------

  return 0;
}


void UpdateDrawFrame(void) {

  BeginDrawing();
		ClearBackground(RAYWHITE);
		draw();
  EndDrawing();
}
