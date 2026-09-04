#include "inventory.h"
#include <cstdio>

namespace game {

void Inventory::Agregar(Item item, int cantidad) {
    for (auto& pila : pilas_) {
        if (pila.item.nombre == item.nombre) {
            pila.cantidad += cantidad;
            return;
        }
    }
    pilas_.push_back(PilaItem{std::move(item), cantidad});
}

ResultadoUsoItem Inventory::Usar(size_t indice, Character& objetivo) {
    if (indice >= pilas_.size()) return ResultadoUsoItem{};

    ResultadoUsoItem r = UsarItem(pilas_[indice].item, objetivo);
    if (r.exitoso) {
        pilas_[indice].cantidad -= 1;
        if (pilas_[indice].cantidad <= 0) {
            pilas_.erase(pilas_.begin() + (long)indice);
        }
    }
    return r;
}

void Inventory::ConsumirUnidad(size_t indice) {
    if (indice >= pilas_.size()) return;
    pilas_[indice].cantidad -= 1;
    if (pilas_[indice].cantidad <= 0) {
        pilas_.erase(pilas_.begin() + (long)indice);
    }
}

ResultadoEquipar Inventory::Equipar(size_t indice, Character& personaje) {
    if (indice >= pilas_.size()) return ResultadoEquipar{};
    if (pilas_[indice].item.tipo != TipoItem::Mejora) return ResultadoEquipar{};

    Item item = pilas_[indice].item;  // copia: la pila se puede borrar antes de usarla

    // Se saca del stack ANTES de equipar (y de potencialmente devolver el
    // item reemplazado al inventario): si son el mismo item, evita mezclar
    // "la unidad que se esta equipando" con "la que vuelve" en una sola
    // pila con la cantidad mal contada.
    pilas_[indice].cantidad -= 1;
    if (pilas_[indice].cantidad <= 0) {
        pilas_.erase(pilas_.begin() + (long)indice);
    }

    ItemEquipado anterior = personaje.Equipar(item);

    char buffer[192];
    if (anterior.ocupado) {
        Agregar(anterior.item);
        std::snprintf(buffer, sizeof(buffer), "%s equipa %s (vuelve al inventario: %s).",
                      personaje.Nombre().c_str(), item.nombre.c_str(), anterior.item.nombre.c_str());
    } else {
        std::snprintf(buffer, sizeof(buffer), "%s equipa %s.",
                      personaje.Nombre().c_str(), item.nombre.c_str());
    }

    ResultadoEquipar r;
    r.exitoso = true;
    r.texto = buffer;
    return r;
}

} // namespace game
