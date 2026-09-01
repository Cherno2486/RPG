#pragma once
#include "../game/mathtypes.h"

namespace input {
// Lee WASD / flechas y devuelve un vector de direccion normalizado (0,0 si no hay input).
game::Vec2 LeerDireccionMovimiento();
}
