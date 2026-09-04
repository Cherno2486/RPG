#pragma once
#include <string>
#include <vector>
#include "dungeon.h"
#include "party.h"
#include "enemy.h"
#include "item.h"

// Guardado de partida: persiste el estado completo de una run en curso a un
// archivo de texto plano (sin depender de ninguna libreria externa de
// serializacion, en linea con el resto del proyecto), para poder cerrar el
// juego y retomar despues eligiendo "Cargar" en el menu de inicio.
//
// Que se guarda: la mazmorra ya generada (salas + paredes + trampas ya
// resueltas, no la "receta" para regenerarla — ver el comentario de
// Dungeon::Dungeon con datos ya calculados en dungeon.h), los 4 miembros del
// party (stats actuales, posicion, equipo), el inventario compartido, los
// enemigos (posicion, stats, sala, si ya fueron derrotados), los cofres
// (posicion, contenido, si ya se abrieron), y el progreso en el mapa de
// mazmorras (que mazmorras ya se ganaron esta run, y si la partida se
// guardo parada en el mapa o a mitad de una mazmorra — ver DatosPartida
// mas abajo).
//
// Que NO se guarda: nada de un combate en curso — solo se puede guardar
// durante la exploracion (ver la tecla F5 y la pausa en main.cpp), asi que
// nunca hay un CombatEncounter activo en ese momento.
//
// Slots: antes era un unico archivo que siempre se pisaba; ahora hay
// kNumSlots archivos independientes (1..kNumSlots, elegidos por el jugador
// en la pantalla de seleccion — ver render/menu_ui.h::DibujarSeleccionSlot),
// cada uno con el mismo formato de siempre. Las funciones de mas abajo
// toman el slot como un indice 0-based (0 = "Slot 1" en la UI, etc).
namespace game {

constexpr int kNumSlots = 3;

// Ruta del archivo de guardado de 'slot' (0..kNumSlots-1), relativa al
// directorio desde donde se corre el ejecutable — el mismo que ya usan los
// assets (ver README). No valida el rango: quien llama debe pasar un slot
// valido (main.cpp solo expone kNumSlots opciones en la UI).
std::string RutaGuardado(int slot);

// Datos ya reconstruidos por CargarPartida, listos para que main.cpp arme
// la mazmorra/party/enemigos/cofres de la partida en curso.
// Cantidad de mazmorras del mapa (Facil/Media/Dificil, ver game::Dificultad
// en main.cpp) — vive aca (no en un header de main.cpp) porque save.h/.cpp
// necesita el tamano fijo para el array 'mazmorraSuperada' de mas abajo.
constexpr int kNumMazmorrasMapa = 3;

struct DatosPartida {
    std::vector<Habitacion> habitaciones;
    std::vector<Rect> paredes;
    std::vector<Trampa> trampas;
    std::vector<Character> miembros;  // mismo orden que CrearPartyDeEjemplo en main.cpp
    std::vector<PilaItem> pilasInventario;
    std::vector<Enemy> enemigos;
    std::vector<Cofre> cofres;

    // --- Mapa de mazmorras (ver EstadoJuego::Mapa en main.cpp) ---
    // Que mazmorras ya se ganaron esta run (se resetea a todo false al
    // empezar una run nueva o al perder del todo).
    bool mazmorraSuperada[kNumMazmorrasMapa] = { false, false, false };
    // True si la partida se guardo parada en el mapa (sin ninguna mazmorra
    // en curso) en vez de a mitad de una exploracion — decide si Cargar
    // debe volver a EstadoJuego::Mapa o a EstadoJuego::Exploracion.
    bool enMapa = true;
    // Indice (0..kNumMazmorrasMapa-1) de la mazmorra en curso dentro de
    // 'habitaciones'/'enemigos'/etc — solo valido si 'enMapa' es false; se
    // necesita para poder marcar el slot correcto de 'mazmorraSuperada' al
    // derrotar a su jefe despues de cargar esta partida.
    int mazmorraActivaIndice = -1;
};

// True si hay un archivo de guardado en el slot pedido — la pantalla de
// seleccion de slot (ver render/menu_ui.h) lo usa para mostrar cada slot
// como "ocupado" o "vacio", y el menu de inicio para decidir si "Cargar"
// esta habilitada (con que UN slot tenga partida alcanza).
bool HayPartidaGuardada(int slot);

// Escribe el estado actual en el archivo de 'slot', pisando lo que hubiera
// antes en ESE slot nomas (los otros dos quedan intactos). Devuelve false
// si no se pudo abrir el archivo para escritura (por ejemplo, sin permisos
// en esa carpeta) — en ese caso no se modifica el archivo anterior, si
// existia.
// 'mazmorraSuperada'/'enMapa'/'mazmorraActivaIndice': ver los mismos campos
// en DatosPartida arriba — mazmorraActivaIndice se ignora (pero igual se
// escribe) si enMapa es true.
bool GuardarPartida(int slot, const Dungeon& mazmorra, const Party& party,
                     const std::vector<Enemy>& enemigos, const std::vector<Cofre>& cofres,
                     const bool mazmorraSuperada[kNumMazmorrasMapa], bool enMapa, int mazmorraActivaIndice);

// Resultado de CargarPartida: 'valido' en false si no habia archivo de
// guardado en ese slot, o si estaba corrupto/incompleto (truncado, de un
// formato viejo, editado a mano de forma invalida) — en ese caso 'datos'
// queda vacio y no se debe usar.
struct ResultadoCarga {
    bool valido = false;
    DatosPartida datos;
};
ResultadoCarga CargarPartida(int slot);

}  // namespace game
