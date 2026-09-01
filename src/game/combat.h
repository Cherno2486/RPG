#pragma once
#include <string>
#include <vector>
#include "character.h"
#include "party.h"
#include "enemy.h"
#include "effects.h"

// Sistema de combate por turnos "tipo BG3": tiradas de d20 contra una
// dificultad para impactar, dados de daño, ventaja en algunas habilidades,
// y efectos de estado (aturdir, veneno, escudo, debilitar, marcar) que se
// pueden aplicar y que se van consumiendo turno a turno.

namespace game {

// Referencia liviana a "quien participa de esta accion" (un personaje del
// party o el enemigo), para que la resolucion de ataques/habilidades no
// tenga que duplicarse entre Character y Enemy.
struct Combatiente {
    std::string nombre;
    Stats* stats = nullptr;
    EstadoCombate* estado = nullptr;
    bool esAliado = false;
    Role rol = Role::Tanque;  // valido solo si esAliado
};

struct ResultadoAccion {
    bool impacto = false;
    bool critico = false;
    int tiradaDado = 0;
    int totalAtaque = 0;
    int dificultad = 0;
    int dano = 0;
    std::string texto;  // linea lista para el log de combate
};

// Nombre de la habilidad de rol de cada uno (para mostrar en la UI).
const char* NombreHabilidadDeRol(Role rol);

// Ataque basico: 1d6 + bono de ataque, sin ventaja, sin efecto adicional.
ResultadoAccion EjecutarAtaqueBasico(Combatiente& atacante, Combatiente& objetivo);

struct ResultadoHabilidad {
    bool ejecutada = false;   // false si no se pudo usar (p.ej. sin recurso)
    ResultadoAccion accion;   // valido si ejecutada y la habilidad ataca
    std::string texto;
};

// Ejecuta la habilidad de rol de 'atacante'. 'objetivoEnemigo' se usa para
// Tanque/Danio; 'objetivoAliado' (puede ser nullptr) se usa para Soporte.
ResultadoHabilidad EjecutarHabilidadDeRol(Combatiente& atacante, Combatiente& objetivoEnemigo, Combatiente* objetivoAliado);

enum class FaseCombate {
    TurnoAliado,   // esperando que el jugador elija accion para AliadoEnTurno()
    TurnoEnemigo,  // la IA va a actuar (con una pequeña demora para que se lea el log)
    Ganado,
    Perdido,
};

// Maneja un encuentro de combate contra un unico enemigo: orden de turnos por
// velocidad, efectos de estado, y las acciones del jugador y del enemigo.
class CombatEncounter {
public:
    CombatEncounter(Party& party, Enemy& enemigo);

    FaseCombate Fase() const { return fase_; }
    const std::vector<std::string>& Log() const { return log_; }

    Party& PartyRef() { return party_; }
    Enemy& EnemyRef() { return enemigo_; }

    // Personaje del party cuyo turno es ahora (nullptr si Fase() != TurnoAliado).
    Character* AliadoEnTurno();

    // Acciones del aliado en turno. Si la habilidad no se pudo ejecutar (p.ej.
    // sin recurso), el turno NO se consume, para que el jugador elija otra cosa.
    void AccionAtaqueBasico();
    void AccionHabilidadDeRol();

    // Llamar cada frame: si es turno del enemigo, hace avanzar un pequeño
    // temporizador y cuando se cumple resuelve su turno solo.
    void Actualizar(float deltaSeconds);

private:
    struct EntradaTurno {
        bool esAliado;
        int indice;  // indice en party_.Miembros(); ignorado si !esAliado (un solo enemigo)
    };

    Party& party_;
    Enemy& enemigo_;
    std::vector<EntradaTurno> orden_;
    size_t turnoActual_ = 0;
    FaseCombate fase_ = FaseCombate::TurnoAliado;
    float timerIA_ = 0.0f;
    std::vector<std::string> log_;

    static constexpr float kEsperaTurnoIA = 1.1f;  // segundos antes de que el enemigo actue

    void AvanzarIndice();
    // Procesa el inicio del turno de quien esta en orden_[turnoActual_]: tickea
    // sus efectos (veneno/aturdido), y si esta vivo y puede actuar, deja fase_
    // en TurnoAliado o TurnoEnemigo segun corresponda. Si esta aturdido o
    // muerto, avanza sola al siguiente.
    void ProcesarInicioDeTurnoActual();
    bool ChequearFinDeCombate();  // devuelve true si el combate termino (Ganado/Perdido)
};

} // namespace game
