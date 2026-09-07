#pragma once
#include <string>
#include "mathtypes.h"
#include "character.h"       // reutiliza Stats
#include "combat_state.h"    // EstadoCombate, TipoEfecto (via effects.h)

namespace game {

// Variedad de enemigos: 3 temas (Bosque/Carcel/Castillo, ver game::Tema en
// main.cpp) x 3 roles cada uno (comun agresivo, comun especial, jefe) = 9
// valores en total. A PROPOSITO se numeran en ese orden fijo (los 3 de
// Bosque primero, despues los 3 de Carcel, despues los 3 de Castillo) para
// que el indice entero del enum codifique directamente tema y rol sin
// necesitar una tabla aparte -- ver kEnemigosPorTema/TemaDeEnemigo/
// RolLocalDeEnemigo mas abajo, que es lo que usan EsAgresivo/EsJefe y el
// resto del codigo que necesita saber "a que tema pertenece este tipo" o
// "que rol cumple dentro de su tema".
//
// Rol comun agresivo (indice local 0): stats parejas, solo ataque basico,
// persigue al jugador en la exploracion (ver EsAgresivo). Rol comun
// especial (indice local 1): pasivo (espera [E] como el resto), a veces en
// vez de un ataque basico usa un golpe que aplica el efecto de estado propio
// de su tema (Veneno/Aturdido/Debilitado -- ver AtaqueEspecialDe). Jefe
// (indice local 2): unico, en la ultima sala de su mazmorra, mas stats que
// cualquier otro enemigo de su tema, alterna entre ataque basico, Golpe
// Aturdidor y un "Doble Golpe" con nombre propio por tema (ver
// NombreDobleGolpeDeJefe), y entra en furia por debajo del 40% de HP
// (siempre Doble Golpe) -- mismo patron de IA para los 3 jefes (ver
// combat.cpp), solo cambia el nombre/sprite.
enum class TipoEnemigo {
    // --- Bosque ---
    LoboSalvaje,     // agresivo: persigue y fuerza combate.
    AranaGigante,    // especial: a veces aplica Veneno.
    AlfaDelBosque,   // jefe.
    // --- Carcel ---
    PresoAmotinado,  // agresivo.
    GuardiaCorrupto, // especial: a veces aplica Aturdido.
    Alcaide,         // jefe.
    // --- Castillo ---
    GuardiaReal,     // agresivo.
    MagoDeLaCorte,   // especial: a veces aplica Debilitado.
    CapitanDeLaGuardia, // jefe.
};

// Cuantos TipoEnemigo hay por tema (siempre 1 agresivo + 1 especial + 1
// jefe) -- ver el comentario del enum de arriba sobre por que el orden
// importa.
constexpr int kEnemigosPorTema = 3;

// Indice de tema (0=Bosque, 1=Carcel, 2=Castillo, mismo orden que
// game::Tema en main.cpp) al que pertenece 'tipo'.
inline int TemaDeEnemigo(TipoEnemigo tipo) { return static_cast<int>(tipo) / kEnemigosPorTema; }

// Rol dentro de su tema: 0=comun agresivo, 1=comun especial, 2=jefe.
inline int RolLocalDeEnemigo(TipoEnemigo tipo) { return static_cast<int>(tipo) % kEnemigosPorTema; }

// True si los enemigos de este tipo persiguen al lider durante la
// exploracion y fuerzan el combate al alcanzarlo (ver el chequeo de
// persecucion en main.cpp), en vez de quedarse quietos hasta que el
// jugador confirma con [E] como el resto. Es el enemigo "comun agresivo"
// de cada tema (rol local 0) -- generaliza lo que antes solo hacia el
// Bandido Aturdidor. Los jefes quedan afuera a proposito (unicos al fondo
// de la mazmorra: perseguir no les suma nada, el jugador ya tiene que
// enfrentarlos si o si para terminar la run).
bool EsAgresivo(TipoEnemigo tipo);

// True si 'tipo' es el jefe de su tema (rol local 2) -- generaliza el
// chequeo que antes solo distinguia a CapitanBandido (ver
// CombatEncounter::Actualizar en combat.cpp y la deteccion de victoria
// final en main.cpp).
bool EsJefe(TipoEnemigo tipo);

// Experiencia que otorga derrotar a un enemigo de este tipo, a CADA
// miembro del party que siga con vida al momento de ganar el combate (no
// se reparte entre los 4 — ver "Sistema de niveles" en docs/design.md y
// game::Character::GanarExperiencia). Por ROL, no por tipo puntual, mismo
// criterio que TirarLootDeEnemigo (item.h): el jefe de cualquier tema da
// bastante mas que un comun, y el especial un poco mas que el agresivo
// (es algo mas peligroso, con su golpe de estado propio).
int XpPorEnemigo(TipoEnemigo tipo);

// Datos del golpe especial del enemigo "comun especial" de cada tema (rol
// local 1): nombre de la accion (para el log), que efecto de estado aplica
// si impacta, y su duracion/magnitud -- generaliza el "Golpe Aturdidor" que
// antes solo tenia el Bandido Aturdidor (ver combat.cpp). Solo tiene
// sentido si RolLocalDeEnemigo(tipo) == 1; para los demas devuelve datos
// sin uso (nunca se consultan).
struct AtaqueEspecialEnemigo {
    const char* nombre;
    TipoEfecto efecto;
    int duracionTurnos;
    int magnitud;
};
AtaqueEspecialEnemigo AtaqueEspecialDe(TipoEnemigo tipo);

// Nombre de flavor del "Doble Golpe" del jefe de cada tema -- mismo patron
// de IA para los 3 (ver combat.cpp), solo cambia como se llama el golpe.
// Solo tiene sentido si EsJefe(tipo) es true.
const char* NombreDobleGolpeDeJefe(TipoEnemigo tipo);

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
