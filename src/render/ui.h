#pragma once
#include "../game/party.h"

namespace ui {
// Panel simple arriba a la izquierda con nombre, rol y HP de cada miembro del party.
void DibujarPanelParty(const game::Party& party);
}
