#include "input.h"
#include "raylib.h"

namespace input {

game::Vec2 LeerDireccionMovimiento() {
    game::Vec2 direccion{ 0.0f, 0.0f };

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    direccion.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  direccion.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  direccion.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) direccion.x += 1.0f;

    return game::Normalize(direccion);
}

} // namespace input
