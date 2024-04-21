#if 0
nodeid make_skill(const char *name, Vector2 pos) {
  nodeid leafid = skilltree->add_leaf();
  Texture texture = LoadTexture(TextFormat(RES_PATH "icons/%s.png", name));
  skillicons[leafid] = Skillicon(texture, pos, leafid);

  return leafid;
}

nodeid
make_skill_next(nodeid rootid, const char *name, Vector2 shift, Skillinfo info,
                BranchProgressMode branchmode = BranchProgressMode::MAXIMUM) {
  const Skillicon *rooticon = &skillicons[rootid];
  Vector2 cell = {128.0 + 16.0, 128.0 + 16.0};
  Vector2 pos = Vector2Add(rooticon->get_pos(), Vector2Multiply(shift, cell));
  nodeid leafid = make_skill(name, pos);
  Leaf *leaf = skilltree->get_leaf(leafid);
  leaf->setup(info);
  skilltree->add_branch(rootid, leafid, branchmode);

  return leafid;
}

nodeid make_skill_next_fast(
    nodeid rootid, const char *name, Vector2 shift,
    BranchProgressMode branchmode = BranchProgressMode::MAXIMUM) {
  const Skillinfo info = {.points = 0,
                          .maxpoints = 5,
                          .active = false,
                          .mode = BranchProgressMode::MAXIMUM};

  return make_skill_next(rootid, name, shift, info, branchmode);
}

void manual_skills_gen() {
  nodeid skill1 = make_skill("SGI_01", (Vector2){0.0, 0.0});
  skilltree->get_leaf(skill1)->setup(0, 5, true);
  nodeid skill2 = make_skill_next_fast(skill1, "SGI_02", {1.0, 0.0},
                                       BranchProgressMode::MINIMUM);
  nodeid skill3 = make_skill_next_fast(skill2, "SGI_03", {0.0, 1.0},
                                       BranchProgressMode::MINIMUM);
  nodeid skill4 = make_skill_next_fast(skill3, "SGI_04", {0.0, 1.0},
                                       BranchProgressMode::MINIMUM);
  nodeid skill5 = make_skill_next_fast(skill3, "SGI_05", {-1.0, 1.0},
                                       BranchProgressMode::ANY);
  skilltree->add_branch(skill1, skill5);
  nodeid skill6 = make_skill_next_fast(skill3, "SGI_06", {1.0, 0.0});
  nodeid skill7 = make_skill_next_fast(skill6, "SGI_07", {1.0, 1.0});
  nodeid skill8 = make_skill_next_fast(skill6, "SGI_08", {1.0, -1.0});
  skilltree->add_branch(skill2, skill8, BranchProgressMode::MAXIMUM);
  nodeid skill9 = make_skill_next_fast(skill6, "SGI_09", {0.0, 1.0},
                                       BranchProgressMode::ANY);
  nodeid skill10 = make_skill_next_fast(skill7, "SGI_10", {1.0, 0.0});
  nodeid skill11 = make_skill_next_fast(skill9, "SGI_11", {0.0, 2.0});
  // nodeid skill12 = make_skill_next_fast(skill9, "SGI_12", {-1.0, 0.0});
  nodeid skill13 = make_skill_next_fast(skill4, "SGI_13", {0.0, 1.0});
  skilltree->add_branch(skill13, skill11, BranchProgressMode::MINIMUM);
}
#endif

#if 0
#include "external/INIReader.h"
#include <sstream>
#include <string>

/**
 * ini entries sorted by name. All names has to be ordered carefully
 *
 * @param skilltree
 */
void _parse_config(Skilltree *skilltree) {
  INIReader reader(config_filename);

  config_file_timestamp = GetFileModTime(config_filename);

  if (reader.ParseError() < 0) {
    TraceLog(LOG_ERROR,
             TextFormat("ini read parse error #%d", reader.ParseError()));
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

    const BranchProgressMode mode =
        name_modes[reader.Get(section, "mode", "max")];
    Skillinfo s = {.points = (int)reader.GetInteger(section, "points", 0),
                   .maxpoints = (int)reader.GetInteger(section, "maxpoints", 4),
                   .active = reader.GetBoolean(section, "active", false),
                   .mode = mode,
                   .name = section};
    leaf->setup(s);

    Vector2 pos = {0.0, 0.0};
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
    Texture texture =
        LoadTexture(TextFormat(RES_PATH "icons/%s.png", icon_name.c_str()));

    skillicons[leafid] = Skillicon(texture, pos, leafid);
  }

  // load branches
  for (std::set<std::string>::iterator sectionsIt = sections.begin();
       sectionsIt != sections.end(); sectionsIt++) {
    auto branches = reader.Get(*sectionsIt, "branches", "");
    if (!branches.length()) {
      continue;
    }

    std::stringstream ss(branches);
    std::string item;

    nodeid leafa = name_to_id[*sectionsIt];
    while (getline(ss, item, delimiter)) {
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
#endif

#if 0
	if(dukscript->call_prepare("test")) {
		dukscript->push_string("aaa");
		if(dukscript->call(1)) {
			const char *ret = dukscript->get_string();
			dukscript->pop();
		}
		dukscript->pop();
	}

#endif
