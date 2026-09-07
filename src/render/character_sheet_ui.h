#pragma once
#include "../game/party.h"
#include "sprites.h"

namespace ui {

// Ficha de personajes: pantalla completa (como ui::DibujarInventario) con
// el detalle completo de los 4 miembros del party — nivel y progreso de
// experiencia, HP y recurso, los 3 stats de combate que hasta ahora no se
// mostraban en ningun lado como numero (Ataque/Defensa/Velocidad), y el
// equipo puesto en cada ranura. Reemplaza al viejo panel expandido (ver
// game::Character::Nivel()/Experiencia() y "Ficha de personajes" en
// docs/design.md) — pensada para llamarse en vez del frame normal de
// exploracion mientras esta abierta (ver main.cpp), igual que el
// inventario. 'sprites' es el mismo SpriteSet del mapa (ver
// render::Renderer::Sprites()), usado para el retrato de cada uno.
void DibujarFichaPersonajes(const game::Party& party, const render::SpriteSet& sprites);

} // namespace ui
