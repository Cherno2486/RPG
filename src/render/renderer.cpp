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

Color ColorDeEnemigo(game::TipoEnemigo tipo) {
    switch (tipo) {
        case game::TipoEnemigo::EsqueletoErrante: return Color{ 160, 40, 40, 255 };
        case game::TipoEnemigo::RataGigante:      return Color{ 150, 110, 55, 255 };
        case game::TipoEnemigo::BanditoAturdidor: return Color{ 130, 55, 150, 255 };
    }
    return Color{ 160, 40, 40, 255 };
}

float RadioDeEnemigo(game::TipoEnemigo tipo) {
    switch (tipo) {
        case game::TipoEnemigo::EsqueletoErrante: return 16.0f;
        case game::TipoEnemigo::RataGigante:      return 11.0f;  // chica y rapida
        case game::TipoEnemigo::BanditoAturdidor: return 18.0f;  // mas corpulento
    }
    return 16.0f;
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

void Renderer::DibujarEscenarioSinUI(const game::Dungeon& mazmorra, const game::Party& party, const std::vector<game::Enemy>& enemigos) {
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

    // Enemigos vivos en la mazmorra: un marcador por cada uno (color/tamaño
    // segun su tipo), con su nombre arriba para saber que es interactuable.
    for (const auto& enemigo : enemigos) {
        if (enemigo.Vencido()) continue;
        Vector2 posEnemigo = { enemigo.Posicion().x, enemigo.Posicion().y };
        float radio = RadioDeEnemigo(enemigo.Tipo());
        DrawCircleV(posEnemigo, radio, ColorDeEnemigo(enemigo.Tipo()));
        DrawCircleLines((int)posEnemigo.x, (int)posEnemigo.y, radio, BLACK);
        int anchoTexto = MeasureText(enemigo.Nombre().c_str(), 12);
        DrawText(enemigo.Nombre().c_str(), (int)posEnemigo.x - anchoTexto / 2, (int)posEnemigo.y - 32, 12, RAYWHITE);
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

void Renderer::DibujarFrame(const game::Dungeon& mazmorra, const game::Party& party, const std::vector<game::Enemy>& enemigos, bool panelExpandido) {
    BeginDrawing();

    DibujarEscenarioSinUI(mazmorra, party, enemigos);

    ui::DibujarPanelParty(party, panelExpandido);

    bool hayEnemigoCerca = false;
    for (const auto& enemigo : enemigos) {
        if (enemigo.Vencido()) continue;
        float distancia = game::Length(party.Lider().Posicion() - enemigo.Posicion());
        if (distancia < 90.0f) { hayEnemigoCerca = true; break; }
    }
    if (hayEnemigoCerca) {
        const char* texto = "[E] Atacar";
        int anchoTexto = MeasureText(texto, 16);
        DrawText(texto, (anchoVentana_ - anchoTexto) / 2, altoVentana_ - 40, 16, Color{ 255, 235, 180, 255 });
    }

    DrawFPS(anchoVentana_ - 90, 10);

    EndDrawing();
}

} // namespace render
