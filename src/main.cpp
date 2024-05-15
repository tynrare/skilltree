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
#include "dust.hpp"
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

  dukscript->eval_file(RES_PATH "skills.js");
  points_spent = 0;

  parse_config(skilltree);
  return;
}

Vector2 pad = {16.0, 16.0};

const char *get_leaf_desription(Leaf *l) {
  const char *str = NULL;
  const char *funcname = l->get_bind().c_str();
  int rc = dukscript->call(funcname, [=]() {
    dukscript->push_int(l->get_points());
    dukscript->push_int(l->get_maxpoints());

    return 2;
  });

  if (rc) {
    str = dukscript->get_string();
    dukscript->pop(2);
  }

  return str;
}

void draw() {
  const int fontsize = 20;
  Vector2 mouse = GetMousePosition();
  bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  bool clicked_second = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

  Leaf *selected_leaf = nullptr;

  // draw branches in first pass (z-index 0)
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

  // draw icons
  for (auto &[id, icon] : skillicons) {
    Leaf *leaf = skilltree->get_leaf(id);
    // Node *node = skilltree->get_node(id);
    const Rectangle rect = icon.get_rect(pad);

    // user interact
    bool collision = CheckCollisionPointRec(mouse, rect) && leaf->is_active();
    if (collision) {
      selected_leaf = leaf;
    }
    // draw icon
    icon.draw(leaf, rect, collision);
  }

  // upgrade logic, user interact.
  // Icon draw will be delayed on one frame
  if (selected_leaf) {
    int direction = 0;
    if (clicked) {
      direction = 1;
    } else if (clicked_second) {
      direction = -1;
    }

    if (direction != 0) {
      const int delta = selected_leaf->upgrade(direction);
      points_spent += delta;
      points_spent += skilltree->refresh_leaf(selected_leaf->get_id());
    }
  }

  // draw leaf text fetchet from js
  if (selected_leaf != nullptr && selected_leaf->has_bind()) {
    const auto bind = selected_leaf->get_bind();
    const char *description = get_leaf_desription(selected_leaf);
    if (description != NULL) {
      Rectangle rect = {mouse.x + 8, mouse.y + 8, 256 - 8, 256 - 8};
      DrawRectangle(mouse.x, mouse.y, 256, 256, Fade(BLACK, 0.7));
      DrawTextBoxed(GetFontDefault(), description, rect, fontsize, 1, true,
                    WHITE);
      // DrawText(description, mouse.x + 6, mouse.y + 6, fontsize, WHITE);
    }
  }

  // canvas drag
  if (selected_leaf == nullptr && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    Vector2 delta = GetMouseDelta();
    pad = Vector2Add(delta, pad);
  }

  // UI, mouse
  DrawText(TextFormat("%d spent", points_spent), 8,
           GetScreenHeight() - fontsize - 8, fontsize, BLACK);

  DrawCircle(mouse.x, mouse.y, 8, WHITE);
  DrawCircle(mouse.x, mouse.y, 6, BLACK);
}

void dispose() {
  skilltree->cleanup();
  skillicons.clear();

  // todo: texture uloading

  delete skilltree;
  delete dukscript;
}

//----------------------------------------------------------------------------------
// Main Entry Point
//----------------------------------------------------------------------------------
int main() {

  // Initialization
  //--------------------------------------------------------------------------------------
  InitWindow(screenWidth, screenHeight, "tynroar skilltree");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  init();

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
  SetTargetFPS(60);
  HideCursor();

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
    const char *bind = dukscript->get_string_by_key("bind", "");
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
      dukscript->pop(); // pop shift
    }
    //
    if (dukscript->read_object("branches")) {
      int len = dukscript->get_array_length();
      for (int i = 0; i < len; i++) {
        std::string b = dukscript->get_string_by_index(i, "");
        if (b.length()) {
          ci.branches.push_back(b);
        }
      }
      dukscript->pop(); // pop branches
    }

    dukscript->pop(2); // pop key, value

    const BranchProgressMode mode = name_modes[smode];
    Skillinfo s = {.points = points,
                   .maxpoints = maxpoints,
                   .active = active,
                   .mode = mode,
                   .name = key,
                   .bind = bind};

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
