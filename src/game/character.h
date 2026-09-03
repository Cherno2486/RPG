#pragma once
#include <string>
#include "mathtypes.h"
#include "combat_state.h"
#include "item_types.h"

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

// Lo que hay puesto (o no) en una ranura de equipo — ver Character::Arma(),
// Character::Accesorio() y Character::Equipar().
struct ItemEquipado {
    bool ocupado = false;
    Item item;  // valido solo si ocupado
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

    // --- Equipo (mejoras permanentes de item.h: Piedra de Fuerza, Amuleto
    // de Proteccion) ---
    // Cada personaje tiene UNA sola ranura de Arma y UNA de Accesorio (ver
    // RanuraEquipo) — asi no tiene sentido, por ejemplo, ponerle 5 Piedras
    // de Fuerza al mismo personaje: la sexta vez que equipa un arma,
    // reemplaza a la anterior en vez de sumarse.
    const ItemEquipado& Arma() const { return arma_; }
    const ItemEquipado& Accesorio() const { return accesorio_; }

    // Equipa 'nuevo' en la ranura que le corresponde segun nuevo.ranura
    // (debe ser Arma o Accesorio — no llamar con Ninguna). Si esa ranura ya
    // tenia algo puesto, le revierte el bono a las stats antes de aplicar
    // el nuevo, y lo devuelve (ocupado=true) para que el llamador lo pueda
    // devolver al inventario compartido en vez de perderlo (ver
    // Inventory::Equipar). Devuelve ocupado=false si la ranura estaba vacia.
    ItemEquipado Equipar(Item nuevo);

    // Fija el equipo directamente, SIN tocar stats_ (a diferencia de
    // Equipar, que aplica/revierte bonos). La usa el sistema de guardado
    // (game/save.h): stats_ ya se restaura con cualquier bono de equipo
    // incluido, asi que volver a aplicarlo via Equipar lo sumaria dos
    // veces.
    void CargarEquipoGuardado(ItemEquipado arma, ItemEquipado accesorio);

private:
    std::string nombre_;
    Role rol_;
    Stats stats_;
    Vec2 posicion_;
    EstadoCombate combate_;
    ItemEquipado arma_;
    ItemEquipado accesorio_;
    static constexpr float kRadioColision = 14.0f;
};

} // namespace game
