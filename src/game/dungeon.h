#pragma once
#include <vector>
#include "mathtypes.h"

namespace game {

// Tamaño de cada celda de la grilla de referencia (solo visual: el movimiento
// dentro de la mazmorra es libre/continuo, no por grilla).
constexpr float kTileSize = 48.0f;

class Dungeon {
public:
    // Arma una sala de prueba rectangular de anchoTiles x altoTiles, con paredes en el borde.
    // (Placeholder de una sola sala; la generacion por salas conectadas es un paso futuro
    // del roadmap.)
    Dungeon(int anchoTiles, int altoTiles);

    int AnchoTiles() const { return anchoTiles_; }
    int AltoTiles() const { return altoTiles_; }

    const std::vector<Rect>& Paredes() const { return paredes_; }

    // Resuelve la colision de un colisionador movil contra todas las paredes,
    // probando eje por eje (X y luego Y) para poder deslizar en angulo contra
    // una pared en vez de trabarse.
    //
    // Limitacion conocida: esto compara solo la posicion final del movimiento,
    // no hace deteccion continua (CCD). Si posicionDeseada esta muchisimo mas
    // lejos que el grosor de una pared (un salto de un solo frame mayor a
    // kGrosorPared), el colisionador puede "atravesarla" sin detectarse. En la
    // practica esto no pasa porque el movimiento por frame (velocidad * dt) es
    // chico comparado con el grosor de pared; main.cpp ademas clampea dt por
    // las dudas. Si mas adelante se permite teletransporte o dash largos, hay
    // que revisar este metodo.
    Vec2 ResolverColision(Rect colisionadorActual, Vec2 posicionActual, Vec2 posicionDeseada) const;

private:
    int anchoTiles_;
    int altoTiles_;
    std::vector<Rect> paredes_;
};

} // namespace game
