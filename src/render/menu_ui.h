#pragma once

namespace ui {

// Cantidad de opciones del menu de inicio (Jugar, Salir) — expuesta para que
// quien lee el input (main.cpp) pueda ciclar la seleccion con el mismo
// numero que usa DibujarMenuInicio para dibujarlas, sin duplicar la lista de
// opciones en dos lugares distintos.
constexpr int kNumOpcionesMenuInicio = 2;

// Pantalla de inicio: titulo del juego + menu con las opciones "Jugar" y
// "Salir". 'opcionSeleccionada' (0 = Jugar, 1 = Salir) lo maneja quien
// llama (main.cpp lee las flechas/W-S y ENTER, y mueve la seleccion); esta
// funcion solo dibuja. Sin BeginDrawing/EndDrawing propios, para que
// main.cpp pueda pintar la mazmorra generada de fondo primero (mismo truco
// visual que ya usa la pantalla de combate para "congelar" la escena detras
// del overlay).
void DibujarMenuInicio(int anchoVentana, int altoVentana, int opcionSeleccionada);

}  // namespace ui
