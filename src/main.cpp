#include "raylib.h"
#include <map>
#include <raymath.h>

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
#include "dukscript.hpp"

using namespace tynskills;

Skilltree *skilltree;
Dukscript *dukscript;
std::map<nodeid, Skillicon> skillicons;
long config_file_timestamp = 0;
const char *config_filename = RES_PATH "skills.ini";
int points_spent = 0;

void UpdateDrawFrame(void);
void parse_config(Skilltree *skilltree);

int screenWidth = 800;
int screenHeight = 450;

void init() {
  skilltree = new Skilltree();
	dukscript = new Dukscript();

	dukscript->eval("print('Dukscript initialized');");
	//dukscript->eval("var a = 1");

  parse_config(skilltree);
	return;
}

Vector2 pad = {16.0, 16.0};

void draw() {
  Vector2 mouse = GetMousePosition();
  bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  bool clicked_second = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

  // draw edges in first pass
  for (auto &[id, icon] : skillicons) {
    Leaf *leaf = skilltree->get_leaf(id);
    Node *node = skilltree->get_node(id);
    const Vector2 center = icon.get_center(pad);

    for (const auto &eid : node->edges) {
      const Edge *edge = skilltree->get_edge(eid);
      if (edge->nodea() != id) {
        continue;
      }

      const Branch *branch = skilltree->get_branch(eid);
      Skillicon *iconb = &skillicons[edge->nodeb()];
      const Vector2 centerb = iconb->get_center(pad);

      bool active = branch->is_active(leaf);
      Color color = active ? RED : GRAY;
      DrawLineEx(center, centerb, 4.0, color);
    }
  }

  // draw icons and user interact in second pass
  bool btn_pressed = false;
  for (auto &[id, icon] : skillicons) {
    Leaf *leaf = skilltree->get_leaf(id);
    Node *node = skilltree->get_node(id);
    const Rectangle rect = icon.get_rect(pad);

    // user interact
    bool collision = CheckCollisionPointRec(mouse, rect) && leaf->is_active();
    btn_pressed = btn_pressed || collision;
    if (collision && clicked) {
			// upgrade leaf
      const int delta = leaf->upgrade();
			points_spent += delta;
			skilltree->refresh_leaf(id);
    } else if (collision && clicked_second) {
			// downgrade leaf
      const int delta = leaf->downgrade();
			points_spent += delta;
			points_spent += skilltree->refresh_leaf(id);
		}
    // draw icon
    icon.draw(leaf, rect, collision);
  }

  if (!btn_pressed && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    Vector2 delta = GetMouseDelta();
    pad = Vector2Add(delta, pad);
  }

	const int fontsize = 20;
	DrawText(TextFormat("%d spent", points_spent), 
			8, screenHeight - fontsize - 8, fontsize, BLACK);
}

void dispose() {
  skilltree->cleanup();
  delete skilltree;
}

//----------------------------------------------------------------------------------
// Main Entry Point
//----------------------------------------------------------------------------------
int main() {

  // Initialization
  //--------------------------------------------------------------------------------------
  InitWindow(screenWidth, screenHeight, "tynroar skilltree");

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
	if (config_file_timestamp != GetFileModTime(config_filename)) {
		dispose();
		init();
	}

  BeginDrawing();
  ClearBackground(RAYWHITE);
  draw();
  EndDrawing();
}

#include <sstream>
#include <string>
#include "external/INIReader.h"

/**
 * ini entries sorted by name. All names has to be ordered carefully
 *
 * @param skilltree
 */
void parse_config(Skilltree *skilltree) {
  INIReader reader(config_filename);

	config_file_timestamp = GetFileModTime(config_filename);

	if (reader.ParseError() < 0) {
		TraceLog(LOG_ERROR, TextFormat("ini read parse error #%d", reader.ParseError()));
		return;
	}

  std::map<std::string, BranchProgressMode> name_modes = {
      {"any", BranchProgressMode::ANY},
      {"min", BranchProgressMode::MINIMUM},
      {"max", BranchProgressMode::MAXIMUM}};
  std::map<std::string, nodeid> name_to_id;

  Vector2 cell = {128.0 + 16.0, 128.0 + 16.0};
	char delimiter = ',';

	// load icons and skills
  std::set<std::string> sections = reader.GetSections();
  for (std::set<std::string>::iterator sectionsIt = sections.begin();
       sectionsIt != sections.end(); sectionsIt++) {
		auto section = *sectionsIt;
		TraceLog(LOG_INFO, TextFormat("Add skill: %s", sectionsIt->c_str()));
    const int leafid = skilltree->add_leaf();
    name_to_id[section] = leafid;
    Leaf *leaf = skilltree->get_leaf(leafid);

		const BranchProgressMode mode = name_modes[reader.Get(section, "mode", "max")];
    Skillinfo s = {
                   .points = (int)reader.GetInteger(section, "points", 0),
                   .maxpoints = (int)reader.GetInteger(section, "maxpoints", 4),
									 .active = reader.GetBoolean(section, "active", false),
                   .mode = mode,
									 .name = section
		};
	 	leaf->setup(s);

		Vector2 pos = { 0.0, 0.0 };
		auto pos_origin = reader.Get(section, "pos_origin", "");
		auto pos_shift = reader.Get(section, "pos_shift", "");

		if (pos_shift.length() && pos_origin.length()) {
			nodeid originid = name_to_id[pos_origin];
			Skillicon *icon = &skillicons[originid];
			auto delimiter_find = pos_shift.find(delimiter);

			if (delimiter_find != std::string::npos) {
				auto sx = pos_shift.substr(0, delimiter_find);
				auto sy = pos_shift.substr(delimiter_find + 1, pos_shift.length());
				float x = std::stof(sx);
				float y = std::stof(sy);
				const Vector2 origin = icon->get_pos();

				pos.x = origin.x + cell.x * x;
				pos.y = origin.y + cell.y * y;
			}
		}

		auto icon_name = reader.Get(section, "icon", "UNKNOWN");
    Texture texture = LoadTexture(TextFormat(RES_PATH "icons/%s.png", icon_name.c_str()));

    skillicons[leafid] = Skillicon(texture, pos, leafid);
  }

	// load branches
  for (std::set<std::string>::iterator sectionsIt = sections.begin();
       sectionsIt != sections.end(); sectionsIt++) {
		auto branches = reader.Get(*sectionsIt, "branches", "");
		if (!branches.length()) {
			continue;
		}

		std::stringstream ss (branches);
		std::string item;

		nodeid leafa = name_to_id[*sectionsIt];
    while (getline (ss, item, delimiter)) {
			nodeid leafb = -1;
			auto delimiter_find = item.find(':');
			BranchProgressMode mode = BranchProgressMode::MAXIMUM;
			if (delimiter_find != std::string::npos) {
				auto name = item.substr(0, delimiter_find);
				auto smode = item.substr(delimiter_find + 1, item.length());
				mode = name_modes[smode];
				leafb = name_to_id[name];
			} else {
				leafb = name_to_id[item];
			}

			// in config branches reversed - they listed in INPUT leafs
			skilltree->add_branch(leafb, leafa, mode);
		}
	}
}
