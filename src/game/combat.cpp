#include "combat.h"
#include "dice.h"
#include <algorithm>
#include <cstdio>

namespace game {

namespace {

int BonusAtaque(const Stats& stats, const EstadoCombate& estado) {
    int bonus = stats.ataque / 2;
    if (estado.TieneEfecto(TipoEfecto::Debilitado)) {
        bonus -= estado.MagnitudEfecto(TipoEfecto::Debilitado);
    }
    return bonus;
}

int ClaseDeDefensa(const Stats& stats) {
    return 10 + stats.defensa;
}

bool PuedeUsarHabilidad(const Combatiente& atacante, int costoRecurso) {
    return atacante.stats->recurso >= costoRecurso;
}

} // namespace

const char* NombreHabilidadDeRol(Role rol) {
    switch (rol) {
        case Role::Tanque:  return "Golpe Provocador";
        case Role::Danio:   return "Golpe Certero";
        case Role::Soporte: return "Curar";
        case Role::Control: return "Grito Debilitante";
    }
    return "?";
}

ResultadoAccion ResolverAtaque(Combatiente& atacante, Combatiente& objetivo,
                                int dadosDano, int carasDano, bool conVentaja,
                                const char* nombreAccion) {
    ResultadoAccion r;
    int bonus = BonusAtaque(*atacante.stats, *atacante.estado);
    int dificultad = ClaseDeDefensa(*objetivo.stats);
    r.dificultad = dificultad;

    int tirada = RollD20();
    if (conVentaja) {
        int tirada2 = RollD20();
        tirada = std::max(tirada, tirada2);
    }
    r.tiradaDado = tirada;
    r.totalAtaque = tirada + bonus;

    bool critico = (tirada == 20);
    bool pifia = (tirada == 1);
    r.critico = critico;

    if (pifia) {
        r.impacto = false;
    } else if (critico) {
        r.impacto = true;
    } else {
        r.impacto = (r.totalAtaque >= dificultad);
    }

    if (r.impacto) {
        int dados = critico ? dadosDano * 2 : dadosDano;
        int dano = RollDados(dados, carasDano, bonus);
        r.dano = AplicarDano(*objetivo.stats, *objetivo.estado, dano);
    }

    char buffer[224];
    if (pifia) {
        std::snprintf(buffer, sizeof(buffer), "%s usa %s contra %s: pifia (1 natural), falla.",
                      atacante.nombre.c_str(), nombreAccion, objetivo.nombre.c_str());
    } else if (!r.impacto) {
        std::snprintf(buffer, sizeof(buffer), "%s usa %s contra %s: tira %d+%d=%d vs %d, no impacta.",
                      atacante.nombre.c_str(), nombreAccion, objetivo.nombre.c_str(),
                      r.tiradaDado, bonus, r.totalAtaque, dificultad);
    } else if (critico) {
        std::snprintf(buffer, sizeof(buffer), "%s usa %s contra %s: ¡CRITICO (20 natural)! %d de daño.",
                      atacante.nombre.c_str(), nombreAccion, objetivo.nombre.c_str(), r.dano);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%s usa %s contra %s: tira %d+%d=%d vs %d, impacta por %d.",
                      atacante.nombre.c_str(), nombreAccion, objetivo.nombre.c_str(),
                      r.tiradaDado, bonus, r.totalAtaque, dificultad, r.dano);
    }
    r.texto = buffer;
    return r;
}

ResultadoAccion EjecutarAtaqueBasico(Combatiente& atacante, Combatiente& objetivo) {
    return ResolverAtaque(atacante, objetivo, 1, 6, false, "un ataque");
}

ResultadoHabilidad EjecutarHabilidadDeRol(Combatiente& atacante, Combatiente& objetivoEnemigo, Combatiente* objetivoAliado) {
    ResultadoHabilidad r;

    switch (atacante.rol) {
        case Role::Tanque: {
            r.accion = ResolverAtaque(atacante, objetivoEnemigo, 1, 6, false, NombreHabilidadDeRol(Role::Tanque));
            r.ejecutada = true;
            r.texto = r.accion.texto;
            if (r.accion.impacto) {
                objetivoEnemigo.estado->AgregarEfecto(EfectoActivo{TipoEfecto::Marcado, 2, 1});
                r.texto += " Queda marcado.";
            }
            // Se expone para provocar, pero tambien se cubre: gana Escudo
            // propio (independiente de si el golpe conecto o no).
            atacante.estado->AgregarEfecto(EfectoActivo{TipoEfecto::Escudo, 99, 4});
            r.texto += " " + atacante.nombre + " se cubre (Escudo 4).";
            break;
        }
        case Role::Danio: {
            const int costo = 5;
            if (!PuedeUsarHabilidad(atacante, costo)) {
                r.ejecutada = false;
                r.texto = atacante.nombre + " no tiene energia suficiente para " + NombreHabilidadDeRol(Role::Danio) + ".";
                break;
            }
            atacante.stats->recurso -= costo;
            r.accion = ResolverAtaque(atacante, objetivoEnemigo, 1, 8, true, NombreHabilidadDeRol(Role::Danio));
            r.ejecutada = true;
            r.texto = r.accion.texto;
            if (r.accion.impacto && r.accion.critico) {
                objetivoEnemigo.estado->AgregarEfecto(EfectoActivo{TipoEfecto::Veneno, 3, 3});
                r.texto += " La herida sangra: " + objetivoEnemigo.nombre + " queda envenenado.";
            }
            break;
        }
        case Role::Soporte: {
            const int costo = 8;
            if (!PuedeUsarHabilidad(atacante, costo) || objetivoAliado == nullptr) {
                r.ejecutada = false;
                r.texto = atacante.nombre + " no puede curar ahora.";
                break;
            }
            atacante.stats->recurso -= costo;
            int tirada = RollDados(1, 8, 2);
            int curado = AplicarCuracion(*objetivoAliado->stats, tirada);
            r.ejecutada = true;
            char buffer[192];
            std::snprintf(buffer, sizeof(buffer), "%s usa Curar sobre %s: 1d8+2 (%d) -> recupera %d de vida.",
                          atacante.nombre.c_str(), objetivoAliado->nombre.c_str(), tirada, curado);
            r.texto = buffer;
            break;
        }
        case Role::Control: {
            const int costo = 6;
            if (!PuedeUsarHabilidad(atacante, costo)) {
                r.ejecutada = false;
                r.texto = atacante.nombre + " no tiene energia suficiente para " + NombreHabilidadDeRol(Role::Control) + ".";
                break;
            }
            atacante.stats->recurso -= costo;
            r.accion = ResolverAtaque(atacante, objetivoEnemigo, 1, 4, false, NombreHabilidadDeRol(Role::Control));
            r.ejecutada = true;
            r.texto = r.accion.texto;
            if (r.accion.impacto) {
                objetivoEnemigo.estado->AgregarEfecto(EfectoActivo{TipoEfecto::Debilitado, 2, 2});
                r.texto += " " + objetivoEnemigo.nombre + " queda debilitado.";
            }
            break;
        }
    }
    return r;
}

// --- CombatEncounter ---

CombatEncounter::CombatEncounter(Party& party, Enemy& enemigo)
    : party_(party), enemigo_(enemigo) {
    for (size_t i = 0; i < party_.Miembros().size(); ++i) {
        orden_.push_back(EntradaTurno{true, static_cast<int>(i)});
    }
    orden_.push_back(EntradaTurno{false, 0});

    std::sort(orden_.begin(), orden_.end(), [this](const EntradaTurno& a, const EntradaTurno& b) {
        float velA = a.esAliado ? party_.Miembros()[a.indice].GetStats().velocidad : enemigo_.GetStats().velocidad;
        float velB = b.esAliado ? party_.Miembros()[b.indice].GetStats().velocidad : enemigo_.GetStats().velocidad;
        return velA > velB;
    });

    log_.push_back("¡Comienza el combate contra " + enemigo_.Nombre() + "!");
    std::string ordenTexto = "Orden de turnos: ";
    for (size_t i = 0; i < orden_.size(); ++i) {
        ordenTexto += orden_[i].esAliado ? party_.Miembros()[orden_[i].indice].Nombre() : enemigo_.Nombre();
        if (i + 1 < orden_.size()) ordenTexto += " > ";
    }
    log_.push_back(ordenTexto);

    turnoActual_ = 0;
    if (!ChequearFinDeCombate()) {
        ProcesarInicioDeTurnoActual();
    }
}

void CombatEncounter::AvanzarIndice() {
    turnoActual_ = (turnoActual_ + 1) % orden_.size();
}

void CombatEncounter::ProcesarInicioDeTurnoActual() {
    while (true) {
        const EntradaTurno& e = orden_[turnoActual_];

        if (e.esAliado) {
            Character& c = party_.Miembros()[e.indice];
            if (!c.EstaVivo()) {
                AvanzarIndice();
                continue;
            }
            auto tick = c.Combate().TickInicioDeTurno();
            if (tick.danoVeneno > 0) {
                int recibido = c.RecibirDano(tick.danoVeneno);
                log_.push_back(c.Nombre() + " sufre " + std::to_string(recibido) + " de daño por veneno.");
                if (!c.EstaVivo()) {
                    log_.push_back(c.Nombre() + " cae.");
                    if (ChequearFinDeCombate()) return;
                    AvanzarIndice();
                    continue;
                }
            }
            if (tick.aturdido) {
                log_.push_back(c.Nombre() + " esta aturdido y pierde el turno.");
                AvanzarIndice();
                continue;
            }
            fase_ = FaseCombate::TurnoAliado;
            return;
        } else {
            if (!enemigo_.EstaVivo()) {
                AvanzarIndice();
                continue;
            }
            auto tick = enemigo_.Combate().TickInicioDeTurno();
            if (tick.danoVeneno > 0) {
                int recibido = enemigo_.RecibirDano(tick.danoVeneno);
                log_.push_back(enemigo_.Nombre() + " sufre " + std::to_string(recibido) + " de daño por veneno.");
                if (!enemigo_.EstaVivo()) {
                    enemigo_.MarcarVencido();
                    log_.push_back(enemigo_.Nombre() + " ha sido derrotado.");
                    if (ChequearFinDeCombate()) return;
                    AvanzarIndice();
                    continue;
                }
            }
            if (tick.aturdido) {
                log_.push_back(enemigo_.Nombre() + " esta aturdido y pierde el turno.");
                AvanzarIndice();
                continue;
            }
            fase_ = FaseCombate::TurnoEnemigo;
            timerIA_ = 0.0f;
            return;
        }
    }
}

bool CombatEncounter::ChequearFinDeCombate() {
    if (!enemigo_.EstaVivo()) {
        fase_ = FaseCombate::Ganado;
        log_.push_back("¡Victoria!");
        return true;
    }
    bool algunAliadoVivo = false;
    for (auto& c : party_.Miembros()) {
        if (c.EstaVivo()) { algunAliadoVivo = true; break; }
    }
    if (!algunAliadoVivo) {
        fase_ = FaseCombate::Perdido;
        log_.push_back("El party ha caido...");
        return true;
    }
    return false;
}

Character* CombatEncounter::AliadoEnTurno() {
    if (fase_ != FaseCombate::TurnoAliado) return nullptr;
    return &party_.Miembros()[orden_[turnoActual_].indice];
}

void CombatEncounter::AccionAtaqueBasico() {
    if (fase_ != FaseCombate::TurnoAliado) return;
    Character& atacante = party_.Miembros()[orden_[turnoActual_].indice];

    Combatiente cAtacante{atacante.Nombre(), &atacante.GetStatsMut(), &atacante.Combate(), true, atacante.Rol()};
    Combatiente cEnemigo{enemigo_.Nombre(), &enemigo_.GetStatsMut(), &enemigo_.Combate(), false, Role::Tanque};

    ResultadoAccion r = EjecutarAtaqueBasico(cAtacante, cEnemigo);
    log_.push_back(r.texto);
    if (!enemigo_.EstaVivo()) enemigo_.MarcarVencido();

    if (ChequearFinDeCombate()) return;
    AvanzarIndice();
    ProcesarInicioDeTurnoActual();
}

void CombatEncounter::AccionHabilidadDeRol() {
    if (fase_ != FaseCombate::TurnoAliado) return;
    Character& atacante = party_.Miembros()[orden_[turnoActual_].indice];

    Combatiente cAtacante{atacante.Nombre(), &atacante.GetStatsMut(), &atacante.Combate(), true, atacante.Rol()};
    Combatiente cEnemigo{enemigo_.Nombre(), &enemigo_.GetStatsMut(), &enemigo_.Combate(), false, Role::Tanque};

    Combatiente aliadoTemp;
    Combatiente* objetivoAliado = nullptr;
    if (atacante.Rol() == Role::Soporte) {
        Character* peorHerido = nullptr;
        for (auto& m : party_.Miembros()) {
            if (!m.EstaVivo()) continue;
            if (peorHerido == nullptr || m.GetStats().hp < peorHerido->GetStats().hp) peorHerido = &m;
        }
        if (peorHerido != nullptr) {
            aliadoTemp = Combatiente{peorHerido->Nombre(), &peorHerido->GetStatsMut(), &peorHerido->Combate(), true, peorHerido->Rol()};
            objetivoAliado = &aliadoTemp;
        }
    }

    ResultadoHabilidad r = EjecutarHabilidadDeRol(cAtacante, cEnemigo, objetivoAliado);
    log_.push_back(r.texto);
    if (!enemigo_.EstaVivo()) enemigo_.MarcarVencido();

    if (!r.ejecutada) {
        // No se pudo usar (p.ej. sin recurso): no se consume el turno.
        return;
    }

    if (ChequearFinDeCombate()) return;
    AvanzarIndice();
    ProcesarInicioDeTurnoActual();
}

void CombatEncounter::Actualizar(float deltaSeconds) {
    if (fase_ != FaseCombate::TurnoEnemigo) return;
    timerIA_ += deltaSeconds;
    if (timerIA_ < kEsperaTurnoIA) return;

    Character* objetivo = nullptr;
    for (auto& m : party_.Miembros()) {
        if (!m.EstaVivo()) continue;
        if (objetivo == nullptr || m.GetStats().hp < objetivo->GetStats().hp) objetivo = &m;
    }
    if (objetivo == nullptr) {
        ChequearFinDeCombate();
        return;
    }

    Combatiente cEnemigo{enemigo_.Nombre(), &enemigo_.GetStatsMut(), &enemigo_.Combate(), false, Role::Tanque};
    Combatiente cObjetivo{objetivo->Nombre(), &objetivo->GetStatsMut(), &objetivo->Combate(), true, objetivo->Rol()};

    ResultadoAccion r = EjecutarAtaqueBasico(cEnemigo, cObjetivo);
    log_.push_back(r.texto);

    if (ChequearFinDeCombate()) return;
    AvanzarIndice();
    ProcesarInicioDeTurnoActual();
}

} // namespace game
