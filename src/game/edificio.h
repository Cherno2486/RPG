#pragma once
#include "mathtypes.h"

namespace game {

// Un edificio interactuable dentro de la Ciudad (ver EstadoJuego::Ciudad en
// main.cpp) -- el layout de la Ciudad es siempre estatico (ver
// ConstruirCiudad en main.cpp: una unica plaza hecha con el constructor de
// datos-ya-resueltos de Dungeon, ver dungeon.h), asi que estos edificios se
// reconstruyen identicos cada vez que se entra a la Ciudad y no hace falta
// guardarlos en el sistema de guardado (game/save.h).
enum class TipoEdificio { Herreria, Tienda, EntradaMazmorras, Academia };

struct Edificio {
    TipoEdificio tipo = TipoEdificio::Herreria;
    Vec2 posicion;
};

// Nombre para mostrar sobre el edificio en el mapa y en el prompt de
// interaccion ("[E] Entrar a la Herreria", ver kDistanciaInteraccion en
// main.cpp). Header-only (inline) para no sumar un .cpp/CMakeLists nuevo
// por un mapeo tan chico.
inline const char* NombreDeEdificio(TipoEdificio tipo) {
    switch (tipo) {
        case TipoEdificio::Herreria:         return "Herreria";
        case TipoEdificio::Tienda:           return "Tienda";
        case TipoEdificio::Academia:         return "Academia";
        default:                             return "Entrada a las mazmorras";  // EntradaMazmorras
    }
}

} // namespace game
