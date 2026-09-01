#include "combat_ui.h"
#include "raylib.h"
#include <cstdio>

namespace ui {

namespace {

Color ColorDeRol(game::Role rol) {
    switch (rol) {
        case game::Role::Tanque:  return Color{ 90, 130, 220, 255 };
        case game::Role::Danio:   return Color{ 220, 90, 90, 255 };
        case game::Role::Soporte: return Color{ 100, 210, 130, 255 };
        case game::Role::Control: return Color{ 210, 170, 90, 255 };
    }
    return WHITE;
}

// Dibuja una lista corta de "tags" de texto con los efectos activos (por
// ejemplo "Envenenado x2", "Escudo 5"), una debajo de otra.
void DibujarEfectos(const game::EstadoCombate& combate, int x, int y) {
    int filaY = y;
    for (const auto& efecto : combate.Efectos()) {
        char texto[64];
        std::snprintf(texto, sizeof(texto), "%s (%d)", game::NombreEfecto(efecto.tipo), efecto.duracionTurnos);
        DrawText(texto, x, filaY, 12, Color{ 230, 200, 120, 255 });
        filaY += 14;
    }
}

void DibujarBarra(int x, int y, int ancho, int alto, float ratio, Color colorLleno, Color colorFondo) {
    DrawRectangle(x, y, ancho, alto, colorFondo);
    DrawRectangle(x, y, (int)(ancho * ratio), alto, colorLleno);
    DrawRectangleLines(x, y, ancho, alto, Color{ 20, 20, 20, 255 });
}

void DibujarFichaAliado(const game::Character& personaje, bool esSuTurno, int x, int y, int ancho) {
    const auto& stats = personaje.GetStats();
    int alto = stats.recursoMax > 0 ? 78 : 60;

    Color fondo = esSuTurno ? Color{ 45, 45, 30, 230 } : Color{ 20, 20, 25, 200 };
    Color borde = esSuTurno ? Color{ 230, 200, 90, 255 } : Color{ 80, 80, 90, 255 };
    DrawRectangle(x, y, ancho, alto, fondo);
    DrawRectangleLines(x, y, ancho, alto, borde);

    DrawCircle(x + 18, y + 18, 10.0f, ColorDeRol(personaje.Rol()));

    char nombre[48];
    std::snprintf(nombre, sizeof(nombre), "%s%s", personaje.Nombre().c_str(), esSuTurno ? " <-- turno" : "");
    DrawText(nombre, x + 36, y + 8, 15, RAYWHITE);
    DrawText(game::RoleName(personaje.Rol()), x + 36, y + 26, 12, LIGHTGRAY);

    if (!personaje.EstaVivo()) {
        DrawText("CAIDO", x + ancho - 70, y + 8, 14, Color{ 200, 60, 60, 255 });
    }

    float ratioHp = stats.hpMax > 0 ? (float)stats.hp / (float)stats.hpMax : 0.0f;
    DibujarBarra(x + 36, y + 42, ancho - 50, 8, ratioHp, Color{ 200, 60, 60, 255 }, Color{ 60, 20, 20, 255 });
    char textoHp[32];
    std::snprintf(textoHp, sizeof(textoHp), "HP %d/%d", stats.hp, stats.hpMax);
    DrawText(textoHp, x + 36, y + 52, 10, LIGHTGRAY);

    if (stats.recursoMax > 0) {
        float ratioRecurso = (float)stats.recurso / (float)stats.recursoMax;
        DibujarBarra(x + 36, y + 62, ancho - 50, 6, ratioRecurso, Color{ 90, 140, 220, 255 }, Color{ 25, 35, 55, 255 });
    }

    DibujarEfectos(personaje.Combate(), x + ancho - 90, y + 4);
}

} // namespace

void DibujarCombate(game::CombatEncounter& encuentro, int anchoVentana, int altoVentana) {
    // Fondo semitransparente para que se note que estamos "en combate".
    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 10, 8, 15, 235 });

    // --- Party, columna izquierda ---
    int x = 24;
    int y = 24;
    int anchoFicha = 260;
    game::Character* enTurno = encuentro.AliadoEnTurno();
    for (auto& personaje : encuentro.PartyRef().Miembros()) {
        bool esSuTurno = (enTurno == &personaje);
        DibujarFichaAliado(personaje, esSuTurno, x, y, anchoFicha);
        y += (personaje.GetStats().recursoMax > 0 ? 78 : 60) + 10;
    }

    // --- Enemigo, arriba a la derecha ---
    const game::Enemy& enemigo = encuentro.EnemyRef();
    int anchoEnemigo = 280;
    int xEnemigo = anchoVentana - anchoEnemigo - 24;
    int yEnemigo = 24;
    bool esTurnoEnemigo = (encuentro.Fase() == game::FaseCombate::TurnoEnemigo);
    DrawRectangle(xEnemigo, yEnemigo, anchoEnemigo, 70,
                  esTurnoEnemigo ? Color{ 45, 25, 25, 230 } : Color{ 20, 20, 25, 200 });
    DrawRectangleLines(xEnemigo, yEnemigo, anchoEnemigo, 70,
                        esTurnoEnemigo ? Color{ 220, 90, 90, 255 } : Color{ 80, 80, 90, 255 });
    char nombreEnemigo[64];
    std::snprintf(nombreEnemigo, sizeof(nombreEnemigo), "%s%s",
                   enemigo.Nombre().c_str(), esTurnoEnemigo ? " <-- turno" : "");
    DrawText(nombreEnemigo, xEnemigo + 12, yEnemigo + 8, 16, RAYWHITE);
    const auto& statsEnemigo = enemigo.GetStats();
    float ratioHpEnemigo = statsEnemigo.hpMax > 0 ? (float)statsEnemigo.hp / (float)statsEnemigo.hpMax : 0.0f;
    DibujarBarra(xEnemigo + 12, yEnemigo + 32, anchoEnemigo - 24, 10, ratioHpEnemigo,
                 Color{ 200, 60, 60, 255 }, Color{ 60, 20, 20, 255 });
    char textoHpEnemigo[32];
    std::snprintf(textoHpEnemigo, sizeof(textoHpEnemigo), "HP %d/%d", statsEnemigo.hp, statsEnemigo.hpMax);
    DrawText(textoHpEnemigo, xEnemigo + 12, yEnemigo + 46, 12, LIGHTGRAY);
    DibujarEfectos(enemigo.Combate(), xEnemigo + anchoEnemigo - 100, yEnemigo + 4);

    // --- Log de combate, abajo ---
    const auto& log = encuentro.Log();
    int altoLog = 150;
    int yLog = altoVentana - altoLog - 24;
    DrawRectangle(24, yLog, anchoVentana - 48, altoLog, Color{ 15, 15, 20, 220 });
    DrawRectangleLines(24, yLog, anchoVentana - 48, altoLog, Color{ 80, 80, 90, 255 });

    int maxLineas = (altoLog - 16) / 18;
    int desde = (int)log.size() > maxLineas ? (int)log.size() - maxLineas : 0;
    int filaY = yLog + 8;
    for (int i = desde; i < (int)log.size(); ++i) {
        DrawText(log[i].c_str(), 34, filaY, 14, RAYWHITE);
        filaY += 18;
    }

    // --- Menu de accion / mensaje de fin ---
    if (encuentro.Fase() == game::FaseCombate::TurnoAliado && enTurno != nullptr) {
        char menu[160];
        std::snprintf(menu, sizeof(menu), "Turno de %s  -  [1] Atacar    [2] %s",
                      enTurno->Nombre().c_str(), game::NombreHabilidadDeRol(enTurno->Rol()));
        int anchoTexto = MeasureText(menu, 20);
        int xMenu = (anchoVentana - anchoTexto) / 2 - 16;
        DrawRectangle(xMenu, yLog - 44, anchoTexto + 32, 34, Color{ 45, 45, 30, 230 });
        DrawRectangleLines(xMenu, yLog - 44, anchoTexto + 32, 34, Color{ 230, 200, 90, 255 });
        DrawText(menu, xMenu + 16, yLog - 38, 20, Color{ 255, 235, 180, 255 });
    } else if (encuentro.Fase() == game::FaseCombate::TurnoEnemigo) {
        const char* texto = "El enemigo esta actuando...";
        int anchoTexto = MeasureText(texto, 20);
        DrawText(texto, (anchoVentana - anchoTexto) / 2, yLog - 38, 20, Color{ 220, 150, 150, 255 });
    } else if (encuentro.Fase() == game::FaseCombate::Ganado) {
        const char* texto = "¡VICTORIA!  -  presiona cualquier tecla para continuar";
        int anchoTexto = MeasureText(texto, 26);
        DrawText(texto, (anchoVentana - anchoTexto) / 2, altoVentana / 2 - 100, 26, Color{ 140, 230, 140, 255 });
    } else if (encuentro.Fase() == game::FaseCombate::Perdido) {
        const char* texto = "DERROTA  -  presiona cualquier tecla para continuar";
        int anchoTexto = MeasureText(texto, 26);
        DrawText(texto, (anchoVentana - anchoTexto) / 2, altoVentana / 2 - 100, 26, Color{ 230, 120, 120, 255 });
    }
}

} // namespace ui
