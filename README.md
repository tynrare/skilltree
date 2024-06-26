# Skilltree

testin' skill trees

[web demo](https://tynroar-skilltree.netlify.app/)

# build

- Raylib has to be compiled with `cmake -DGRAPHICS=GRAPHICS_API_OPENGL_ES2 ..` option to work properly with glsl 100 version
- To build release version run `cmake -DCMAKE_BUILD_TYPE=Release ..`
- By default, debug version 'res' folder used directly. In release version 'build/res' directory used.

# src usage example

## Skilltree

Required: `src/graph.hpp`, `src/graph.cpp`, `src/skilltree.hpp`.

### Minimal example

```cpp

#include "skilltree.hpp"

void init(Skilltree *skilltree) {
  nodeid leaf_ida = skilltree->add_leaf();
  nodeid leaf_idb = skilltree->add_leaf();
  
  const Skillinfo info_a = { 
      .points = 0,
      .maxpoints = 5,
      .active = false,
      .mode = BranchProgressMode::MAXIMUM
      .name = "anything",
      .bind = "anywhere",
  }

  const Skillinfo info_b = { /* ... */ }

  skilltree->get_leaf(leaf_ida)->setup(info_a);
  skilltree->get_leaf(leaf_idb)->setup(info_b);

  skilltree->add_branch(leaf_ida, leaf_idb, BranchProgressMode::MAXIMUM);
}

int upgrade(Skilltree *skilltree, int id), int direction) {
    int points_spent = skilltree->get_leaf(id)->upgrade(direction);

    // returns negative number when downgrading (directive negative)
    // returns zero if there's no tree changes
    points_spent += skilltree->refresh_leaf(id);

    return points_spent;
}

```

### Progress modes

There three progress modes applied to leafs and branches:

- `BranchProgressMode::ANY`
- `BranchProgressMode::MINIMUM`
- `BranchProgressMode::MAXIMUM`

...
- `ANY` and `MINIMUM` leaf mode activates leaf when one or more input branches is active. `MAXIMUM` mode requires all input branches to be active.
- `ANY` branch mode activates branch whenever input leaf is active (even with 0 points). `MINIMUM` mode requires at least one point in input leaf to activate branch. `MAXIMUM` points requires input leaf to be fully upgraded.


