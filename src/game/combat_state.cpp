#include "combat_state.h"
#include "character.h"  // definicion completa de Stats
#include <algorithm>

namespace game {

void EstadoCombate::AgregarEfecto(EfectoActivo efecto) {
    // Si ya tiene un efecto del mismo tipo, se refresca (se queda con la
    // mayor duracion/magnitud) en vez de acumular entradas duplicadas.
    for (auto& existente : efectos_) {
        if (existente.tipo == efecto.tipo) {
            existente.duracionTurnos = std::max(existente.duracionTurnos, efecto.duracionTurnos);
            existente.magnitud = std::max(existente.magnitud, efecto.magnitud);
            return;
        }
    }
    efectos_.push_back(efecto);
}

bool EstadoCombate::TieneEfecto(TipoEfecto tipo) const {
    for (const auto& e : efectos_) {
        if (e.tipo == tipo) return true;
    }
    return false;
}

int EstadoCombate::MagnitudEfecto(TipoEfecto tipo) const {
    for (const auto& e : efectos_) {
        if (e.tipo == tipo) return e.magnitud;
    }
    return 0;
}

void EstadoCombate::QuitarEfecto(TipoEfecto tipo) {
    efectos_.erase(
        std::remove_if(efectos_.begin(), efectos_.end(),
            [tipo](const EfectoActivo& e) { return e.tipo == tipo; }),
        efectos_.end());
}

void EstadoCombate::LimpiarTodo() {
    efectos_.clear();
}

EstadoCombate::ResultadoTick EstadoCombate::TickInicioDeTurno() {
    ResultadoTick resultado;

    for (const auto& efecto : efectos_) {
        if (efecto.tipo == TipoEfecto::Aturdido) resultado.aturdido = true;
        if (efecto.tipo == TipoEfecto::Veneno) resultado.danoVeneno += efecto.magnitud;
    }

    // Descontar duracion y sacar los que expiraron.
    for (auto& efecto : efectos_) {
        efecto.duracionTurnos -= 1;
    }
    efectos_.erase(
        std::remove_if(efectos_.begin(), efectos_.end(),
            [](const EfectoActivo& e) { return e.duracionTurnos <= 0; }),
        efectos_.end());

    return resultado;
}

int AplicarDano(Stats& stats, EstadoCombate& estado, int dano) {
    if (dano <= 0) return 0;

    int danoRestante = dano;
    if (estado.TieneEfecto(TipoEfecto::Escudo)) {
        int escudo = estado.MagnitudEfecto(TipoEfecto::Escudo);
        int absorbido = std::min(escudo, danoRestante);
        danoRestante -= absorbido;
        int escudoRestante = escudo - absorbido;
        estado.QuitarEfecto(TipoEfecto::Escudo);
        if (escudoRestante > 0) {
            estado.AgregarEfecto(EfectoActivo{TipoEfecto::Escudo, 99, escudoRestante});
        }
    }

    int danoALaVida = std::min(danoRestante, stats.hp);
    stats.hp -= danoALaVida;
    if (stats.hp < 0) stats.hp = 0;
    return danoALaVida;
}

int AplicarCuracion(Stats& stats, int cantidad) {
    if (cantidad <= 0) return 0;
    int curacionReal = std::min(cantidad, stats.hpMax - stats.hp);
    if (curacionReal < 0) curacionReal = 0;
    stats.hp += curacionReal;
    return curacionReal;
}

} // namespace game
