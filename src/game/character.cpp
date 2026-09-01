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

} // namespace game
