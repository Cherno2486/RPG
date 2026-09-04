#pragma once

#include <string>
#include "../game/save.h"  // game::kNumSlots

namespace ui {

// Opciones del menu de inicio, en el orden en que se dibujan y se ciclan
// (main.cpp castea el indice de seleccion directo a este enum). La lista es
// SIEMPRE de 4 — a diferencia de la primera version de este menu, "Cargar"
// no desaparece cuando no hay partida guardada, se dibuja deshabilitada
// (ver DibujarMenuInicio) para que el jugador siempre vea la forma "tipica"
// del menu.
enum class OpcionMenuInicio { NuevaPartida, Cargar, SobreMi, Salir };
constexpr int kNumOpcionesMenuInicio = 4;

// Pantalla de inicio: titulo del juego + las 4 opciones de arriba.
// 'opcionSeleccionada' (0..kNumOpcionesMenuInicio-1) lo maneja quien llama
// (main.cpp lee las flechas/W-S y ENTER/ESPACIO, y mueve la seleccion); esta
// funcion solo dibuja, no decide nada. 'hayPartidaGuardada' en false dibuja
// "Cargar" atenuada — el cursor igual puede pararse ahi (con un cartel
// aclarando por que no hace nada si se confirma), pero main.cpp es quien
// decide no hacer nada al confirmarla en ese estado. Sin
// BeginDrawing/EndDrawing propios, para que main.cpp pueda pintar la
// mazmorra generada de fondo primero (mismo truco visual que ya usa la
// pantalla de combate para "congelar" la escena detras del overlay).
void DibujarMenuInicio(int anchoVentana, int altoVentana, int opcionSeleccionada, bool hayPartidaGuardada);

// Pantalla placeholder de "Sobre Mi" (todavia sin contenido definido — ver
// Roadmap en docs/design.md). Mismo criterio visual y de dibujo que
// DibujarMenuInicio (sin BeginDrawing/EndDrawing propios).
void DibujarSobreMi(int anchoVentana, int altoVentana);

// Opciones del menu de pausa (ver DibujarPausa), en el orden en que se
// dibujan y se ciclan — mismo criterio de casteo directo indice->enum que
// OpcionMenuInicio. Se llega aca con ESC durante la exploracion (ver
// EstadoJuego::Pausa en main.cpp); "Guardar" reusa la misma logica que ya
// tenia la tecla F5, "MenuPrincipal" vuelve a EstadoJuego::MenuInicio sin
// cerrar el juego (a diferencia de la primera version de este menu, ahora
// SI se puede volver del gameplay al menu de inicio).
// VolverAlMapa (nueva) abandona la mazmorra en curso sin marcarla como
// superada y vuelve a EstadoJuego::Mapa, manteniendo el party tal cual esta
// (con su HP/inventario/equipo actual — "sigue con el desgaste" entre
// mazmorras, ver DibujarMapa) — pensada para el jugador que quiere probar
// otra dificultad sin terminar la actual. Si la pausa se abrio desde el
// propio Mapa (no hay mazmorra en curso), es equivalente a "Continuar".
enum class OpcionPausa { Continuar, Guardar, VolverAlMapa, MenuPrincipal, Salir };
constexpr int kNumOpcionesPausa = 5;

// Pantalla de pausa: titulo del juego + las 4 opciones de arriba.
// 'opcionSeleccionada' (0..kNumOpcionesPausa-1) lo maneja quien llama, igual
// que en DibujarMenuInicio. 'mensajeGuardado' (puede venir vacio) muestra un
// cartel chico debajo de las opciones con el resultado de la ultima vez que
// se eligio "Guardar" en esta pausa (p.ej. "Partida guardada.") — main.cpp
// es quien decide cuando ponerlo y cuando limpiarlo. Sin BeginDrawing/
// EndDrawing propios, mismo criterio que las otras pantallas de este archivo.
void DibujarPausa(int anchoVentana, int altoVentana, int opcionSeleccionada, const std::string& mensajeGuardado);

// Pantalla de seleccion de slot: se llega aca desde F5 o "Guardar" en la
// pausa (modoGuardar=true) o desde "Cargar" en el menu de inicio
// (modoGuardar=false) — ver EstadoJuego::SeleccionSlot en main.cpp. Muestra
// los game::kNumSlots slots mas una opcion "Volver", en el mismo estilo que
// las otras pantallas de este archivo (sin BeginDrawing/EndDrawing propios).
// 'opcionSeleccionada' (0..kNumOpcionesSlot-1, los primeros kNumSlots son
// los slots y el ultimo es "Volver") lo maneja quien llama, igual que en
// las demas pantallas. 'ocupado[i]' dice si el slot i tiene partida
// guardada — en modo Cargar, un slot vacio se dibuja atenuado (mismo
// criterio que "Cargar" en DibujarMenuInicio cuando no hay ningun
// guardado). 'mensaje' (puede venir vacio) muestra el resultado de la
// ultima accion sobre un slot en esta misma visita a la pantalla (p.ej.
// "Guardado en Slot 2." o "Ese slot esta vacio.") — main.cpp decide cuando
// ponerlo y cuando limpiarlo.
constexpr int kNumOpcionesSlot = game::kNumSlots + 1;  // los slots + "Volver"

void DibujarSeleccionSlot(int anchoVentana, int altoVentana, int opcionSeleccionada, bool modoGuardar,
                           const bool ocupado[game::kNumSlots], const std::string& mensaje);

// Mapa de mazmorras: 3 mazmorras seleccionables (Facil/Media/Dificil, en ese
// orden fijo — ver game::Dificultad en main.cpp, este archivo no depende de
// game/ para mantener render/ desacoplado de la logica de generacion),
// elegibles en cualquier orden (no un camino obligado) — ver
// EstadoJuego::Mapa en main.cpp. 'opcionSeleccionada' (0..
// kNumMazmorrasMapa-1) lo maneja quien llama, igual que en las otras
// pantallas de este archivo. 'superada[i]' marca con un check la mazmorra i
// si ya se gano esta run — se resetea a todo false al empezar una run nueva
// o al perder del todo (Game Over reinicia la run completa, ver
// EstadoJuego::Combate/FaseCombate::Perdido en main.cpp), pero NO al volver
// al mapa sin terminar una mazmorra (VolverAlMapa en la pausa).
constexpr int kNumMazmorrasMapa = 3;
void DibujarMapa(int anchoVentana, int altoVentana, int opcionSeleccionada, const bool superada[kNumMazmorrasMapa]);

}  // namespace ui
