#pragma once
#include "../game/party.h"
#include "sprites.h"

namespace ui {
// Panel con retrato, nombre, rol y HP de cada miembro del party, arriba a
// la izquierda. 'expandido' controla si se muestra completo (con nombre/
// rol/barra de HP grande) o una version compacta (solo un retrato + barrita
// chica por miembro) para no taparle el mapa al jugador todo el tiempo.
// 'sprites' es el mismo SpriteSet que ya usa Renderer para el mapa (ver
// render::Renderer::Sprites()) — los retratos reusan el sprite de personaje
// a tamaño chico en vez de duplicar texturas propias.
void DibujarPanelParty(const game::Party& party, bool expandido, const render::SpriteSet& sprites);
}
