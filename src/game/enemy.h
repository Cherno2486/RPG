#pragma once
#include <string>
#include "mathtypes.h"
#include "character.h"       // reutiliza Stats
#include "combat_state.h"

namespace game {

// Variedad de enemigos: por ahora solo cambia el set de stats y, para
// Bandido y el Capitan, la IA en combate (ver CombatEncounter::Actualizar en
// combat.cpp, que resuelve sus golpes especiales). Pensado para crecer
// cuando haya generacion real de encuentros con mas de un enemigo a la vez.
enum class TipoEnemigo {
    EsqueletoErrante,  // el original: stats parejas, solo ataque basico.
    RataGigante,       // rapida y fragil, ataque basico nomas.
    BanditoAturdidor,  // mas dura; a veces en vez de atacar usa un golpe
                        // que aplica Aturdido (hace perder el turno).
    CapitanBandido,    // jefe de la mazmorra: unico, en la ultima sala. Mas
                        // stats que cualquier otro enemigo, alterna entre
                        // ataque basico, Golpe Aturdidor y "Doble Tajo" (dos
                        // golpes en un mismo turno), y entra en furia por
                        // debajo del 40% de HP (siempre Doble Tajo).
};

// True si los enemigos de este tipo persiguen al lider durante la
// exploracion y fuerzan el combate al alcanzarlo (ver el chequeo de
// persecucion en main.cpp), en vez de quedarse quietos hasta que el
// jugador confirma con [E] como el resto. Solo el Bandido Aturdidor es
// agresivo por ahora — pedido explicito del usuario: "que persigan y
// peleen, pero no todos los enemigos". El Capitan Bandido queda afuera a
// proposito (es un jefe unico al fondo de la mazmorra: perseguir no le
// suma nada, el jugador ya tiene que enfrentarlo si o si para terminar la
// run) y los otros dos tipos comunes se quedan con el comportamiento
// pasivo original.
bool EsAgresivo(TipoEnemigo tipo);

// Enemigo simple: mismos stats base que un personaje, pero sin rol ni
// habilidades propias del party (su "habilidad", si tiene una especial
// segun el tipo, la resuelve la capa de combate).
class Enemy {
public:
    // 'salaIndice' identifica a que sala de la mazmorra pertenece este
    // enemigo (ver game::Dungeon), para poder agrupar a todos los de una
    // misma sala en un solo encuentro de combate al engancharlos. -1 (el
    // default) para enemigos de prueba fuera de una mazmorra real.
    Enemy(std::string nombre, TipoEnemigo tipo, Stats stats, Vec2 posicionInicial, int salaIndice = -1);

    const std::string& Nombre() const { return nombre_; }
    TipoEnemigo Tipo() const { return tipo_; }
    const Stats& GetStats() const { return stats_; }
    Stats& GetStatsMut() { return stats_; }
    int Sala() const { return sala_; }

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

    // Cooldown de dano de trampa de piso (ver game::Trampa en dungeon.h):
    // mientras sea mayor a 0, este enemigo no puede recibir otro tick de
    // dano de trampa, aunque siga parado/persiguiendo sobre una. Estado
    // puramente de exploracion, sin relacion con el combate -- por eso no
    // se persiste en el guardado (perder el cooldown a mitad de una trampa
    // al cargar una partida es un detalle menor, no cambia el resultado).
    float CooldownTrampa() const { return cooldownTrampa_; }
    void ActualizarCooldownTrampa(float deltaSeconds) {
        if (cooldownTrampa_ > 0.0f) cooldownTrampa_ -= deltaSeconds;
    }
    void ReiniciarCooldownTrampa(float duracion) { cooldownTrampa_ = duracion; }

private:
    std::string nombre_;
    TipoEnemigo tipo_;
    Stats stats_;
    Vec2 posicion_;
    EstadoCombate combate_;
    bool vencido_ = false;
    int sala_ = -1;
    float cooldownTrampa_ = 0.0f;
    static constexpr float kRadioColision = 16.0f;
};

} // namespace game
