#include "enemy.h"

namespace game {

bool EsAgresivo(TipoEnemigo tipo) {
    return tipo == TipoEnemigo::BanditoAturdidor;
}

Enemy::Enemy(std::string nombre, TipoEnemigo tipo, Stats stats, Vec2 posicionInicial, int salaIndice)
    : nombre_(std::move(nombre)), tipo_(tipo), stats_(stats), posicion_(posicionInicial), sala_(salaIndice) {}

Rect Enemy::Colisionador() const {
    return Rect{
        posicion_.x - kRadioColision,
        posicion_.y - kRadioColision,
        kRadioColision * 2.0f,
        kRadioColision * 2.0f
    };
}

int Enemy::RecibirDano(int cantidad) {
    return AplicarDano(stats_, combate_, cantidad);
}

int Enemy::Curar(int cantidad) {
    return AplicarCuracion(stats_, cantidad);
}

} // namespace game
