#pragma once
#include "../game/party.h"
#include "sprites.h"

namespace ui {

// Dibuja la pantalla de inventario: la lista de items apilados (con un
// numero al lado de cada uno para usarlo) y la fila de miembros del party,
// resaltando a 'indiceObjetivo' como quien va a recibir el efecto del
// proximo item que se use. Pensado para llamarse en vez del frame normal de
// exploracion mientras el inventario esta abierto (ver main.cpp). 'sprites'
// es el mismo SpriteSet del mapa (ver render::Renderer::Sprites()), usado
// para el retrato de cada miembro.
void DibujarInventario(const game::Party& party, size_t indiceObjetivo, const render::SpriteSet& sprites);

} // namespace ui
