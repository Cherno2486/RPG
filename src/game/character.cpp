#include "character.h"
#include <algorithm>

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

const char* NombreRecurso(Role rol) {
    return UsaConcentracion(rol) ? "Concentración" : "Resistencia";
}

bool UsaConcentracion(Role rol) {
    return rol == Role::Soporte || rol == Role::Control;
}

int ExperienciaParaNivel(int nivelActual) {
    if (nivelActual >= kNivelMaximo) return 0;
    return 20 + (nivelActual - 1) * 12;
}

namespace {
// Crecimiento de stats por nivel, uno por rol — aplicado una vez por cada
// nivel ganado (ver Character::AplicarCrecimientoDeNivel). HP y recurso
// maximo suben de a poco en los 4 roles; ataque/defensa reflejan la
// identidad de cada uno: Tanque gana defensa (nunca ataque), Danio y
// Control ganan ataque (los dos roles con foco ofensivo), Soporte no gana
// ninguno de los dos — su fuerte al subir de nivel es el recurso, para
// poder curar o aplicar su habilidad mas seguido. Velocidad NO crece con
// el nivel a proposito, mismo criterio que ya usa el escalado de
// dificultad de enemigos: no alterar el orden de turno de forma menos
// predecible.
struct CrecimientoPorNivel { int hpMax; int recursoMax; int ataque; int defensa; };

CrecimientoPorNivel CrecimientoDe(Role rol) {
    switch (rol) {
        case Role::Tanque:  return CrecimientoPorNivel{5, 1, 0, 1};
        case Role::Danio:   return CrecimientoPorNivel{3, 1, 1, 0};
        case Role::Soporte: return CrecimientoPorNivel{3, 2, 0, 0};
        case Role::Control: return CrecimientoPorNivel{3, 2, 1, 0};
    }
    return CrecimientoPorNivel{3, 1, 0, 0};
}

// Porcion del recurso MAXIMO que se recupera al ganar un combate (ver
// Character::RegenerarRecursoPorVictoria) — pensado para que, en la
// practica, ronde el costo de una habilidad de rol mas (Golpe Provocador y
// Golpe Certero cuestan 5, Grito Debilitante 6, Curar 8, ver
// EjecutarHabilidadDeRol en combat.cpp) sin dejar el recurso lleno de
// nuevo victoria tras victoria — sigue habiendo que administrarlo durante
// una mazmorra larga, solo que ya no queda seco para siempre despues de la
// primera pelea.
constexpr float kPorcentajeRegenRecursoPorVictoria = 0.3f;
}  // namespace

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
    int danoReal = AplicarDano(stats_, combate_, cantidad);
    // La Concentracion se rompe/reduce al recibir dano (a diferencia de la
    // Resistencia fisica, que solo se gasta al usar la habilidad de rol) —
    // se le resta el mismo dano que efectivamente llego a la vida (ya
    // descontado el Escudo), sin bajar de 0. Solo afecta a Soporte/Control:
    // Tanque y Danio no usan Concentracion (ver NombreRecurso/UsaConcentracion).
    if (danoReal > 0 && UsaConcentracion(rol_)) {
        stats_.recurso -= std::min(stats_.recurso, danoReal);
    }
    return danoReal;
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
    // acumular stats de items que ya no estan puestos. MejorarVidaMaxima es
    // el unico caso que toca dos campos: baja hpMax y, si hp quedo por
    // encima del nuevo maximo (nada la bajo mientras tanto), lo recorta —
    // sin restarle a hp el bono a ciegas, porque pudo haber cambiado por
    // combate desde que se equipo.
    if (anterior.ocupado) {
        if (anterior.item.efecto == EfectoItem::MejorarAtaque) stats_.ataque -= anterior.item.bono;
        else if (anterior.item.efecto == EfectoItem::MejorarDefensa) stats_.defensa -= anterior.item.bono;
        else if (anterior.item.efecto == EfectoItem::MejorarVelocidad) stats_.velocidad -= anterior.item.bono;
        else if (anterior.item.efecto == EfectoItem::MejorarVidaMaxima) {
            stats_.hpMax -= anterior.item.bono;
            if (stats_.hpMax < 0) stats_.hpMax = 0;
            if (stats_.hp > stats_.hpMax) stats_.hp = stats_.hpMax;
        }
    }

    // Al aplicar MejorarVidaMaxima, la vida actual sube junto con el maximo
    // (se siente como una mejora real al toque, no solo una barra mas
    // grande para rellenar despues).
    if (nuevo.efecto == EfectoItem::MejorarAtaque) stats_.ataque += nuevo.bono;
    else if (nuevo.efecto == EfectoItem::MejorarDefensa) stats_.defensa += nuevo.bono;
    else if (nuevo.efecto == EfectoItem::MejorarVelocidad) stats_.velocidad += nuevo.bono;
    else if (nuevo.efecto == EfectoItem::MejorarVidaMaxima) {
        stats_.hpMax += nuevo.bono;
        stats_.hp += nuevo.bono;
        if (stats_.hp > stats_.hpMax) stats_.hp = stats_.hpMax;
    }

    *ranura = ItemEquipado{true, std::move(nuevo)};
    return anterior;
}

void Character::CargarEquipoGuardado(ItemEquipado arma, ItemEquipado accesorio) {
    arma_ = std::move(arma);
    accesorio_ = std::move(accesorio);
}

void Character::AplicarCrecimientoDeNivel() {
    CrecimientoPorNivel c = CrecimientoDe(rol_);
    // HP y recurso actuales suben junto con sus maximos (mismo criterio
    // que MejorarVidaMaxima al equipar una mejora) — subir de nivel se
    // siente como una mejora real al toque, no solo una barra mas grande
    // para rellenar despues.
    stats_.hpMax += c.hpMax;
    stats_.hp += c.hpMax;
    if (stats_.hp > stats_.hpMax) stats_.hp = stats_.hpMax;
    stats_.recursoMax += c.recursoMax;
    stats_.recurso += c.recursoMax;
    if (stats_.recurso > stats_.recursoMax) stats_.recurso = stats_.recursoMax;
    stats_.ataque += c.ataque;
    stats_.defensa += c.defensa;
}

void Character::GanarExperiencia(int cantidad) {
    if (cantidad <= 0 || nivel_ >= kNivelMaximo) return;
    xp_ += cantidad;
    int umbral = ExperienciaParaNivel(nivel_);
    while (umbral > 0 && xp_ >= umbral) {
        xp_ -= umbral;
        ++nivel_;
        AplicarCrecimientoDeNivel();
        umbral = ExperienciaParaNivel(nivel_);
    }
    // Ya en el tope: no hay proximo nivel al que aportar, no tiene sentido
    // dejar experiencia "de sobra" acumulada sin efecto.
    if (nivel_ >= kNivelMaximo) xp_ = 0;
}

int Character::ExperienciaAcumuladaTotal() const {
    int total = xp_;
    for (int n = 1; n < nivel_; ++n) total += ExperienciaParaNivel(n);
    return total;
}

void Character::CargarNivelGuardado(int nivel, int experiencia) {
    nivel_ = nivel;
    xp_ = experiencia;
}

void Character::RegenerarRecursoPorVictoria() {
    if (stats_.recursoMax <= 0) return;
    int regen = static_cast<int>(stats_.recursoMax * kPorcentajeRegenRecursoPorVictoria + 0.5f);
    stats_.recurso = std::min(stats_.recursoMax, stats_.recurso + regen);
}

} // namespace game
