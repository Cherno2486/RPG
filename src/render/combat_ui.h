#pragma once
#include "../game/combat.h"
#include "sprites.h"

namespace ui {

// Dibuja toda la pantalla de combate: party, enemigo, barras de vida/recurso,
// efectos activos, el log de la pelea, y el menu de accion del aliado en
// turno (si corresponde). 'deltaSeconds' se usa para animar el feedback
// visual (numeritos de daño/curacion flotantes y el flash al recibir un
// golpe) que se genera a partir de encuentro.UltimosEventos(). 'sprites' es
// el mismo SpriteSet del mapa (ver render::Renderer::Sprites()), usado para
// el retrato de cada aliado en su ficha.
void DibujarCombate(game::CombatEncounter& encuentro, int anchoVentana, int altoVentana, float deltaSeconds,
                     const render::SpriteSet& sprites);

// Descarta cualquier numerito/flash que hubiera quedado animando de un
// combate anterior. Hay que llamarla una vez, justo despues de crear un
// game::CombatEncounter nuevo (antes del primer DibujarCombate sobre el) —
// sin esto, un combate que arranca reutilizando la misma direccion de
// memoria que uno recien terminado podria heredar su feedback visual a
// medio desvanecer.
void ReiniciarFeedbackVisual();

} // namespace ui
