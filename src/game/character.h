#pragma once
#include <string>
#include "mathtypes.h"
#include "combat_state.h"

namespace game {

enum class Role {
    Tanque,
    Danio,      // DPS
    Soporte,
    Control
};

const char* RoleName(Role role);

struct Stats {
    int hpMax = 10;
    int hp = 10;
    int recursoMax = 0;        // energia / mana / stamina, segun el rol
    int recurso = 0;
    int ataque = 1;
    int defensa = 0;
    float velocidad = 100.0f;  // usada para orden de turno en combate y velocidad de movimiento
};

class Character {
public:
    Character(std::string nombre, Role rol, Stats stats, Vec2 posicionInicial);

    const std::string& Nombre() const { return nombre_; }
    Role Rol() const { return rol_; }
    const Stats& GetStats() const { return stats_; }
    Stats& GetStatsMut() { return stats_; }

    Vec2 Posicion() const { return posicion_; }
    void SetPosicion(Vec2 pos) { posicion_ = pos; }

    // Rect de colision centrado en la posicion del personaje.
    Rect Colisionador() const;

    // --- Combate ---
    EstadoCombate& Combate() { return combate_; }
    const EstadoCombate& Combate() const { return combate_; }

    bool EstaVivo() const { return stats_.hp > 0; }
    // Aplica dano (consumiendo escudo si tiene) y devuelve el dano real a la vida.
    int RecibirDano(int cantidad);
    // Cura vida sin pasarse del maximo y devuelve lo curado.
    int Curar(int cantidad);

    // Revive al personaje a HP/recurso completo y limpia sus efectos de
    // combate (Aturdido, Veneno, etc). Pensado para reiniciar el party
    // despues de una derrota, para que un game over no deje al party
    // muerto para siempre (ver CombatEncounter::FaseCombate::Perdido).
    void Revivir();

private:
    std::string nombre_;
    Role rol_;
    Stats stats_;
    Vec2 posicion_;
    EstadoCombate combate_;
    static constexpr float kRadioColision = 14.0f;
};

} // namespace game
