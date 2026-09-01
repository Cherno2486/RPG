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

void Renderer::DibujarEscenarioSinUI(const game::Dungeon& mazmorra, const game::Party& party, const game::Enemy* enemigo) {
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

    // Enemigo (si hay uno vivo en la mazmorra): un marcador simple, distinto
    // de los del party, con su nombre arriba para saber que es interactuable.
    if (enemigo != nullptr && !enemigo->Vencido()) {
        Vector2 posEnemigo = { enemigo->Posicion().x, enemigo->Posicion().y };
        DrawCircleV(posEnemigo, 16.0f, Color{ 160, 40, 40, 255 });
        DrawCircleLines((int)posEnemigo.x, (int)posEnemigo.y, 16.0f, BLACK);
        int anchoTexto = MeasureText(enemigo->Nombre().c_str(), 12);
        DrawText(enemigo->Nombre().c_str(), (int)posEnemigo.x - anchoTexto / 2, (int)posEnemigo.y - 32, 12, RAYWHITE);
    }

    // Party: se dibuja del ultimo al primero para que el lider quede arriba de los demas
    const auto& miembros = party.Miembros();
    for (size_t i = miembros.size(); i-- > 0; ) {
        const auto& personaje = miembros[i];
        Vector2 pos = { personaje.Posicion().x, personaje.Posicion().y };
        DrawCircleV(pos, 14.0f, ColorDeRol(personaje.Rol()));
        DrawCircleLines((int)pos.x, (int)pos.y, 14.0f, BLACK);
    }
}

void Renderer::DibujarFrame(const game::Dungeon& mazmorra, const game::Party& party, const game::Enemy* enemigo, bool panelExpandido) {
    BeginDrawing();

    DibujarEscenarioSinUI(mazmorra, party, enemigo);

    ui::DibujarPanelParty(party, panelExpandido);

    if (enemigo != nullptr && !enemigo->Vencido()) {
        float distancia = game::Length(party.Lider().Posicion() - enemigo->Posicion());
        if (distancia < 90.0f) {
            const char* texto = "[E] Atacar";
            int anchoTexto = MeasureText(texto, 16);
            DrawText(texto, (anchoVentana_ - anchoTexto) / 2, altoVentana_ - 40, 16, Color{ 255, 235, 180, 255 });
        }
    }

    DrawFPS(anchoVentana_ - 90, 10);

    EndDrawing();
}

} // namespace render
