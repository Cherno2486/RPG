#pragma once

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

}  // namespace ui
