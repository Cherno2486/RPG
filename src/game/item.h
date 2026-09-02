#pragma once
#include <string>
#include "item_types.h"  // Item, TipoItem, EfectoItem, RanuraEquipo
#include "character.h"   // reutiliza Stats/Character
#include "enemy.h"        // TipoEnemigo, para el botin por tipo de enemigo
#include "mathtypes.h"

// Items del prototipo: consumibles que curan (vida o recurso) y "mejoras"
// que suben una stat de forma permanente. Los Consumibles se GASTAN al
// usarse (ver UsarItem, Inventory::Usar); las Mejoras en cambio se EQUIPAN
// en una ranura del personaje (ver item_types.h: RanuraEquipo,
// Character::Equipar, Inventory::Equipar) — no se "usan" instantaneo, para
// que tenga un limite natural (una ranura de Arma y una de Accesorio por
// personaje) en vez de poder acumularse sin tope en el mismo personaje.

namespace game {

// Catalogo fijo de items del prototipo.
Item PocionCuracionMenor();     // Consumible: cura 2d6+2 de vida
Item ElixirDeEnergia();         // Consumible: restaura 1d6+4 de recurso
Item PiedraDeFuerza();          // Mejora (ranura Arma): +1 de ataque permanente
Item AmuletoDeProteccion();     // Mejora (ranura Accesorio): +1 de defensa permanente
Item DagaVeloz();               // Mejora (ranura Arma): +10 de velocidad permanente
Item TalismanDeVitalidad();     // Mejora (ranura Accesorio): +5 de vida maxima permanente

// Una Mejora al azar entre las 4 del catalogo (una de cada ranura x sabor),
// para no repetir la misma logica de sorteo en ItemAleatorioDeCofre y
// TirarLootDeEnemigo.
Item MejoraAleatoria();

// Item al azar para el contenido de un cofre (pensado con mas chance de
// consumibles que de mejoras, para que las mejoras se sientan especiales).
Item ItemAleatorioDeCofre();

struct ResultadoUsoItem {
    bool exitoso = false;
    int valor = 0;       // hp/recurso curado
    std::string texto;   // linea lista para mostrar como mensaje/log
};

// Aplica el efecto instantaneo de un item Consumible sobre 'objetivo' (cura
// vida o recurso). No saca el item del inventario — eso lo maneja
// Inventory::Usar. Devuelve exitoso=false sin hacer nada si 'item' no es de
// tipo Consumible (una Mejora se equipa con Character::Equipar /
// Inventory::Equipar, no se "usa" con esta funcion).
ResultadoUsoItem UsarItem(const Item& item, Character& objetivo);

// Botin que puede soltar un enemigo derrotado, segun su tipo. 'hay' en
// false significa que esta vez no solto nada (no todos los enemigos
// derrotados dejan item).
struct ResultadoLoot {
    bool hay = false;
    Item item;
};
ResultadoLoot TirarLootDeEnemigo(TipoEnemigo tipo);

// Un cofre fijo en la mazmorra: contenido decidido al generarla, se abre
// una sola vez (marcado por 'abierto') y agrega su contenido al inventario
// que se le pase (ver main.cpp).
struct Cofre {
    Vec2 posicion;
    Item contenido;
    bool abierto = false;
};

} // namespace game
