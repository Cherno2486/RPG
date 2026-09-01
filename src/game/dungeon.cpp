#include "dungeon.h"

namespace game {

namespace {
constexpr float kGrosorPared = 24.0f;
}

Dungeon::Dungeon(int anchoTiles, int altoTiles)
    : anchoTiles_(anchoTiles), altoTiles_(altoTiles) {
    float anchoPx = anchoTiles_ * kTileSize;
    float altoPx = altoTiles_ * kTileSize;

    // Pared superior, inferior, izquierda, derecha, como rectangulos gruesos en el borde.
    paredes_.push_back(Rect{0, 0, anchoPx, kGrosorPared});
    paredes_.push_back(Rect{0, altoPx - kGrosorPared, anchoPx, kGrosorPared});
    paredes_.push_back(Rect{0, 0, kGrosorPared, altoPx});
    paredes_.push_back(Rect{anchoPx - kGrosorPared, 0, kGrosorPared, altoPx});
}

Vec2 Dungeon::ResolverColision(Rect colisionadorActual, Vec2 posicionActual, Vec2 posicionDeseada) const {
    Vec2 resultado = posicionActual;

    // --- Eje X ---
    Vec2 intentoX = { posicionDeseada.x, posicionActual.y };
    Rect rectX = colisionadorActual;
    rectX.x += (intentoX.x - posicionActual.x);
    bool bloqueadoX = false;
    for (const auto& pared : paredes_) {
        if (CheckCollision(rectX, pared)) { bloqueadoX = true; break; }
    }
    if (!bloqueadoX) resultado.x = intentoX.x;

    // --- Eje Y (ya con el resultado de X aplicado) ---
    Vec2 intentoY = { resultado.x, posicionDeseada.y };
    Rect rectY = colisionadorActual;
    rectY.x += (resultado.x - posicionActual.x);
    rectY.y += (intentoY.y - posicionActual.y);
    bool bloqueadoY = false;
    for (const auto& pared : paredes_) {
        if (CheckCollision(rectY, pared)) { bloqueadoY = true; break; }
    }
    if (!bloqueadoY) resultado.y = intentoY.y;

    return resultado;
}

} // namespace game
