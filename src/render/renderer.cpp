#include "renderer.h"
#include "ui.h"
#include "raylib.h"
#include <cmath>
#include <cstdint>
#include <unordered_set>

namespace render {

namespace {
Color ColorDePiso()  { return Color{ 40, 38, 45, 255 }; }

// Hash entero determinístico por tile (no criptográfico, solo necesita
// verse disperso) — se usa para decidir, sin guardar ningún estado nuevo en
// la capa de juego, qué tiles de piso llevan una decoración suelta (y
// cuál) y qué tiles de pared "de frente" llevan una antorcha. Determinístico
// en las mismas coordenadas de tile: la mazmorra se ve igual cuadro a
// cuadro, no parpadea ni cambia de decoración sola.
uint32_t HashTile(int x, int y) {
    uint32_t h = (uint32_t)(x * 374761393 + y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Cuantos tiles de piso de cada 100 (dentro de una sala) llevan alguna
// decoración suelta — disperso a propósito, para que se note como "detalle"
// y no como otro patrón repetitivo.
constexpr int kChanceDecoracionPisoDe100 = 12;

// Empaqueta una coordenada de tile (con offset para tolerar negativos, que
// no deberían darse pero cuestan nada de cubrir) en una clave de 64 bits
// para el set de tiles de pared.
int64_t ClaveTile(int tx, int ty) {
    return (int64_t)(tx + 100000) * 1000000LL + (int64_t)(ty + 100000);
}

// Relacion entre un tile de juego (game::kTileSize, 48px) y el lienzo nativo
// de las texturas de piso/pared (kCanvasTile, 16px) — cuanto hay que
// escalar esas texturas para que un tile de textura cubra exactamente un
// tile de juego. La usan tanto el tileado de piso/pared como el escalado de
// personajes/enemigos/cofres (comparten el mismo lienzo base).
constexpr float kEscalaTile = game::kTileSize / (float)kCanvasTile;  // 3.0

// Escala de los sprites de personaje/enemigo comunes y del cofre en el
// mapa — elegidas para que ocupen un tamaño similar al que tenian los
// circulos que reemplazan (radio 14 los personajes, ~16-18 los enemigos
// comunes, mitad=14 el cofre). El jefe usa una escala mayor aparte (ver
// abajo) para seguir notandose mas grande e imponente que el resto, como ya
// pasaba con su radio de 26.
constexpr float kEscalaPersonaje = 1.8f;
constexpr float kEscalaJefe = 2.4f;
constexpr float kEscalaCofre = 1.6f;
constexpr float kEscalaAntorcha = 1.6f;

Color ColorDeGrillaJefe() { return Color{ 230, 190, 80, 255 }; }  // anillo dorado detras del Capitan

// Cofres: sprite de tapa cerrada (con banda dorada) o abierto (tapa
// levantada, interior oscuro a la vista) — mismo criterio que antes (se
// notaba a simple vista si ya no tenia nada adentro), ahora con arte
// pixel-art en vez de un rectangulo liso.
void DibujarCofre(const game::Cofre& cofre, const SpriteSet& sprites) {
    const Texture2D& tex = cofre.abierto ? sprites.CofreAbierto() : sprites.CofreCerrado();
    DibujarSpriteCentrado(tex, Vector2{ cofre.posicion.x, cofre.posicion.y }, kEscalaCofre);
}
} // namespace

Renderer::Renderer(int anchoVentana, int altoVentana, const char* titulo)
    : anchoVentana_(anchoVentana), altoVentana_(altoVentana) {
    InitWindow(anchoVentana_, altoVentana_, titulo);
    SetTargetFPS(60);
    // raylib por defecto usa ESC como "tecla de salida" (hace que
    // WindowShouldClose() de true apenas se aprieta, sin pasar por el loop
    // principal). Lo desactivamos porque ESC ahora tiene un uso propio
    // dentro del juego (volver de la pantalla "Sobre mi" al menu, ver
    // EstadoJuego::SobreMi en main.cpp) — "Salir" en el menu de inicio
    // sigue siendo la unica forma de cerrar el juego.
    SetExitKey(KEY_NULL);

    camara_.offset = Vector2{ anchoVentana_ / 2.0f, altoVentana_ / 2.0f };
    camara_.target = Vector2{ 0.0f, 0.0f };
    camara_.rotation = 0.0f;
    camara_.zoom = 1.0f;

    // Recien aca hay un contexto GL valido (InitWindow ya corrio arriba) —
    // por eso SpriteSet no puede ser un miembro por valor construido antes
    // del cuerpo de este constructor.
    sprites_ = std::make_unique<SpriteSet>();
}

Renderer::~Renderer() {
    // sprites_ tiene que liberar sus texturas GL (UnloadTexture) ANTES de
    // que CloseWindow() cierre el contexto — si no, se libera con el
    // contexto ya invalido y crashea. El orden normal de destruccion de
    // miembros (reverso a la declaracion) no alcanza aca porque los
    // miembros se destruyen DESPUES de que termina el cuerpo de este
    // destructor, no antes; por eso el reset() explicito.
    sprites_.reset();
    CloseWindow();
}

void Renderer::DibujarEscenarioSinUI(const game::Dungeon& mazmorra, const game::Party& party,
                                      const std::vector<game::Enemy>& enemigos, const std::vector<game::Cofre>& cofres) {
    ClearBackground(ColorDePiso());

    // La camara sigue al lider: la mazmorra generada por salas es mas
    // grande que la ventana, asi que sin esto no se veria nada apenas se
    // sale de la sala inicial.
    camara_.target = Vector2{ party.Lider().Posicion().x, party.Lider().Posicion().y };

    BeginMode2D(camara_);

    // Piso tileado, solo dentro de cada sala (los pasillos siguen mostrando
    // el color de fondo liso de ClearBackground, que coincide con el color
    // base del tile — ver ColorDePiso()/CrearTilePiso()).
    for (const auto& sala : mazmorra.Habitaciones()) {
        float x0 = sala.x * game::kTileSize;
        float y0 = sala.y * game::kTileSize;
        float x1 = (sala.x + sala.ancho) * game::kTileSize;
        float y1 = (sala.y + sala.alto) * game::kTileSize;
        DibujarTileado(sprites_->TilePiso(), Rectangle{ x0, y0, x1 - x0, y1 - y0 }, kEscalaTile);

        // Decoracion suelta de piso (grieta/musgo/escombros/charco),
        // disperso por tile via HashTile — se dibuja ANTES que las paredes
        // (mas abajo), asi que si algun tile "cae" sobre una muesca de sala
        // en L o un pilar (que tambien son parte de este bounding box, ver
        // "Variedad de formas de sala"), la pared que se dibuja despues lo
        // tapa sin dejar rastro; no hace falta que este loop sepa distinguir
        // piso real de hueco.
        for (int ty = sala.y; ty < sala.y + sala.alto; ++ty) {
            for (int tx = sala.x; tx < sala.x + sala.ancho; ++tx) {
                uint32_t h = HashTile(tx, ty);
                if ((int)(h % 100) >= kChanceDecoracionPisoDe100) continue;
                int variante = (int)((h / 100) % SpriteSet::kNumDecoracionesPiso);
                Vector2 centro{ (tx + 0.5f) * game::kTileSize, (ty + 0.5f) * game::kTileSize };
                DibujarSpriteCentrado(sprites_->DecoracionPiso(variante), centro, kEscalaTile);
            }
        }
    }

    // Paredes, con textura de ladrillos tileada en vez de un rectangulo
    // liso.
    for (const auto& pared : mazmorra.Paredes()) {
        DibujarTileado(sprites_->TilePared(), Rectangle{ pared.x, pared.y, pared.width, pared.height }, kEscalaTile);
    }

    // Antorchas: solo en tiles de pared "de frente" (el tile de abajo, hacia
    // el jugador, NO es pared — por construccion de Paredes(), ver "Sistema
    // de mazmorras", eso significa que es piso) — tipicamente la fila
    // superior de cada sala, la unica cara de pared que este angulo de
    // camara realmente "mira". Se reparten cada 3 tiles, con un offset por
    // fila derivado del hash para que no quede perfectamente alineado entre
    // salas distintas.
    {
        std::unordered_set<int64_t> tilesDePared;
        tilesDePared.reserve(mazmorra.Paredes().size() * 2);
        for (const auto& pared : mazmorra.Paredes()) {
            int tx = (int)std::lround(pared.x / game::kTileSize);
            int ty = (int)std::lround(pared.y / game::kTileSize);
            tilesDePared.insert(ClaveTile(tx, ty));
        }

        float tiempo = (float)GetTime();
        for (const auto& pared : mazmorra.Paredes()) {
            int tx = (int)std::lround(pared.x / game::kTileSize);
            int ty = (int)std::lround(pared.y / game::kTileSize);
            if (tilesDePared.count(ClaveTile(tx, ty + 1)) != 0) continue;  // no es pared "de frente"

            uint32_t h = HashTile(tx, ty);
            int offsetFila = (int)(HashTile(0, ty) % 3);
            if (((tx + offsetFila) % 3 + 3) % 3 != 0) continue;

            float fase = (float)(h % 628) / 100.0f;  // 0..~2π, distinto por antorcha
            float escala = kEscalaAntorcha * (1.0f + 0.06f * sinf(tiempo * 6.0f + fase));
            Vector2 posicionPies{ (tx + 0.5f) * game::kTileSize, (ty + 1) * game::kTileSize };
            DibujarSpritePlantado(sprites_->Antorcha(), posicionPies, escala);
        }
    }

    // Cofres
    for (const auto& cofre : cofres) {
        DibujarCofre(cofre, *sprites_);
    }

    // Enemigos vivos en la mazmorra: su sprite pixel-art segun tipo, con su
    // nombre arriba para saber que es interactuable.
    for (const auto& enemigo : enemigos) {
        if (enemigo.Vencido()) continue;
        Vector2 posEnemigo = { enemigo.Posicion().x, enemigo.Posicion().y };
        bool esJefe = enemigo.Tipo() == game::TipoEnemigo::CapitanBandido;
        float escala = esJefe ? kEscalaJefe : kEscalaPersonaje;
        float alturaSprite = kCanvasPersonajeAlto * escala;

        // El jefe lleva un anillo dorado detras del sprite (mismo recurso
        // visual que antes tenia como contorno de su circulo), para que se
        // note a simple vista que es distinto apenas se lo ve.
        if (esJefe) {
            Vector2 centroAnillo{ posEnemigo.x, posEnemigo.y - alturaSprite * 0.5f };
            DrawCircleLines((int)centroAnillo.x, (int)centroAnillo.y, kCanvasPersonaje * escala * 0.6f, ColorDeGrillaJefe());
        }
        DibujarSpritePlantado(sprites_->Enemigo(enemigo.Tipo()), posEnemigo, escala);

        int anchoTexto = MeasureText(enemigo.Nombre().c_str(), 12);
        DrawText(enemigo.Nombre().c_str(), (int)posEnemigo.x - anchoTexto / 2, (int)(posEnemigo.y - alturaSprite - 14), 12, RAYWHITE);
    }

    // Party: se dibuja del ultimo al primero para que el lider quede arriba de los demas
    const auto& miembros = party.Miembros();
    for (size_t i = miembros.size(); i-- > 0; ) {
        const auto& personaje = miembros[i];
        Vector2 pos = { personaje.Posicion().x, personaje.Posicion().y };
        DibujarSpritePlantado(sprites_->Personaje(personaje.Rol()), pos, kEscalaPersonaje);
    }

    EndMode2D();
}

void Renderer::DibujarFrame(const game::Dungeon& mazmorra, const game::Party& party,
                             const std::vector<game::Enemy>& enemigos, const std::vector<game::Cofre>& cofres,
                             bool panelExpandido, const std::string& promptInteraccion, const std::string& mensajeFlotante) {
    BeginDrawing();

    DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres);

    ui::DibujarPanelParty(party, panelExpandido, *sprites_);

    if (!mensajeFlotante.empty()) {
        int anchoTexto = MeasureText(mensajeFlotante.c_str(), 18);
        int x = (anchoVentana_ - anchoTexto) / 2;
        int y = altoVentana_ - 68;
        DrawRectangle(x - 12, y - 6, anchoTexto + 24, 30, Color{ 30, 28, 20, 210 });
        DrawText(mensajeFlotante.c_str(), x, y, 18, Color{ 230, 210, 140, 255 });
    }

    if (!promptInteraccion.empty()) {
        int anchoTexto = MeasureText(promptInteraccion.c_str(), 16);
        DrawText(promptInteraccion.c_str(), (anchoVentana_ - anchoTexto) / 2, altoVentana_ - 40, 16, Color{ 255, 235, 180, 255 });
    }

    DrawFPS(anchoVentana_ - 90, 10);

    // Hint fijo de guardado — F5 funciona en toda la exploracion (ver
    // main.cpp), asi que el recordatorio tambien es fijo, no depende de
    // estar cerca de nada como el prompt de interaccion.
    DrawText("[F5] Guardar", 16, altoVentana_ - 24, 14, Color{ 150, 150, 160, 255 });

    EndDrawing();
}

} // namespace render
