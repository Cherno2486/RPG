#pragma once
#include <string>
#include "mathtypes.h"
#include "character.h"       // reutiliza Stats
#include "combat_state.h"

namespace game {

// Enemigo simple: mismos stats base que un personaje, pero sin rol ni
// habilidades propias del party (su "habilidad" es un ataque basico como
// cualquier otro combatiente, resuelto por la capa de combate).
class Enemy {
public:
    Enemy(std::string nombre, Stats stats, Vec2 posicionInicial);

    const std::string& Nombre() const { return nombre_; }
    const Stats& GetStats() const { return stats_; }
    Stats& GetStatsMut() { return stats_; }

    Vec2 Posicion() const { return posicion_; }
    void SetPosicion(Vec2 pos) { posicion_ = pos; }

    Rect Colisionador() const;

    EstadoCombate& Combate() { return combate_; }
    const EstadoCombate& Combate() const { return combate_; }

    bool EstaVivo() const { return stats_.hp > 0; }
    int RecibirDano(int cantidad);
    int Curar(int cantidad);

    // Se pone en true cuando se lo derrota, para que deje de dibujarse y de
    // poder re-engancharse en combate durante la exploracion.
    bool Vencido() const { return vencido_; }
    void MarcarVencido() { vencido_ = true; }

private:
    std::string nombre_;
    Stats stats_;
    Vec2 posicion_;
    EstadoCombate combate_;
    bool vencido_ = false;
    static constexpr float kRadioColision = 16.0f;
};

} // namespace game
