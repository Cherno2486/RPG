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

// Nombre del recurso de habilidad segun el rol — ya no hay un "mana"
// generico: Tanque y Danio (roles fisicos) gastan Resistencia al pelear
// cuerpo a cuerpo (el cansancio de un espadazo o lanzazo), y Soporte y
// Control (los magos del party) gastan Concentracion al castear, que ademas
// se les rompe/reduce al recibir dano en combate (ver
// combat.cpp::ResolverAtaque y Character::RecibirDano) — es mas fragil que
// la Resistencia fisica, en linea con "necesitan concentracion para
// castear el hechizo".
const char* NombreRecurso(Role rol);

// True si 'rol' usa Concentracion (Soporte/Control) en vez de Resistencia
// (Tanque/Danio) — ver NombreRecurso. Centraliza el chequeo para no repetir
// "rol == Soporte || rol == Control" en cada lugar que necesita distinguir
// el comportamiento (ahora mismo, solo Character::RecibirDano).
bool UsaConcentracion(Role rol);

// Sistema de niveles: progreso PERMANENTE del jugador, no de una run en
// curso — sobrevive tanto a un Game Over como a elegir "Nueva partida" a
// proposito (a diferencia del equipo, el inventario y el progreso en el
// mapa de mazmorras, que si se resetean del todo en ambos casos; ver
// "Sistema de niveles" en docs/design.md y CrearPartyConProgreso en
// main.cpp). Cada uno de los 4 personajes tiene su propio nivel/XP
// independiente (no uno solo compartido por todo el party).
//
// Nivel maximo alcanzable — por encima de este tope, GanarExperiencia no
// hace nada mas (la experiencia de sobra no se acumula sin efecto).
constexpr int kNivelMaximo = 10;

// Experiencia necesaria para pasar de 'nivelActual' a 'nivelActual+1' (ver
// Character::GanarExperiencia). Devuelve 0 si 'nivelActual' ya esta en
// kNivelMaximo o mas — no hay siguiente nivel al que subir. Curva simple,
// creciente de a 12 por nivel (20/32/44/.../116, sumando 612 en total para
// llegar de nivel 1 a 10) — pensada para que subir de nivel se sienta
// varias veces por mazmorra sin volverse instantaneo, y llegar al tope
// tome unas cuantas mazmorras completas, no una sola. Ver "Sistema de
// niveles" en docs/design.md para el detalle de balance.
int ExperienciaParaNivel(int nivelActual);

struct Stats {
    int hpMax = 10;
    int hp = 10;
    int recursoMax = 0;        // Resistencia o Concentracion, segun el rol — ver NombreRecurso(Role)
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
    // Aplica dano (consumiendo escudo si tiene) y devuelve el dano real a la
    // vida. Si el rol usa Concentracion (Soporte/Control — ver
    // UsaConcentracion), ademas le rompe/reduce esa cantidad de recurso: a
    // un mago le cuesta mantener el hechizo mientras lo golpean. Es el unico
    // punto de entrada para dano que pasa por esta clase (los golpes de
    // combate via Combatiente/AplicarDano en combat.cpp aplican la misma
    // regla por separado, ver ResolverAtaque — este cubre el resto, como el
    // dano de veneno por turno).
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

    // --- Nivel y experiencia (ver el comentario junto a kNivelMaximo mas
    // arriba) ---
    int Nivel() const { return nivel_; }
    // Experiencia acumulada DENTRO del nivel actual (no total desde el
    // nivel 1) — p.ej. en nivel 3 con 12/44 de progreso, esto devuelve 12.
    // Se resetea (con el sobrante trasladado) cada vez que se sube de
    // nivel, ver GanarExperiencia. Para el total acumulado desde el nivel
    // 1, ver ExperienciaAcumuladaTotal.
    int Experiencia() const { return xp_; }

    // Otorga 'cantidad' de experiencia y sube de nivel las veces que
    // corresponda (una sola llamada puede subir mas de un nivel si la
    // cantidad alcanza), aplicando en cada subida el crecimiento de stats
    // propio del rol de este personaje (ver character.cpp) — HP y el
    // recurso actuales suben junto con sus maximos, mismo criterio que
    // MejorarVidaMaxima al equipar una mejora. No hace nada si 'cantidad'
    // no es positiva o si ya esta en kNivelMaximo.
    void GanarExperiencia(int cantidad);

    // Experiencia TOTAL acumulada desde el nivel 1 (a diferencia de
    // Experiencia(), que es solo el progreso del nivel actual) — suma los
    // umbrales de todos los niveles ya superados mas el progreso en
    // curso. La usa main.cpp para "reconstruir" un personaje nuevo (stats
    // de nivel 1, ver CrearPartyDeEjemplo) hasta este mismo nivel via
    // GanarExperiencia(ExperienciaAcumuladaTotal()) — asi el nivel
    // sobrevive a un reset de equipo/inventario sin duplicar la logica de
    // crecimiento (ver CrearPartyConProgreso en main.cpp).
    int ExperienciaAcumuladaTotal() const;

    // Fija nivel/experiencia directo, SIN aplicar ningun crecimiento de
    // stats — lo usa el sistema de guardado (game/save.h): stats_ ya se
    // restaura con cualquier crecimiento de nivel incluido, asi que
    // volver a aplicarlo via GanarExperiencia lo sumaria de nuevo. Mismo
    // patron que CargarEquipoGuardado.
    void CargarNivelGuardado(int nivel, int experiencia);

    // Recupera una porcion del recurso (Resistencia/Concentracion) MAXIMO al
    // ganar un combate (ver kPorcentajeRegenRecursoPorVictoria en
    // character.cpp y "Regeneracion de recurso" en docs/design.md) —
    // clampeado, nunca pasa el maximo. Sin esto, el recurso solo se
    // recuperaba con items o al revivir tras un Game Over, asi que quedaba
    // seco para el resto de la run apenas se gastaba. No hace nada si el
    // rol no usa recurso (recursoMax == 0, no debería pasar en la practica).
    void RegenerarRecursoPorVictoria();

    // Cooldown de dano de trampa de piso (ver game::Trampa en dungeon.h) --
    // mismo mecanismo que Enemy::CooldownTrampa, ver ese comentario. Solo lo
    // usa el lider del party durante la exploracion (es el unico personaje
    // con colision propia contra el mundo, ver Party::ActualizarFormacion).
    float CooldownTrampa() const { return cooldownTrampa_; }
    void ActualizarCooldownTrampa(float deltaSeconds) {
        if (cooldownTrampa_ > 0.0f) cooldownTrampa_ -= deltaSeconds;
    }
    void ReiniciarCooldownTrampa(float duracion) { cooldownTrampa_ = duracion; }

private:
    std::string nombre_;
    Role rol_;
    Stats stats_;
    Vec2 posicion_;
    EstadoCombate combate_;
    ItemEquipado arma_;
    ItemEquipado accesorio_;
    int nivel_ = 1;
    int xp_ = 0;
    void AplicarCrecimientoDeNivel();
    float cooldownTrampa_ = 0.0f;
    static constexpr float kRadioColision = 14.0f;
};

} // namespace game
