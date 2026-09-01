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
        if (efecto.tipo == game::TipoEfecto::Escudo) {
            // El Escudo no "dura" turnos: se consume por daño. Mostrar la
            // duracion interna (que usa un numero grande como centinela)
            // confundiria; en cambio se muestran los puntos que le quedan.
            std::snprintf(texto, sizeof(texto), "%s (%d pts)", game::NombreEfecto(efecto.tipo), efecto.magnitud);
        } else {
            std::snprintf(texto, sizeof(texto), "%s (%d)", game::NombreEfecto(efecto.tipo), efecto.duracionTurnos);
        }
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

// Dibuja la ficha de un enemigo. 'esSuTurno' resalta cuando la IA lo esta
// jugando; 'esObjetivo' resalta (con otro color) cuando es a quien le van a
// pegar las acciones del aliado en turno — con mas de un enemigo a la vez,
// el jugador necesita saber a cual le esta apuntando.
void DibujarFichaEnemigo(const game::Enemy& enemigo, bool esSuTurno, bool esObjetivo, int x, int y, int ancho) {
    int alto = 70;

    Color fondo = esSuTurno ? Color{ 45, 25, 25, 230 } : Color{ 20, 20, 25, 200 };
    Color borde = esSuTurno ? Color{ 220, 90, 90, 255 } : (esObjetivo ? Color{ 230, 200, 90, 255 } : Color{ 80, 80, 90, 255 });
    if (enemigo.Vencido()) {
        fondo = Color{ 15, 15, 18, 160 };
    }
    DrawRectangle(x, y, ancho, alto, fondo);
    DrawRectangleLines(x, y, ancho, alto, borde);
    if (esObjetivo && !esSuTurno && !enemigo.Vencido()) {
        // Borde extra para que el marcador de objetivo se note incluso con
        // varias fichas una debajo de otra.
        DrawRectangleLines(x - 1, y - 1, ancho + 2, alto + 2, Color{ 230, 200, 90, 180 });
    }

    char nombre[80];
    std::snprintf(nombre, sizeof(nombre), "%s%s%s", esObjetivo && !enemigo.Vencido() ? "> " : "",
                  enemigo.Nombre().c_str(), esSuTurno ? " <-- turno" : "");
    DrawText(nombre, x + 12, y + 8, 16, enemigo.Vencido() ? GRAY : RAYWHITE);

    if (enemigo.Vencido()) {
        DrawText("DERROTADO", x + ancho - 110, y + 8, 14, Color{ 140, 140, 140, 255 });
        return;
    }

    const auto& stats = enemigo.GetStats();
    float ratioHp = stats.hpMax > 0 ? (float)stats.hp / (float)stats.hpMax : 0.0f;
    DibujarBarra(x + 12, y + 32, ancho - 24, 10, ratioHp, Color{ 200, 60, 60, 255 }, Color{ 60, 20, 20, 255 });
    char textoHp[32];
    std::snprintf(textoHp, sizeof(textoHp), "HP %d/%d", stats.hp, stats.hpMax);
    DrawText(textoHp, x + 12, y + 46, 12, LIGHTGRAY);

    DibujarEfectos(enemigo.Combate(), x + ancho - 100, y + 4);
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

    // --- Enemigos, columna derecha (uno o varios, uno debajo del otro) ---
    const auto& enemigos = encuentro.Enemigos();
    int anchoEnemigo = 280;
    int xEnemigo = anchoVentana - anchoEnemigo - 24;
    int yEnemigo = 24;
    game::Enemy* enemigoEnTurno = encuentro.EnemigoEnTurno();
    int indiceObjetivo = encuentro.IndiceObjetivo();
    for (size_t i = 0; i < enemigos.size(); ++i) {
        const game::Enemy* enemigo = enemigos[i];
        bool esSuTurno = (enemigoEnTurno == enemigo);
        bool esObjetivo = (encuentro.Fase() == game::FaseCombate::TurnoAliado && (int)i == indiceObjetivo);
        DibujarFichaEnemigo(*enemigo, esSuTurno, esObjetivo, xEnemigo, yEnemigo, anchoEnemigo);
        yEnemigo += 70 + 10;
    }

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
        int enemigosVivos = 0;
        for (auto* e : enemigos) if (e->EstaVivo()) enemigosVivos++;

        char menu[200];
        if (enemigosVivos > 1) {
            std::snprintf(menu, sizeof(menu), "Turno de %s  -  [1] Atacar    [2] %s    [TAB] Cambiar objetivo",
                          enTurno->Nombre().c_str(), game::NombreHabilidadDeRol(enTurno->Rol()));
        } else {
            std::snprintf(menu, sizeof(menu), "Turno de %s  -  [1] Atacar    [2] %s",
                          enTurno->Nombre().c_str(), game::NombreHabilidadDeRol(enTurno->Rol()));
        }
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
        const char* titulo = "GAME OVER";
        int anchoTitulo = MeasureText(titulo, 46);
        DrawText(titulo, (anchoVentana - anchoTitulo) / 2, altoVentana / 2 - 130, 46, Color{ 220, 60, 60, 255 });

        const char* subtitulo = "El party ha caido...";
        int anchoSub = MeasureText(subtitulo, 20);
        DrawText(subtitulo, (anchoVentana - anchoSub) / 2, altoVentana / 2 - 68, 20, Color{ 230, 150, 150, 255 });

        const char* prompt = "presiona cualquier tecla para revivir y volver al punto de partida";
        int anchoPrompt = MeasureText(prompt, 18);
        DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana / 2 - 34, 18, Color{ 210, 210, 210, 255 });
    }
}

} // namespace ui
