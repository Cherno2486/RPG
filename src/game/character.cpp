#include "character.h"

namespace game {

const char* RoleName(Role role) {
    switch (role) {
        case Role::Tanque:  return "Tanque";
        case Role::Danio:   return "Daño";
        case Role::Soporte: return "Soporte";
        case Role::Control: return "Control";
    }
    return "?";
}

Character::Character(std::string nombre, Role rol, Stats stats, Vec2 posicionInicial)
    : nombre_(std::move(nombre)), rol_(rol), stats_(stats), posicion_(posicionInicial) {}

Rect Character::Colisionador() const {
    return Rect{
        posicion_.x - kRadioColision,
        posicion_.y - kRadioColision,
        kRadioColision * 2.0f,
        kRadioColision * 2.0f
    };
}

int Character::RecibirDano(int cantidad) {
    return AplicarDano(stats_, combate_, cantidad);
}

int Character::Curar(int cantidad) {
    return AplicarCuracion(stats_, cantidad);
}

void Character::Revivir() {
    stats_.hp = stats_.hpMax;
    stats_.recurso = stats_.recursoMax;
    combate_.LimpiarTodo();
}

ItemEquipado Character::Equipar(Item nuevo) {
    ItemEquipado* ranura = (nuevo.ranura == RanuraEquipo::Arma) ? &arma_ : &accesorio_;
    ItemEquipado anterior = *ranura;

    // Revierte el bono de lo que hubiera antes en esa ranura, para no
    // acumular stats de items que ya no estan puestos.
    if (anterior.ocupado) {
        if (anterior.item.efecto == EfectoItem::MejorarAtaque) stats_.ataque -= anterior.item.bono;
        else if (anterior.item.efecto == EfectoItem::MejorarDefensa) stats_.defensa -= anterior.item.bono;
    }

    if (nuevo.efecto == EfectoItem::MejorarAtaque) stats_.ataque += nuevo.bono;
    else if (nuevo.efecto == EfectoItem::MejorarDefensa) stats_.defensa += nuevo.bono;

    *ranura = ItemEquipado{true, std::move(nuevo)};
    return anterior;
}

} // namespace game
