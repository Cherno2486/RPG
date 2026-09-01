#include "dungeon.h"
#include "dice.h"
#include <set>
#include <utility>
#include <algorithm>

namespace game {

namespace {

struct RoomTemplate {
    int ancho;
    int alto;
};

// Los "room templates" de los que se arma la mazmorra: un par de salas
// chicas/cuadradas y una alargada, para que el layout generado no sea
// siempre del mismo tamaño. Ver docs/design.md.
constexpr RoomTemplate kTemplates[] = {
    {8, 8},    // chica
    {14, 10},  // grande
    {6, 14},   // alargada (vertical)
    {10, 10},  // mediana
};
constexpr int kNumTemplates = 4;

constexpr int kNumSalas = 5;       // 1 inicial + 4 con contenido
constexpr int kAnchoPasillo = 3;   // en tiles

} // namespace

Dungeon::Dungeon() {
    // Tiles de piso (sala + pasillos), en coordenadas de tile. Se arma
    // primero el set de piso completo, y recien al final se calculan las
    // paredes: cualquier tile del bounding box que no sea piso es pared.
    // Esto evita tener que calcular "aberturas" a mano donde un pasillo
    // conecta con una sala — al ser piso de los dos lados, ya no hay pared
    // en el medio.
    std::set<std::pair<int, int>> piso;

    int cursorX = 0, cursorY = 0;      // esquina superior izquierda de la sala anterior
    int prevAncho = 0, prevAlto = 0;

    for (int i = 0; i < kNumSalas; ++i) {
        const RoomTemplate& t = kTemplates[Roll(kNumTemplates) - 1];
        int rx, ry;

        if (i == 0) {
            rx = 0;
            ry = 0;
        } else if (Roll(2) == 1) {
            // Se extiende hacia el Este: pasillo horizontal, centrado en el
            // solape vertical entre la sala anterior y la nueva.
            rx = cursorX + prevAncho + kAnchoPasillo;
            ry = cursorY + (prevAlto - t.alto) / 2;
            int corridorY = cursorY + prevAlto / 2 - kAnchoPasillo / 2;
            for (int cx = cursorX + prevAncho; cx < rx; ++cx) {
                for (int cy = corridorY; cy < corridorY + kAnchoPasillo; ++cy) {
                    piso.insert({cx, cy});
                }
            }
        } else {
            // Se extiende hacia el Sur: pasillo vertical, centrado en el
            // solape horizontal.
            ry = cursorY + prevAlto + kAnchoPasillo;
            rx = cursorX + (prevAncho - t.ancho) / 2;
            int corridorX = cursorX + prevAncho / 2 - kAnchoPasillo / 2;
            for (int cy = cursorY + prevAlto; cy < ry; ++cy) {
                for (int cx = corridorX; cx < corridorX + kAnchoPasillo; ++cx) {
                    piso.insert({cx, cy});
                }
            }
        }

        for (int x = rx; x < rx + t.ancho; ++x) {
            for (int y = ry; y < ry + t.alto; ++y) {
                piso.insert({x, y});
            }
        }

        habitaciones_.push_back(Habitacion{rx, ry, t.ancho, t.alto});
        cursorX = rx;
        cursorY = ry;
        prevAncho = t.ancho;
        prevAlto = t.alto;
    }

    // Bounding box de todo el layout generado, con 1 tile de margen para
    // poder cerrar las paredes exteriores.
    int minX = 0, minY = 0, maxX = 0, maxY = 0;
    for (const auto& h : habitaciones_) {
        minX = std::min(minX, h.x);
        minY = std::min(minY, h.y);
        maxX = std::max(maxX, h.x + h.ancho);
        maxY = std::max(maxY, h.y + h.alto);
    }

    // Una pared (rect de 1 tile) por cada celda del bounding box que no sea
    // piso. Es mas pared de la estrictamente necesaria (podrian mezclarse
    // en rects mas grandes), pero a esta escala (unas pocas salas) el
    // costo de colision es insignificante y esto es mucho mas simple y
    // dificil de romper que mezclar rects.
    for (int y = minY - 1; y <= maxY; ++y) {
        for (int x = minX - 1; x <= maxX; ++x) {
            if (piso.count({x, y}) > 0) continue;
            paredes_.push_back(Rect{
                x * kTileSize, y * kTileSize, kTileSize, kTileSize
            });
        }
    }
}

Vec2 Dungeon::CentroDeSala(size_t indice) const {
    if (indice >= habitaciones_.size()) return Vec2{0.0f, 0.0f};
    const Habitacion& h = habitaciones_[indice];
    return Vec2{
        (h.x + h.ancho / 2.0f) * kTileSize,
        (h.y + h.alto / 2.0f) * kTileSize
    };
}

Vec2 Dungeon::ResolverColision(Rect colisionadorActual, Vec2 posicionActual, Vec2 posicionDeseada) const {
    Vec2 resultado = posicionActual;

    // --- Eje X ---
    Vec2 intentoX = { posicionDeseada.x, posicionActual.y };
    Rect rectX = colisionadorActual;
    rectX.x += (intentoX.x - posicionActual.x);
    bool bloqueadoX = false;
    for (const auto& pared : paredes_) {
        if (CheckCollision(rectX, pared)) { bloqueadoX = true; break; }
    }
    if (!bloqueadoX) resultado.x = intentoX.x;

    // --- Eje Y (ya con el resultado de X aplicado) ---
    Vec2 intentoY = { resultado.x, posicionDeseada.y };
    Rect rectY = colisionadorActual;
    rectY.x += (resultado.x - posicionActual.x);
    rectY.y += (intentoY.y - posicionActual.y);
    bool bloqueadoY = false;
    for (const auto& pared : paredes_) {
        if (CheckCollision(rectY, pared)) { bloqueadoY = true; break; }
    }
    if (!bloqueadoY) resultado.y = intentoY.y;

    return resultado;
}

} // namespace game
