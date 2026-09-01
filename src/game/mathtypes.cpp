#include "mathtypes.h"
#include <cmath>

namespace game {

float Length(const Vec2& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

Vec2 Normalize(const Vec2& v) {
    float len = Length(v);
    if (len < 0.0001f) return { 0.0f, 0.0f };
    return { v.x / len, v.y / len };
}

bool CheckCollision(const Rect& a, const Rect& b) {
    return a.x < b.x + b.width &&
           a.x + a.width > b.x &&
           a.y < b.y + b.height &&
           a.y + a.height > b.y;
}

} // namespace game
