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
