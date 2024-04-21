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
#include "dukscript.hpp"
#include "skillicon.hpp"
#include "skilltree.hpp"

using namespace tynskills;

Skilltree *skilltree;
Dukscript *dukscript;
std::map<nodeid, Skillicon> skillicons;
long config_file_timestamp = 0;
const char *config_filename = RES_PATH "skills.json";
int points_spent = 0;

void UpdateDrawFrame(void);
bool parse_config(Skilltree *skilltree);

int screenWidth = 800;
int screenHeight = 450;

void init() {
  skilltree = new Skilltree();
  dukscript = new Dukscript();

  dukscript->eval("print('Dukscript initialized');");

	points_spent = 0;

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
    //Node *node = skilltree->get_node(id);
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
  DrawText(TextFormat("%d spent", points_spent), 8, GetScreenHeight() - fontsize - 8,
           fontsize, BLACK);
}

void dispose() {
  skilltree->cleanup();
	skillicons.clear();

	// todo: texture uloading
	
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

// ----

#include <vector>

struct SkilliconContructInfo {
  Skillinfo info;
  Vector2 shift;
  std::string follows;
  std::string icon_name;
  std::vector<std::string> branches;
};

bool parse_config(Skilltree *skilltree) {
  config_file_timestamp = GetFileModTime(config_filename);

  const bool parsed = dukscript->parse_json_file(RES_PATH "skills.json");
  if (!parsed) {
    return false;
  }

  std::vector<SkilliconContructInfo> construct_infos;

  std::map<std::string, BranchProgressMode> name_modes = {
      {"any", BranchProgressMode::ANY},
      {"min", BranchProgressMode::MINIMUM},
      {"max", BranchProgressMode::MAXIMUM}};

  while (dukscript->next()) {
    SkilliconContructInfo ci;

    const char *key = dukscript->get_string(-2);

    const char *icon_name = dukscript->get_string_by_key("icon", "UNKNOWN");
    int points = dukscript->get_int_by_key("points", 0);
    int maxpoints = dukscript->get_int_by_key("maxpoints", 4);
    bool active = dukscript->get_bool_by_key("active", false);
    const char *follows = dukscript->get_string_by_key("follows", "");
    const char *smode = dukscript->get_string_by_key("mode", "max");
    float sx = 0;
    float sy = 0;

    if (dukscript->read_object("shift")) {
      sx = dukscript->get_int_by_index(0, 0);
      sy = dukscript->get_int_by_index(1, 0);
    }
    dukscript->pop(); // pop shift
                      //
    if (dukscript->read_object("branches")) {
      int len = dukscript->get_array_length();
      for (int i = 0; i < len; i++) {
        std::string b = dukscript->get_string_by_index(i, "");
        if (b.length()) {
          ci.branches.push_back(b);
        }
      }
    }
    dukscript->pop();  // pop branches
    dukscript->pop(2); // pop key, value

    const BranchProgressMode mode = name_modes[smode];
    Skillinfo s = {.points = points,
                   .maxpoints = maxpoints,
                   .active = active,
                   .mode = mode,
                   .name = key};

    ci.info = s;
    ci.shift = {sx, sy};
    ci.follows = follows;
    ci.icon_name = icon_name;

    construct_infos.push_back(ci);
  }

	// enumerator pop
	// json string pop
  dukscript->pop(2); 

  // --- adding skills into tree

  std::map<std::string, nodeid> name_to_id;
  Vector2 cell = {128.0 + 16.0, 128.0 + 16.0};

  // create leafs icons
  for (const auto &ci : construct_infos) {
    const int leafid = skilltree->add_leaf();
    name_to_id[ci.info.name] = leafid;
    Leaf *leaf = skilltree->get_leaf(leafid);
    leaf->setup(ci.info);

    // pos
    Vector2 pos = {cell.x * ci.shift.x, cell.y * ci.shift.y};
    if (ci.follows.length()) {
      nodeid originid = name_to_id[ci.follows];
      Skillicon *icon = &skillicons[originid];
      const Vector2 origin = icon->get_pos();

      pos.x += origin.x;
      pos.y += origin.y;
    }

    Texture texture =
        LoadTexture(TextFormat(RES_PATH "icons/%s.png", ci.icon_name.c_str()));
    skillicons[leafid] = Skillicon(texture, pos, leafid);
  }

  // create branches
  for (const auto &ci : construct_infos) {
    nodeid leafa = name_to_id[ci.info.name];

    for (const auto &branch : ci.branches) {
      nodeid leafb = -1;
      auto delimiter_find = branch.find(':');
      BranchProgressMode mode = BranchProgressMode::MAXIMUM;
      if (delimiter_find != std::string::npos) {
        auto name = branch.substr(0, delimiter_find);
        auto smode = branch.substr(delimiter_find + 1, branch.length());
        mode = name_modes[smode];
        leafb = name_to_id[name];
      } else {
        leafb = name_to_id[branch];
      }

      // in config branches reversed - they listed in INPUT leafs
      skilltree->add_branch(leafb, leafa, mode);
    }
  }

	return true;
}

