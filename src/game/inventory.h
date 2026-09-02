#pragma once
#include <string>
#include <vector>
#include "item.h"

namespace game {

struct PilaItem {
    Item item;
    int cantidad = 0;
};

struct ResultadoEquipar {
    bool exitoso = false;
    std::string texto;  // linea lista para mostrar como mensaje/log
};

// Inventario compartido por todo el party (no uno por personaje): los items
// recolectados (de cofres o botin de enemigos derrotados) se apilan por
// nombre. Los Consumibles se usan directo desde aca (Usar); las Mejoras en
// cambio se equipan (Equipar) en una ranura del personaje elegido, ver
// item_types.h y Character::Equipar.
class Inventory {
public:
    void Agregar(Item item, int cantidad = 1);

    const std::vector<PilaItem>& Pilas() const { return pilas_; }
    bool Vacio() const { return pilas_.empty(); }

    // Usa una unidad del item Consumible en Pilas()[indice] sobre 'objetivo':
    // aplica su efecto y resta 1 al stack (lo saca de la lista si llega a
    // 0). No hace nada (resultado no exitoso) si el indice es invalido o si
    // el item en ese indice no es un Consumible (ver Equipar para Mejoras).
    ResultadoUsoItem Usar(size_t indice, Character& objetivo);

    // Equipa una unidad del item Mejora en Pilas()[indice] sobre 'personaje'
    // (en la ranura Arma o Accesorio, segun el item) y resta 1 al stack. Si
    // esa ranura ya tenia algo puesto, el item reemplazado vuelve solo al
    // inventario compartido (no se pierde, ver Character::Equipar). No hace
    // nada si el indice es invalido o si el item no es una Mejora (ver Usar
    // para Consumibles).
    ResultadoEquipar Equipar(size_t indice, Character& personaje);

private:
    std::vector<PilaItem> pilas_;
};

} // namespace game
