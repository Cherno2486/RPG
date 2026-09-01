#pragma once
#include "../game/combat.h"

namespace ui {

// Dibuja toda la pantalla de combate: party, enemigo, barras de vida/recurso,
// efectos activos, el log de la pelea, y el menu de accion del aliado en
// turno (si corresponde).
void DibujarCombate(game::CombatEncounter& encuentro, int anchoVentana, int altoVentana);

} // namespace ui
