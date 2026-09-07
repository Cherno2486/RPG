#include "enemy.h"

namespace game {

bool EsAgresivo(TipoEnemigo tipo) {
    return RolLocalDeEnemigo(tipo) == 0;
}

bool EsJefe(TipoEnemigo tipo) {
    return RolLocalDeEnemigo(tipo) == 2;
}

int XpPorEnemigo(TipoEnemigo tipo) {
    if (EsJefe(tipo)) return 50;
    return RolLocalDeEnemigo(tipo) == 1 ? 10 : 8;
}

AtaqueEspecialEnemigo AtaqueEspecialDe(TipoEnemigo tipo) {
    switch (tipo) {
        case TipoEnemigo::AranaGigante:
            return AtaqueEspecialEnemigo{"Mordida Venenosa", TipoEfecto::Veneno, /*duracion*/ 3, /*magnitud*/ 3};
        case TipoEnemigo::GuardiaCorrupto:
            return AtaqueEspecialEnemigo{"Golpe con Grillete", TipoEfecto::Aturdido, /*duracion*/ 1, /*magnitud*/ 0};
        case TipoEnemigo::MagoDeLaCorte:
            return AtaqueEspecialEnemigo{"Maleficio Debilitante", TipoEfecto::Debilitado, /*duracion*/ 2, /*magnitud*/ 2};
        default:
            // No deberia consultarse para otro tipo (ver el comentario en
            // enemy.h) -- valor de respaldo inocuo si algun dia se llama mal.
            return AtaqueEspecialEnemigo{"un golpe", TipoEfecto::Aturdido, 0, 0};
    }
}

const char* NombreDobleGolpeDeJefe(TipoEnemigo tipo) {
    switch (tipo) {
        case TipoEnemigo::AlfaDelBosque:        return "Doble Zarpazo";
        case TipoEnemigo::Alcaide:              return "Doble Golpe de Porra";
        case TipoEnemigo::CapitanDeLaGuardia:   return "Doble Estocada";
        default:                                return "Doble Tajo";
    }
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
