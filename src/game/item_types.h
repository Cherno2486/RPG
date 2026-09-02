#pragma once
#include <string>

namespace game {

enum class TipoItem {
    Consumible,  // cura vida o recurso tirando dados; se gasta al usarse
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
};

// Ranura de equipo donde va un item de tipo Mejora una vez equipado. Cada
// personaje tiene como mucho un item por ranura (ver Character::Equipar,
// que devuelve el que hubiera antes para que vuelva al inventario
// compartido en vez de perderse). Ninguna para los Consumibles, que nunca
// se equipan.
enum class RanuraEquipo {
    Ninguna,
    Arma,       // mejoras que suben ataque
    Accesorio,  // mejoras que suben defensa
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
};

} // namespace game
