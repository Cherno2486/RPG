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

CombatEncounter::CombatEncounter(Party& party, std::vector<Enemy*> enemigos)
    : party_(party), enemigos_(std::move(enemigos)) {
    for (size_t i = 0; i < party_.Miembros().size(); ++i) {
        orden_.push_back(EntradaTurno{true, static_cast<int>(i)});
    }
    for (size_t i = 0; i < enemigos_.size(); ++i) {
        orden_.push_back(EntradaTurno{false, static_cast<int>(i)});
    }

    std::sort(orden_.begin(), orden_.end(), [this](const EntradaTurno& a, const EntradaTurno& b) {
        float velA = a.esAliado ? party_.Miembros()[a.indice].GetStats().velocidad : enemigos_[a.indice]->GetStats().velocidad;
        float velB = b.esAliado ? party_.Miembros()[b.indice].GetStats().velocidad : enemigos_[b.indice]->GetStats().velocidad;
        return velA > velB;
    });

    if (enemigos_.size() == 1) {
        log_.push_back("¡Comienza el combate contra " + enemigos_[0]->Nombre() + "!");
    } else {
        std::string nombres;
        for (size_t i = 0; i < enemigos_.size(); ++i) {
            nombres += enemigos_[i]->Nombre();
            if (i + 1 < enemigos_.size()) nombres += ", ";
        }
        char cabecera[256];
        std::snprintf(cabecera, sizeof(cabecera), "¡Comienza el combate contra %zu enemigos: ", enemigos_.size());
        log_.push_back(std::string(cabecera) + nombres + "!");
    }
    std::string ordenTexto = "Orden de turnos: ";
    for (size_t i = 0; i < orden_.size(); ++i) {
        ordenTexto += orden_[i].esAliado ? party_.Miembros()[orden_[i].indice].Nombre() : enemigos_[orden_[i].indice]->Nombre();
        if (i + 1 < orden_.size()) ordenTexto += " > ";
    }
    log_.push_back(ordenTexto);

    turnoActual_ = 0;
    AsegurarObjetivoValido();
    if (!ChequearFinDeCombate()) {
        ProcesarInicioDeTurnoActual();
    }
}

void CombatEncounter::AvanzarIndice() {
    turnoActual_ = (turnoActual_ + 1) % orden_.size();
}

void CombatEncounter::AsegurarObjetivoValido() {
    if (objetivoActual_ >= 0 && objetivoActual_ < (int)enemigos_.size() && enemigos_[objetivoActual_]->EstaVivo()) {
        return;  // el objetivo actual sigue siendo valido
    }
    objetivoActual_ = -1;
    for (size_t i = 0; i < enemigos_.size(); ++i) {
        if (enemigos_[i]->EstaVivo()) {
            objetivoActual_ = (int)i;
            break;
        }
    }
}

void CombatEncounter::CiclarObjetivo() {
    if (enemigos_.empty()) return;
    int vivos = 0;
    for (auto* e : enemigos_) if (e->EstaVivo()) vivos++;
    if (vivos <= 1) return;

    int siguiente = objetivoActual_;
    for (size_t paso = 0; paso < enemigos_.size(); ++paso) {
        siguiente = (siguiente + 1) % (int)enemigos_.size();
        if (enemigos_[siguiente]->EstaVivo()) {
            objetivoActual_ = siguiente;
            return;
        }
    }
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
            AsegurarObjetivoValido();
            fase_ = FaseCombate::TurnoAliado;
            return;
        } else {
            Enemy& en = *enemigos_[e.indice];
            if (!en.EstaVivo()) {
                AvanzarIndice();
                continue;
            }
            auto tick = en.Combate().TickInicioDeTurno();
            if (tick.danoVeneno > 0) {
                int recibido = en.RecibirDano(tick.danoVeneno);
                log_.push_back(en.Nombre() + " sufre " + std::to_string(recibido) + " de daño por veneno.");
                if (!en.EstaVivo()) {
                    en.MarcarVencido();
                    log_.push_back(en.Nombre() + " ha sido derrotado.");
                    if (ChequearFinDeCombate()) return;
                    AvanzarIndice();
                    continue;
                }
            }
            if (tick.aturdido) {
                log_.push_back(en.Nombre() + " esta aturdido y pierde el turno.");
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
    bool algunEnemigoVivo = false;
    for (auto* e : enemigos_) {
        if (e->EstaVivo()) { algunEnemigoVivo = true; break; }
    }
    if (!algunEnemigoVivo) {
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

Enemy* CombatEncounter::EnemigoEnTurno() {
    if (fase_ != FaseCombate::TurnoEnemigo) return nullptr;
    return enemigos_[orden_[turnoActual_].indice];
}

void CombatEncounter::AccionAtaqueBasico() {
    if (fase_ != FaseCombate::TurnoAliado) return;
    AsegurarObjetivoValido();
    if (objetivoActual_ < 0) return;
    Character& atacante = party_.Miembros()[orden_[turnoActual_].indice];
    Enemy& objetivo = *enemigos_[objetivoActual_];

    Combatiente cAtacante{atacante.Nombre(), &atacante.GetStatsMut(), &atacante.Combate(), true, atacante.Rol()};
    Combatiente cEnemigo{objetivo.Nombre(), &objetivo.GetStatsMut(), &objetivo.Combate(), false, Role::Tanque};

    ResultadoAccion r = EjecutarAtaqueBasico(cAtacante, cEnemigo);
    log_.push_back(r.texto);
    if (!objetivo.EstaVivo()) objetivo.MarcarVencido();

    if (ChequearFinDeCombate()) return;
    AsegurarObjetivoValido();
    AvanzarIndice();
    ProcesarInicioDeTurnoActual();
}

void CombatEncounter::AccionHabilidadDeRol() {
    if (fase_ != FaseCombate::TurnoAliado) return;
    AsegurarObjetivoValido();
    if (objetivoActual_ < 0) return;
    Character& atacante = party_.Miembros()[orden_[turnoActual_].indice];
    Enemy& objetivo = *enemigos_[objetivoActual_];

    Combatiente cAtacante{atacante.Nombre(), &atacante.GetStatsMut(), &atacante.Combate(), true, atacante.Rol()};
    Combatiente cEnemigo{objetivo.Nombre(), &objetivo.GetStatsMut(), &objetivo.Combate(), false, Role::Tanque};

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
    if (!objetivo.EstaVivo()) objetivo.MarcarVencido();

    if (!r.ejecutada) {
        // No se pudo usar (p.ej. sin recurso): no se consume el turno.
        return;
    }

    if (ChequearFinDeCombate()) return;
    AsegurarObjetivoValido();
    AvanzarIndice();
    ProcesarInicioDeTurnoActual();
}

void CombatEncounter::Actualizar(float deltaSeconds) {
    if (fase_ != FaseCombate::TurnoEnemigo) return;
    timerIA_ += deltaSeconds;
    if (timerIA_ < kEsperaTurnoIA) return;

    Enemy& atacante = *enemigos_[orden_[turnoActual_].indice];

    // Elige a quien ataca este enemigo: si esta Marcado (provocado por el
    // Golpe Provocador del Tanque) prioriza al Tanque; si no, al aliado
    // vivo con menos HP. Es una lambda porque el Capitan Bandido puede
    // necesitar volver a elegir objetivo entre su primer y segundo golpe
    // (si el primero mato a quien tenia en la mira).
    auto elegirObjetivo = [&]() -> Character* {
        Character* obj = nullptr;
        if (atacante.Combate().TieneEfecto(TipoEfecto::Marcado)) {
            for (auto& m : party_.Miembros()) {
                if (m.EstaVivo() && m.Rol() == Role::Tanque) { obj = &m; break; }
            }
        }
        if (obj == nullptr) {
            for (auto& m : party_.Miembros()) {
                if (!m.EstaVivo()) continue;
                if (obj == nullptr || m.GetStats().hp < obj->GetStats().hp) obj = &m;
            }
        }
        return obj;
    };

    Character* objetivo = elegirObjetivo();
    if (objetivo == nullptr) {
        ChequearFinDeCombate();
        return;
    }

    Combatiente cEnemigo{atacante.Nombre(), &atacante.GetStatsMut(), &atacante.Combate(), false, Role::Tanque};

    // Resuelve un golpe de 'atacante' contra 'obj' y lo agrega al log; con
    // 'aturde' en true, si impacta aplica Aturdido (Golpe Aturdidor, tanto
    // el del Bandido comun como el del Capitan).
    auto golpear = [&](Character& obj, int dados, int caras, const char* nombreAccion, bool aturde) {
        Combatiente cObjetivo{obj.Nombre(), &obj.GetStatsMut(), &obj.Combate(), true, obj.Rol()};
        ResultadoAccion r = ResolverAtaque(cEnemigo, cObjetivo, dados, caras, false, nombreAccion);
        if (aturde && r.impacto) {
            cObjetivo.estado->AgregarEfecto(EfectoActivo{TipoEfecto::Aturdido, 1, 0});
            r.texto += " " + cObjetivo.nombre + " queda aturdido.";
        }
        log_.push_back(r.texto);
    };

    if (atacante.Tipo() == TipoEnemigo::CapitanBandido) {
        // Jefe de mazmorra: alterna entre ataque basico, Golpe Aturdidor
        // (igual que el Bandido comun) y "Doble Tajo" (dos golpes basicos
        // en el mismo turno, re-eligiendo objetivo para el segundo por si
        // el primero se llevo puesto a quien tenia en la mira). Por debajo
        // del 40% de HP entra en furia: deja de aturdir y usa Doble Tajo
        // siempre, todo o nada en el tramo final de la pelea.
        const Stats& stats = atacante.GetStats();
        bool enfurecido = stats.hpMax > 0 && stats.hp <= stats.hpMax * 0.4f;
        int tirada = Roll(10);
        if (enfurecido || tirada <= 4) {
            golpear(*objetivo, 1, 6, "Doble Tajo", false);
            if (ChequearFinDeCombate()) return;
            Character* segundoObjetivo = elegirObjetivo();
            if (segundoObjetivo != nullptr) {
                golpear(*segundoObjetivo, 1, 6, "Doble Tajo", false);
                if (ChequearFinDeCombate()) return;
            }
        } else if (tirada <= 7) {
            golpear(*objetivo, 1, 4, "Golpe Aturdidor", true);
            if (ChequearFinDeCombate()) return;
        } else {
            golpear(*objetivo, 1, 6, "un ataque", false);
            if (ChequearFinDeCombate()) return;
        }
    } else if (atacante.Tipo() == TipoEnemigo::BanditoAturdidor && Roll(2) == 1) {
        // El Bandido Aturdidor a veces, en vez de un ataque basico, usa un
        // golpe mas debil pero que aturde (le hace perder el turno al objetivo).
        golpear(*objetivo, 1, 4, "Golpe Aturdidor", true);
        if (ChequearFinDeCombate()) return;
    } else {
        golpear(*objetivo, 1, 6, "un ataque", false);
        if (ChequearFinDeCombate()) return;
    }

    AsegurarObjetivoValido();
    AvanzarIndice();
    ProcesarInicioDeTurnoActual();
}

} // namespace game
