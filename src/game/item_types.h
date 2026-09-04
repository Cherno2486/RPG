#pragma once
#include <string>
#include "effects.h"  // TipoEfecto, para items que aplican/curan un estado (ver EfectoItem)

namespace game {

enum class TipoItem {
    Consumible,  // cura vida/recurso o aplica/cura un estado; se gasta al usarse
    Mejora,      // sube una stat un monto fijo, para siempre; se EQUIPA
                 // (ver RanuraEquipo, Character::Equipar) en vez de "usarse"
                 // directo, asi no se puede apilar la misma mejora sin limite
                 // en un solo personaje.
};

enum class EfectoItem {
    CurarVida,
    CurarRecurso,
    MejorarAtaque,
    MejorarDefensa,
    MejorarVelocidad,    // Mejora de ranura Arma, alternativa a MejorarAtaque
    MejorarVidaMaxima,   // Mejora de ranura Accesorio, alternativa a MejorarDefensa
    AplicarEstado,       // aplica el TipoEfecto de Item::estado sobre el objetivo,
                         // sin tirada de impacto (Bomba de Veneno, Frasco de Escudo)
    CurarEstados,        // quita Aturdido y Veneno del objetivo (Antidoto)
};

// Ranura de equipo donde va un item de tipo Mejora una vez equipado. Cada
// personaje tiene como mucho un item por ranura (ver Character::Equipar,
// que devuelve el que hubiera antes para que vuelva al inventario
// compartido en vez de perderse). Ninguna para los Consumibles, que nunca
// se equipan. Cada ranura tiene dos "sabores" posibles, para que equipar
// algo sea una eleccion real y no un solo camino: Arma sube ataque
// (MejorarAtaque) o velocidad (MejorarVelocidad); Accesorio sube defensa
// (MejorarDefensa) o vida maxima (MejorarVidaMaxima).
enum class RanuraEquipo {
    Ninguna,
    Arma,       // mejoras ofensivas: mas daño o actuar mas seguido
    Accesorio,  // mejoras defensivas: esquivar mas o aguantar mas golpes
};

// Definicion de un item del catalogo (ver game/item.h para las instancias
// concretas — PocionCuracionMenor(), PiedraDeFuerza(), etc). Vive en su
// propio header, separado de item.h, para que game/character.h pueda
// declarar sus ranuras de equipo (ItemEquipado) sin depender de item.h
// (que si depende de character.h, para UsarItem(Character&)).
struct Item {
    std::string nombre;
    std::string descripcion;
    TipoItem tipo = TipoItem::Consumible;
    EfectoItem efecto = EfectoItem::CurarVida;
    RanuraEquipo ranura = RanuraEquipo::Ninguna;
    int dados = 0;
    int caras = 0;
    int bono = 0;

    // Solo usados cuando efecto == AplicarEstado: 'estado' es el TipoEfecto a
    // aplicar (reutiliza 'dados' como duracionTurnos y 'bono' como magnitud
    // del EfectoActivo resultante, mismo criterio que Mejora reutiliza 'bono'
    // para el monto de su stat — asi no hace falta un struct aparte). No se
    // usan con CurarEstados (Antidoto no necesita magnitud/duracion, quita
    // Aturdido y Veneno directo).
    TipoEfecto estado = TipoEfecto::Aturdido;
    // True si este item apunta a un ENEMIGO en vez de a un aliado (caso de
    // Bomba de Veneno) — ver game::CombatEncounter::AccionUsarItem, que lo
    // usa para decidir si aplica el efecto sobre IndiceObjetivo() o sobre el
    // aliado elegido en el sub-menu de "[3] Usar item".
    bool apuntaAEnemigo = false;
};

} // namespace game
