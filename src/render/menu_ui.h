#pragma once

#include <string>

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
enum class OpcionPausa { Continuar, Guardar, MenuPrincipal, Salir };
constexpr int kNumOpcionesPausa = 4;

// Pantalla de pausa: titulo del juego + las 4 opciones de arriba.
// 'opcionSeleccionada' (0..kNumOpcionesPausa-1) lo maneja quien llama, igual
// que en DibujarMenuInicio. 'mensajeGuardado' (puede venir vacio) muestra un
// cartel chico debajo de las opciones con el resultado de la ultima vez que
// se eligio "Guardar" en esta pausa (p.ej. "Partida guardada.") — main.cpp
// es quien decide cuando ponerlo y cuando limpiarlo. Sin BeginDrawing/
// EndDrawing propios, mismo criterio que las otras pantallas de este archivo.
void DibujarPausa(int anchoVentana, int altoVentana, int opcionSeleccionada, const std::string& mensajeGuardado);

}  // namespace ui
