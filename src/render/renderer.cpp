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

    camara_.offset = Vector2{ anchoVentana_ / 2.0f, altoVentana_ / 2.0f };
    camara_.target = Vector2{ 0.0f, 0.0f };
    camara_.rotation = 0.0f;
    camara_.zoom = 1.0f;
}

Renderer::~Renderer() {
    CloseWindow();
}

void Renderer::DibujarEscenarioSinUI(const game::Dungeon& mazmorra, const game::Party& party, const std::vector<game::Enemy>& enemigos) {
    ClearBackground(ColorDePiso());

    // La camara sigue al lider: la mazmorra generada por salas es mas
    // grande que la ventana, asi que sin esto no se veria nada apenas se
    // sale de la sala inicial.
    camara_.target = Vector2{ party.Lider().Posicion().x, party.Lider().Posicion().y };

    BeginMode2D(camara_);

    // Grilla de tiles de referencia (solo dentro de cada sala, no en los
    // pasillos ni fuera del layout — es solo un apoyo visual).
    for (const auto& sala : mazmorra.Habitaciones()) {
        int x0 = sala.x * (int)game::kTileSize;
        int y0 = sala.y * (int)game::kTileSize;
        int x1 = (sala.x + sala.ancho) * (int)game::kTileSize;
        int y1 = (sala.y + sala.alto) * (int)game::kTileSize;
        for (int x = sala.x; x <= sala.x + sala.ancho; ++x) {
            int px = x * (int)game::kTileSize;
            DrawLine(px, y0, px, y1, ColorDeGrilla());
        }
        for (int y = sala.y; y <= sala.y + sala.alto; ++y) {
            int py = y * (int)game::kTileSize;
            DrawLine(x0, py, x1, py, ColorDeGrilla());
        }
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

    EndMode2D();
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
