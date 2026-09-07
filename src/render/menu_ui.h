#pragma once

#include <string>
#include <vector>
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
// VolverAlMapa abandona la mazmorra en curso sin marcarla como superada y
// vuelve a la Ciudad (EstadoJuego::Ciudad — antes volvia directo a
// EstadoJuego::MapaTema; desde que la Ciudad es el hub central, MapaTema
// solo se llega caminando hasta la Entrada a las mazmorras dentro de ella,
// ver ConstruirCiudad en main.cpp), manteniendo el party tal cual esta (con
// su HP/inventario/equipo/oro actual — "sigue con el desgaste" entre
// mazmorras, ver DibujarMapaTema/DibujarMapaDificultad mas abajo) —
// pensada para el jugador que quiere probar otro tema o dificultad sin
// terminar la actual, o simplemente ir a gastar el oro juntado antes de
// seguir. El nombre del enum se mantiene (evita renombrar el case en
// main.cpp) aunque el texto que se dibuja ahora diga "Volver a la ciudad"
// (ver kNombresOpcionesPausa en menu_ui.cpp).
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

// Mapa de mazmorras: dos pasos. Primero se elige un TEMA (Bosque/Carcel/
// Castillo, en ese orden fijo — ver game::Tema en main.cpp, este archivo no
// depende de game/ para mantener render/ desacoplado de la logica de
// generacion) y despues, DENTRO de ese tema, una dificultad (Facil/Media/
// Dificil, mismo orden fijo que game::Dificultad) — 9 combinaciones en
// total, todas elegibles en cualquier orden (no un camino obligado) y
// rejugables sin limite. Ver EstadoJuego::MapaTema/MapaDificultad en
// main.cpp.
constexpr int kNumTemasMapa = 3;
constexpr int kNumMazmorrasMapa = 3;  // dificultades por tema

// Paso 1: elegir tema. 'opcionSeleccionada' (0..kNumTemasMapa-1) lo maneja
// quien llama, igual que en las otras pantallas de este archivo.
// 'progresoPorTema[i]' es cuantas de las kNumMazmorrasMapa dificultades de
// ese tema ya estan superadas esta run (0..kNumMazmorrasMapa) — se muestra
// como "x/3 superadas" en la tarjeta, o nada si todavia es 0. ENTER pasa al
// paso 2 (ver DibujarMapaDificultad); ESC abre la pausa.
void DibujarMapaTema(int anchoVentana, int altoVentana, int opcionSeleccionada,
                      const int progresoPorTema[kNumTemasMapa]);

// Paso 2: elegir dificultad DENTRO del tema ya elegido en el paso 1
// ('temaElegido', 0..kNumTemasMapa-1, solo para el titulo de la pantalla).
// 'opcionSeleccionada' (0..kNumMazmorrasMapa-1) lo maneja quien llama.
// 'superada[i]' son los flags de ESTE tema nomas (el llamador ya extrajo el
// sub-rango correspondiente del array de kNumCombinacionesMapa
// combinaciones, ver game/save.h) — se resetea a todo false al empezar una
// run nueva o al perder del todo (Game Over reinicia la run completa, ver
// EstadoJuego::Combate/FaseCombate::Perdido en main.cpp), pero NO al volver
// al mapa sin terminar una mazmorra (VolverAlMapa en la pausa). ENTER genera
// la mazmorra y entra a explorar; ESC vuelve al paso 1 (no a la pausa).
void DibujarMapaDificultad(int anchoVentana, int altoVentana, int temaElegido, int opcionSeleccionada,
                            const bool superada[kNumMazmorrasMapa]);

// --- Comercio (Herreria/Tienda de la Ciudad, ver EstadoJuego::Herreria/
// Tienda y ConstruirCiudad en main.cpp) ---
// Un item comprable, ya resuelto a texto por quien llama -- main.cpp arma
// esta lista a partir de su propio catalogo de precios (ver
// OfertaComercio/kOfertasHerreria/kOfertasTienda ahi, un tipo DISTINTO de
// este con el mismo nombre pero en namespace game, no confundir), leyendo
// nombre/descripcion del game::Item real (ItemFabrica().nombre/descripcion)
// para no duplicar ese texto a mano aca. Este archivo no incluye game/item.h
// a proposito, mismo desacople que el resto de render/menu_ui.h.
struct OfertaComercio {
    std::string nombre;
    std::string descripcion;
    int precio = 0;
};

// Pantalla de comercio: titulo segun 'esHerreria' (Herreria si true, Tienda
// si false), oro disponible arriba a la derecha, y lista numerada [1]-[9]
// de 'ofertas' con su precio (atenuado si el oro actual no alcanza para
// esa oferta). 'mensaje' (puede venir vacio) muestra el resultado de la
// ultima compra intentada en esta visita a la pantalla (p.ej. "Compraste:
// Pocion de Curacion Menor." o "No te alcanza el oro para Daga Veloz.") —
// main.cpp decide que tecla numerica dispara la compra, resuelve el precio
// contra party.Oro() y arma 'mensaje'; esta funcion solo dibuja. Sin
// BeginDrawing/EndDrawing propios, mismo criterio que el resto de este
// archivo. ESC vuelve a EstadoJuego::Ciudad (ver main.cpp).
void DibujarComercio(int anchoVentana, int altoVentana, bool esHerreria, const std::vector<OfertaComercio>& ofertas,
                      int oro, const std::string& mensaje);

}  // namespace ui
