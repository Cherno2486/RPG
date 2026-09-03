#pragma once
#include <vector>
#include "dungeon.h"
#include "party.h"
#include "enemy.h"
#include "item.h"

// Guardado de partida: persiste el estado completo de una run en curso a un
// archivo de texto plano (sin depender de ninguna libreria externa de
// serializacion, en linea con el resto del proyecto), para poder cerrar el
// juego y retomar despues con "Continuar" en el menu de inicio.
//
// Que se guarda: la mazmorra ya generada (salas + paredes ya resueltas, no
// la "receta" para regenerarla — ver el comentario de Dungeon::Dungeon con
// datos ya calculados en dungeon.h), los 4 miembros del party (stats
// actuales, posicion, equipo), el inventario compartido, los enemigos
// (posicion, stats, sala, si ya fueron derrotados) y los cofres (posicion,
// contenido, si ya se abrieron).
//
// Que NO se guarda: nada de un combate en curso — solo se puede guardar
// durante la exploracion (ver la tecla F5 en main.cpp), asi que nunca hay
// un CombatEncounter activo en ese momento. Es un unico slot de guardado
// (el prototipo no tiene UI para elegir entre varios) — GuardarPartida
// siempre pisa el archivo anterior.
namespace game {

// Datos ya reconstruidos por CargarPartida, listos para que main.cpp arme
// la mazmorra/party/enemigos/cofres de la partida en curso.
struct DatosPartida {
    std::vector<Habitacion> habitaciones;
    std::vector<Rect> paredes;
    std::vector<Character> miembros;  // mismo orden que CrearPartyDeEjemplo en main.cpp
    std::vector<PilaItem> pilasInventario;
    std::vector<Enemy> enemigos;
    std::vector<Cofre> cofres;
};

// Ruta fija del archivo de guardado, relativa al directorio desde donde se
// corre el ejecutable — el mismo que ya usan los assets (ver README, "se
// corre con `.\build\rpg_mazmorras.exe` desde la raiz del proyecto").
constexpr const char* kRutaGuardado = "savegame.txt";

// True si hay un archivo de guardado en kRutaGuardado — el menu de inicio
// lo usa para decidir si mostrar la opcion "Continuar" (ver render/menu_ui.h).
bool HayPartidaGuardada();

// Escribe el estado actual a kRutaGuardado, pisando lo que hubiera antes.
// Devuelve false si no se pudo abrir el archivo para escritura (por
// ejemplo, sin permisos en esa carpeta) — en ese caso no se modifica el
// archivo anterior, si existia.
bool GuardarPartida(const Dungeon& mazmorra, const Party& party,
                     const std::vector<Enemy>& enemigos, const std::vector<Cofre>& cofres);

// Resultado de CargarPartida: 'valido' en false si no habia archivo de
// guardado, o si estaba corrupto/incompleto (truncado, de un formato viejo,
// editado a mano de forma invalida) — en ese caso 'datos' queda vacio y no
// se debe usar.
struct ResultadoCarga {
    bool valido = false;
    DatosPartida datos;
};
ResultadoCarga CargarPartida();

}  // namespace game
