#include "renderer.h"
#include "ui.h"
#include "raylib.h"

namespace render {

namespace {
Color ColorDePared() { return Color{ 90, 70, 60, 255 }; }
Color ColorDePiso()  { return Color{ 40, 38, 45, 255 }; }
Color ColorDeGrilla(){ return Color{ 55, 53, 60, 255 }; }

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

Renderer::Renderer(int anchoVentana, int altoVentana, const char* titulo)
    : anchoVentana_(anchoVentana), altoVentana_(altoVentana) {
    InitWindow(anchoVentana_, altoVentana_, titulo);
    SetTargetFPS(60);
}

Renderer::~Renderer() {
    CloseWindow();
}

void Renderer::DibujarFrame(const game::Dungeon& mazmorra, const game::Party& party) {
    BeginDrawing();
    ClearBackground(ColorDePiso());

    // Grilla de tiles de referencia (el movimiento es libre, esto es solo visual)
    int anchoPx = mazmorra.AnchoTiles() * (int)game::kTileSize;
    int altoPx = mazmorra.AltoTiles() * (int)game::kTileSize;
    for (int x = 0; x <= mazmorra.AnchoTiles(); ++x) {
        int px = x * (int)game::kTileSize;
        DrawLine(px, 0, px, altoPx, ColorDeGrilla());
    }
    for (int y = 0; y <= mazmorra.AltoTiles(); ++y) {
        int py = y * (int)game::kTileSize;
        DrawLine(0, py, anchoPx, py, ColorDeGrilla());
    }

    // Paredes
    for (const auto& pared : mazmorra.Paredes()) {
        DrawRectangle((int)pared.x, (int)pared.y, (int)pared.width, (int)pared.height, ColorDePared());
    }

    // Party: se dibuja del ultimo al primero para que el lider quede arriba de los demas
    const auto& miembros = party.Miembros();
    for (size_t i = miembros.size(); i-- > 0; ) {
        const auto& personaje = miembros[i];
        Vector2 pos = { personaje.Posicion().x, personaje.Posicion().y };
        DrawCircleV(pos, 14.0f, ColorDeRol(personaje.Rol()));
        DrawCircleLines((int)pos.x, (int)pos.y, 14.0f, BLACK);
    }

    ui::DibujarPanelParty(party);

    DrawFPS(anchoVentana_ - 90, 10);

    EndDrawing();
}

} // namespace render
