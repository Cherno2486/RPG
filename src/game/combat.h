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

// Maneja un encuentro de combate contra uno o mas enemigos a la vez (todos
// los de una misma sala de la mazmorra, tipicamente): orden de turnos por
// velocidad entre TODOS los combatientes (aliados y enemigos intercalados),
// efectos de estado, y las acciones del jugador y de cada enemigo.
class CombatEncounter {
public:
    // 'enemigos' no puede estar vacio. Los punteros deben seguir siendo
    // validos durante toda la vida del encuentro (apuntan a los Enemy reales
    // de la mazmorra).
    CombatEncounter(Party& party, std::vector<Enemy*> enemigos);

    FaseCombate Fase() const { return fase_; }
    const std::vector<std::string>& Log() const { return log_; }

    Party& PartyRef() { return party_; }
    const std::vector<Enemy*>& Enemigos() const { return enemigos_; }

    // Personaje del party cuyo turno es ahora (nullptr si Fase() != TurnoAliado).
    Character* AliadoEnTurno();
    // Enemigo cuyo turno es ahora (nullptr si Fase() != TurnoEnemigo).
    Enemy* EnemigoEnTurno();

    // Indice (en Enemigos()) del enemigo actualmente seleccionado como
    // objetivo de las acciones del aliado en turno. Se autocorrige si el
    // objetivo previo murio o todavia no se eligio ninguno (apunta al
    // primer enemigo vivo). -1 si no queda ningun enemigo vivo.
    int IndiceObjetivo() const { return objetivoActual_; }
    // Cambia el objetivo al siguiente enemigo vivo (para cuando hay mas de
    // uno). No hace nada si hay 0 o 1 enemigos vivos.
    void CiclarObjetivo();

    // Acciones del aliado en turno, contra el enemigo en IndiceObjetivo().
    // Si la habilidad no se pudo ejecutar (p.ej. sin recurso), el turno NO
    // se consume, para que el jugador elija otra cosa.
    void AccionAtaqueBasico();
    void AccionHabilidadDeRol();

    // Llamar cada frame: si es turno de un enemigo, hace avanzar un pequeño
    // temporizador y cuando se cumple resuelve su turno solo.
    void Actualizar(float deltaSeconds);

private:
    struct EntradaTurno {
        bool esAliado;
        int indice;  // indice en party_.Miembros() o en enemigos_, segun esAliado
    };

    Party& party_;
    std::vector<Enemy*> enemigos_;
    std::vector<EntradaTurno> orden_;
    size_t turnoActual_ = 0;
    int objetivoActual_ = 0;
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
    // Deja objetivoActual_ apuntando a un enemigo vivo (el mismo si ya lo
    // estaba, o el primero vivo que encuentre); -1 si no queda ninguno.
    void AsegurarObjetivoValido();
};

} // namespace game
