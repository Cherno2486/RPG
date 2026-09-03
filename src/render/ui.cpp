#include "ui.h"
#include "raylib.h"
#include <cstdio>

namespace ui {

namespace {
// Escala de los retratos chicos del panel — el lienzo nativo del sprite de
// personaje mide render::kCanvasPersonaje px de ancho (20); a esta escala
// el retrato queda mas o menos del mismo diametro que el circulo de radio
// 10 que dibujaba antes.
constexpr float kEscalaRetrato = 1.0f;
constexpr float kEscalaRetratoCompacto = 1.2f;

void DibujarPanelExpandido(const game::Party& party, const render::SpriteSet& sprites) {
    const int panelX = 16;
    const int panelY = 16;
    // Un poco mas ancho que antes (220) para que "Arma / Accesorio" con
    // nombres largos no se corte contra el borde del panel.
    const int anchoPanel = 270;
    // +16 respecto de antes: una linea extra para el equipo (Arma/Accesorio
    // en una sola linea, para no duplicar el alto de fila otra vez).
    const int altoFila = 70;

    const auto& miembros = party.Miembros();
    int altoPanel = 12 + altoFila * (int)miembros.size();
    DrawRectangle(panelX, panelY, anchoPanel, altoPanel, Color{ 20, 20, 25, 220 });
    DrawRectangleLines(panelX, panelY, anchoPanel, altoPanel, Color{ 80, 80, 90, 255 });

    int filaY = panelY + 8;
    for (size_t i = 0; i < miembros.size(); ++i) {
        const auto& personaje = miembros[i];
        const auto& stats = personaje.GetStats();

        render::DibujarSpriteCentrado(sprites.Personaje(personaje.Rol()), Vector2{ (float)(panelX + 20), (float)(filaY + 18) }, kEscalaRetrato);

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

        // Equipo actual (ver Character::Equipar) — "-" si no tiene nada
        // puesto en esa ranura. Se muestra tambien aca (y no solo en el
        // inventario) para poder chusmear el equipo del party sin tener que
        // abrirlo con [I].
        const auto& arma = personaje.Arma();
        const auto& accesorio = personaje.Accesorio();
        char linea3[96];
        std::snprintf(linea3, sizeof(linea3), "%s / %s",
                       arma.ocupado ? arma.item.nombre.c_str() : "-",
                       accesorio.ocupado ? accesorio.item.nombre.c_str() : "-");
        DrawText(linea3, panelX + 40, filaY + 48, 11, Color{ 190, 190, 160, 255 });

        filaY += altoFila;
    }

    DrawText("[TAB] ocultar", panelX + 8, panelY + altoPanel + 6, 12, Color{ 180, 180, 190, 255 });
}

void DibujarPanelCompacto(const game::Party& party, const render::SpriteSet& sprites) {
    const int x = 16;
    const int y = 16;
    const auto& miembros = party.Miembros();
    const int anchoItem = 46;
    int ancho = anchoItem * (int)miembros.size() + 8;

    DrawRectangle(x, y, ancho, 54, Color{ 20, 20, 25, 180 });
    DrawRectangleLines(x, y, ancho, 54, Color{ 80, 80, 90, 200 });

    for (size_t i = 0; i < miembros.size(); ++i) {
        const auto& personaje = miembros[i];
        const auto& stats = personaje.GetStats();
        int cx = x + 8 + (int)i * anchoItem + 14;

        render::DibujarSpriteCentrado(sprites.Personaje(personaje.Rol()), Vector2{ (float)cx, (float)(y + 20) }, kEscalaRetratoCompacto);
        if (!personaje.EstaVivo()) {
            DrawLine(cx - 10, y + 10, cx + 10, y + 30, Color{ 220, 60, 60, 255 });
        }

        float ratio = stats.hpMax > 0 ? (float)stats.hp / (float)stats.hpMax : 0.0f;
        DrawRectangle(cx - 15, y + 36, 30, 5, Color{ 60, 20, 20, 255 });
        DrawRectangle(cx - 15, y + 36, (int)(30 * ratio), 5, Color{ 200, 60, 60, 255 });
    }

    DrawText("[TAB]", x + 8, y + 58, 12, Color{ 180, 180, 190, 255 });
}

} // namespace

void DibujarPanelParty(const game::Party& party, bool expandido, const render::SpriteSet& sprites) {
    if (expandido) {
        DibujarPanelExpandido(party, sprites);
    } else {
        DibujarPanelCompacto(party, sprites);
    }
}

} // namespace ui
