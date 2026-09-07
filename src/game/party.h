#pragma once
#include <algorithm>
#include <vector>
#include <deque>
#include "character.h"
#include "inventory.h"

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

    // Inventario compartido por todo el party (items de cofres y botin de
    // enemigos derrotados) — ver game/item.h e game/inventory.h.
    Inventory& Inventario() { return inventario_; }
    const Inventory& Inventario() const { return inventario_; }

    // Oro compartido por todo el party -- moneda para comprar en la Herreria
    // y la Tienda de la Ciudad (ver EstadoJuego::Ciudad/Herreria/Tienda en
    // main.cpp). Progreso DE LA CORRIDA, no permanente: se resetea a 0 con
    // "Nueva partida" y con un Game Over, igual que el inventario y el
    // equipo (a diferencia del nivel/experiencia de cada personaje, que si
    // persiste) -- mismo criterio de "las apuestas de la corrida se
    // pierden" que ya aplicaba antes de que existiera el oro. GanarOro no
    // permite que oro_ quede negativo (sature en 0) por las dudas, aunque en
    // la practica solo se resta desde compras ya validadas contra el oro
    // disponible (ver la logica de compra en main.cpp).
    int Oro() const { return oro_; }
    void GanarOro(int cantidad) { oro_ = std::max(0, oro_ + cantidad); }

private:
    std::vector<Character> miembros_;
    std::deque<Vec2> historialLider_;
    Inventory inventario_;
    int oro_ = 0;
    static constexpr float kEspaciado = 34.0f;
    static constexpr int kMaxHistorial = 600;
};

} // namespace game
