#include "character_sheet_ui.h"
#include "raylib.h"
#include <cstdio>

namespace ui {

namespace {

// Mismo criterio que el resto de las pantallas de UI (ui.cpp/combat_ui.cpp/
// inventory_ui.cpp): el retrato reusa el sprite de personaje del mapa, acá
// un poco más grande que en las otras fichas — esta pantalla tiene más
// lugar y es la única de las tres pensada para mirarse con calma, no de
// pasada durante la exploración o el combate.
constexpr float kEscalaRetrato = 1.5f;

constexpr int kAnchoFicha = 280;
constexpr int kAltoFicha = 250;
constexpr int kEspacioEntreFichas = 20;

void DibujarBarra(int x, int y, int ancho, int alto, float ratio, Color colorLleno, Color colorFondo) {
    DrawRectangle(x, y, ancho, alto, colorFondo);
    DrawRectangle(x, y, (int)(ancho * ratio), alto, colorLleno);
    DrawRectangleLines(x, y, ancho, alto, Color{ 20, 20, 20, 255 });
}

void DibujarFicha(const game::Character& personaje, bool esLider, int x, int y, const render::SpriteSet& sprites) {
    DrawRectangle(x, y, kAnchoFicha, kAltoFicha, Color{ 20, 20, 25, 230 });
    DrawRectangleLines(x, y, kAnchoFicha, kAltoFicha, Color{ 80, 80, 90, 255 });

    render::DibujarSpriteCentrado(sprites.Personaje(personaje.Rol()), Vector2{ (float)(x + 26), (float)(y + 26) }, kEscalaRetrato);
    if (!personaje.EstaVivo()) {
        DrawLine(x + 12, y + 12, x + 40, y + 40, Color{ 220, 60, 60, 255 });
    }

    char nombre[48];
    std::snprintf(nombre, sizeof(nombre), "%s%s", personaje.Nombre().c_str(), esLider ? " (lider)" : "");
    DrawText(nombre, x + 56, y + 8, 16, RAYWHITE);

    char rolYNivel[48];
    std::snprintf(rolYNivel, sizeof(rolYNivel), "%s  Nv.%d", game::RoleName(personaje.Rol()), personaje.Nivel());
    DrawText(rolYNivel, x + 56, y + 28, 13, Color{ 230, 200, 90, 255 });

    const auto& stats = personaje.GetStats();
    int filaY = y + 58;

    // --- HP ---
    float ratioHp = stats.hpMax > 0 ? (float)stats.hp / (float)stats.hpMax : 0.0f;
    DibujarBarra(x + 16, filaY, kAnchoFicha - 32, 9, ratioHp, Color{ 200, 60, 60, 255 }, Color{ 60, 20, 20, 255 });
    char textoHp[32];
    std::snprintf(textoHp, sizeof(textoHp), "HP %d/%d", stats.hp, stats.hpMax);
    DrawText(textoHp, x + 16, filaY + 12, 11, LIGHTGRAY);
    filaY += 30;

    // --- Recurso (Resistencia/Concentracion), solo si el rol lo usa ---
    if (stats.recursoMax > 0) {
        bool esConcentracion = game::UsaConcentracion(personaje.Rol());
        Color colorRecurso = esConcentracion ? Color{ 150, 110, 220, 255 } : Color{ 220, 150, 70, 255 };
        Color colorRecursoFondo = esConcentracion ? Color{ 40, 30, 55, 255 } : Color{ 55, 40, 20, 255 };
        float ratioRecurso = (float)stats.recurso / (float)stats.recursoMax;
        DibujarBarra(x + 16, filaY, kAnchoFicha - 32, 9, ratioRecurso, colorRecurso, colorRecursoFondo);
        char textoRecurso[48];
        std::snprintf(textoRecurso, sizeof(textoRecurso), "%s %d/%d",
                      game::NombreRecurso(personaje.Rol()), stats.recurso, stats.recursoMax);
        DrawText(textoRecurso, x + 16, filaY + 12, 11, LIGHTGRAY);
        filaY += 30;
    }

    // --- Experiencia hacia el proximo nivel (ver "Sistema de niveles" en
    // docs/design.md) — "nivel maximo" en vez de una barra vacia una vez
    // alcanzado kNivelMaximo. ---
    int xpProximoNivel = game::ExperienciaParaNivel(personaje.Nivel());
    char textoXp[32];
    if (xpProximoNivel > 0) {
        float ratioXp = (float)personaje.Experiencia() / (float)xpProximoNivel;
        DibujarBarra(x + 16, filaY, kAnchoFicha - 32, 7, ratioXp, Color{ 130, 150, 230, 255 }, Color{ 40, 40, 55, 255 });
        std::snprintf(textoXp, sizeof(textoXp), "XP %d/%d", personaje.Experiencia(), xpProximoNivel);
    } else {
        DibujarBarra(x + 16, filaY, kAnchoFicha - 32, 7, 1.0f, Color{ 130, 150, 230, 255 }, Color{ 40, 40, 55, 255 });
        std::snprintf(textoXp, sizeof(textoXp), "XP: nivel maximo");
    }
    DrawText(textoXp, x + 16, filaY + 10, 11, Color{ 170, 170, 190, 255 });
    filaY += 32;

    // --- Ataque/Defensa/Velocidad: hasta esta pantalla, ningun lado del
    // juego los mostraba como numero (solo se notaban indirectamente en el
    // resultado del combate). ---
    DrawText("Stats", x + 16, filaY, 12, Color{ 150, 150, 160, 255 });
    filaY += 16;
    char textoStats[64];
    std::snprintf(textoStats, sizeof(textoStats), "Ataque %d    Defensa %d    Vel %.0f",
                  stats.ataque, stats.defensa, stats.velocidad);
    DrawText(textoStats, x + 16, filaY, 13, RAYWHITE);
    filaY += 26;

    // --- Equipo: una ranura de Arma y una de Accesorio por personaje (ver
    // Character::Equipar) — "-" cuando esta vacia. ---
    Color colorEquipo{ 190, 190, 160, 255 };
    const auto& arma = personaje.Arma();
    char lineaArma[64];
    std::snprintf(lineaArma, sizeof(lineaArma), "Arma: %s", arma.ocupado ? arma.item.nombre.c_str() : "-");
    DrawText(lineaArma, x + 16, filaY, 12, colorEquipo);
    filaY += 16;

    const auto& accesorio = personaje.Accesorio();
    char lineaAccesorio[64];
    std::snprintf(lineaAccesorio, sizeof(lineaAccesorio), "Accesorio: %s", accesorio.ocupado ? accesorio.item.nombre.c_str() : "-");
    DrawText(lineaAccesorio, x + 16, filaY, 12, colorEquipo);
}

} // namespace

void DibujarFichaPersonajes(const game::Party& party, const render::SpriteSet& sprites) {
    int anchoVentana = GetScreenWidth();
    int altoVentana = GetScreenHeight();

    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 10, 8, 15, 235 });

    DrawText("Personajes", 40, 32, 28, RAYWHITE);

    const auto& miembros = party.Miembros();
    int xFicha = 40;
    int yFicha = 90;
    for (size_t i = 0; i < miembros.size(); ++i) {
        DibujarFicha(miembros[i], i == 0, xFicha, yFicha, sprites);
        xFicha += kAnchoFicha + kEspacioEntreFichas;
    }

    DrawText("[TAB] o [ESC] Cerrar", 40, yFicha + kAltoFicha + 20, 16, Color{ 180, 180, 190, 255 });
}

} // namespace ui
