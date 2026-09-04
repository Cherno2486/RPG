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

// Dibuja el sub-menu de "[3] Usar item" encima de la pantalla de combate ya
// dibujada (llamar DESPUES de DibujarCombate, mientras el sub-menu esta
// abierto -- ver main.cpp): la lista de Consumibles del inventario
// compartido del party (con un numero al lado, mismo criterio que
// ui::DibujarInventario) y, para el aliado elegido con [TAB]
// ('indiceAliadoObjetivo'), su ficha resaltada -- los consumibles que
// apuntan a un enemigo (Bomba de Veneno) en cambio se aplican sobre el
// objetivo de combate ya elegido en la pantalla principal, sin necesidad de
// elegir aliado.
void DibujarSubmenuUsarItem(const game::Party& party, size_t indiceAliadoObjetivo);

} // namespace ui
