#pragma once
#include <vector>
#include "mathtypes.h"

namespace game {

// Tamaño de cada celda de la grilla de referencia (solo visual: el movimiento
// dentro de la mazmorra es libre/continuo, no por grilla).
constexpr float kTileSize = 48.0f;

// Una sala del layout generado, en coordenadas de tile (no de pixeles).
struct Habitacion {
    int x = 0;
    int y = 0;
    int ancho = 0;
    int alto = 0;
};

// Mazmorra procedural: una cadena de salas de distinto tamaño (elegidas al
// azar de un set fijo de "room templates"), conectadas por pasillos rectos,
// que se van extendiendo siempre hacia el Este o el Sur — así nunca hace
// falta detectar superposición entre salas, cada una queda garantizada en
// espacio libre. Ver docs/design.md, sección "Sistema de mazmorras", para
// el detalle de diseño y las limitaciones conocidas (layout siempre en
// cadena, sin loops ni ramificaciones, todavía).
class Dungeon {
public:
    Dungeon();

    const std::vector<Rect>& Paredes() const { return paredes_; }
    const std::vector<Habitacion>& Habitaciones() const { return habitaciones_; }

    // Centro de la sala 'indice' (mismo orden que Habitaciones()), en
    // pixeles. El indice 0 es la sala inicial, donde arranca el party y
    // que no tiene enemigo.
    Vec2 CentroDeSala(size_t indice) const;

    // Resuelve la colision de un colisionador movil contra todas las
    // paredes, probando eje por eje (X y luego Y) para poder deslizar en
    // angulo contra una pared en vez de trabarse.
    //
    // Misma limitacion conocida que antes: comparar solo la posicion final
    // del movimiento (no hay deteccion continua/CCD). No es un problema en
    // la practica porque el movimiento por frame es chico y main.cpp
    // clampea dt.
    Vec2 ResolverColision(Rect colisionadorActual, Vec2 posicionActual, Vec2 posicionDeseada) const;

private:
    std::vector<Habitacion> habitaciones_;
    std::vector<Rect> paredes_;
};

} // namespace game
