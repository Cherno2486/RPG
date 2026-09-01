#pragma once
#include <vector>
#include <deque>
#include "character.h"

namespace game {

class Party {
public:
    explicit Party(std::vector<Character> miembros);

    // El primer miembro es el lider (el que controla el jugador directamente).
    Character& Lider() { return miembros_[0]; }
    const Character& Lider() const { return miembros_[0]; }

    std::vector<Character>& Miembros() { return miembros_; }
    const std::vector<Character>& Miembros() const { return miembros_; }

    // Actualiza la formacion "tren": cada seguidor va hacia un punto del rastro
    // de posiciones del lider, espaciado por kEspaciado (estilo conga/tren),
    // para que los tres se vean en pantalla durante la exploracion.
    void ActualizarFormacion(float deltaSeconds);

    // Teletransporta a todo el party a 'posicion' y borra el rastro de
    // formacion. Sin esto, un teletransporte (por ejemplo al revivir tras
    // una derrota) dejaria a los seguidores "corriendo" desde el rastro
    // viejo en vez de aparecer ya formados en el punto nuevo.
    void ReiniciarFormacion(Vec2 posicion);

private:
    std::vector<Character> miembros_;
    std::deque<Vec2> historialLider_;
    static constexpr float kEspaciado = 34.0f;
    static constexpr int kMaxHistorial = 600;
};

} // namespace game
