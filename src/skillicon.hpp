#include "raylib.h"
#include "raymath.h"
#include "skilltree.hpp"

namespace tynskills {
class Skillicon {
  Texture texture;
  Vector2 pos;
  int leafid;

public:
  Skillicon(Texture texture, Vector2 pos, int leafid) {
    this->texture = texture;
    this->pos = pos;
    this->leafid = leafid;
  }
	Skillicon() {
    this->texture = {};
    this->pos = {};
    this->leafid = -1;
	}
	Skillicon(const Skillicon &s) {
    this->texture = s.texture;
    this->pos = s.pos;
    this->leafid = s.leafid;
	}

  int get_id() { return this->leafid; }

  Rectangle get_rect(Vector2 origin) const {
    return {origin.x + this->pos.x, origin.y + this->pos.y, 128.0, 128.0};
  }

	Vector2 get_center(Vector2 origin) const {
		const Rectangle rect = this->get_rect(origin);
		const Vector2 v = { rect.x + rect.width / 2 ,
		 	rect.y + rect.height / 2 };

		return v;
	}

  void draw(Leaf *leaf, Rectangle dest, bool highlight) const {
    Rectangle source = {0.0, 0.0, (float)this->texture.width,
                        (float)this->texture.height};
    Color color = leaf->is_active() ? WHITE : GRAY;
    Color outline_color = highlight && leaf->is_active() ? RED : BLACK;

    DrawTexturePro(this->texture, source, dest, Vector2Zero(), 0.0, color);
    DrawRectangleLinesEx(dest, 2.0, outline_color);

    if (leaf->is_active()) {
      DrawText(TextFormat("%d/%d", leaf->get_points(), leaf->get_maxpoints()),
               dest.x + 8, dest.y + 8, 16, WHITE);
    } else {
      DrawText("---", dest.x + 8, dest.y + 8, 16, WHITE);
    }
  }
};
} // namespace tynskills
