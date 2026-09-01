#pragma once

// Tipos matematicos propios de la capa de logica (game layer).
// A proposito NO se usan los tipos de raylib (Vector2, Rectangle) aca:
// la idea es que game/ pueda migrarse a Unreal sin arrastrar dependencias
// de raylib. La capa de render (src/render) hace la conversion.

namespace game {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

inline Vec2 operator+(const Vec2& a, const Vec2& b) { return { a.x + b.x, a.y + b.y }; }
inline Vec2 operator-(const Vec2& a, const Vec2& b) { return { a.x - b.x, a.y - b.y }; }
inline Vec2 operator*(const Vec2& a, float s) { return { a.x * s, a.y * s }; }

float Length(const Vec2& v);
Vec2 Normalize(const Vec2& v);

// Rectangulo alineado a los ejes (AABB), en coordenadas de mundo (pixeles).
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

bool CheckCollision(const Rect& a, const Rect& b);

} // namespace game
