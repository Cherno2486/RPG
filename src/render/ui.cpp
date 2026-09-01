#include "ui.h"
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
} // namespace

void DibujarPanelParty(const game::Party& party) {
    const int panelX = 16;
    const int panelY = 16;
    const int anchoPanel = 220;
    const int altoFila = 54;

    const auto& miembros = party.Miembros();
    int altoPanel = 12 + altoFila * (int)miembros.size();
    DrawRectangle(panelX, panelY, anchoPanel, altoPanel, Color{ 20, 20, 25, 200 });
    DrawRectangleLines(panelX, panelY, anchoPanel, altoPanel, Color{ 80, 80, 90, 255 });

    int filaY = panelY + 8;
    for (size_t i = 0; i < miembros.size(); ++i) {
        const auto& personaje = miembros[i];
        const auto& stats = personaje.GetStats();

        DrawCircle(panelX + 20, filaY + 18, 10.0f, ColorDeRol(personaje.Rol()));

        char linea1[64];
        std::snprintf(linea1, sizeof(linea1), "%s%s",
                       personaje.Nombre().c_str(),
                       i == 0 ? " (lider)" : "");
        DrawText(linea1, panelX + 40, filaY, 16, RAYWHITE);

        char linea2[64];
        std::snprintf(linea2, sizeof(linea2), "%s  HP %d/%d",
                       game::RoleName(personaje.Rol()), stats.hp, stats.hpMax);
        DrawText(linea2, panelX + 40, filaY + 18, 14, LIGHTGRAY);

        float ratio = stats.hpMax > 0 ? (float)stats.hp / (float)stats.hpMax : 0.0f;
        DrawRectangle(panelX + 40, filaY + 36, 150, 6, Color{ 60, 20, 20, 255 });
        DrawRectangle(panelX + 40, filaY + 36, (int)(150 * ratio), 6, Color{ 200, 60, 60, 255 });

        filaY += altoFila;
    }
}

} // namespace ui
