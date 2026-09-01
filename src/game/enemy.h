#pragma once
#include <string>
#include "mathtypes.h"
#include "character.h"       // reutiliza Stats
#include "combat_state.h"

namespace game {

// Variedad de enemigos: por ahora solo cambia el set de stats y, para
// Bandido, la IA en combate (ver CombatEncounter::Actualizar en combat.cpp,
// que a veces usa "Golpe Aturdidor" en vez de un ataque basico). Pensado
// para crecer cuando haya generacion real de encuentros con mas de un
// enemigo a la vez.
enum class TipoEnemigo {
    EsqueletoErrante,  // el original: stats parejas, solo ataque basico.
    RataGigante,       // rapida y fragil, ataque basico nomas.
    BanditoAturdidor,  // mas dura; a veces en vez de atacar usa un golpe
                        // que aplica Aturdido (hace perder el turno).
};

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

private:
    std::string nombre_;
    TipoEnemigo tipo_;
    Stats stats_;
    Vec2 posicion_;
    EstadoCombate combate_;
    bool vencido_ = false;
    int sala_ = -1;
    static constexpr float kRadioColision = 16.0f;
};

} // namespace game
