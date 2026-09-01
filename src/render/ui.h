#pragma once
#include "../game/party.h"

namespace ui {
// Panel con nombre, rol y HP de cada miembro del party, arriba a la
// izquierda. 'expandido' controla si se muestra completo (con nombre/rol/
// barra de HP grande) o una version compacta (solo un circulo + barrita
// chica por miembro) para no taparle el mapa al jugador todo el tiempo.
void DibujarPanelParty(const game::Party& party, bool expandido);
}
