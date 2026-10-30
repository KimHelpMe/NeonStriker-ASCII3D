#pragma once
// ============================================================
//  아이템 (NEON STRIKER Item을 3D로)
//  적이 죽은 자리에서 플레이어 평면(z=0)까지 천천히 날아온 뒤, 그 평면에서 벽 튕김 (15초 수명)
// ============================================================
#include "Math3D.h"
#include "Config.h"

class Game;

struct Item {
    ItemType type;
    Vec3  pos, dir;
    float lifeTimer;
    bool  active;
};

class ItemSystem {
public:
    void Reset();
    void Drop(Vec3 pos);
    void Update(Game& g, float dt);
    void Draw() const;
private:
    Item pool[ITEM_COUNT];
};
