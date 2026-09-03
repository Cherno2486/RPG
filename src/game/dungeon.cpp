#include "dungeon.h"
#include "dice.h"
#include <set>
#include <utility>
#include <algorithm>

namespace game {

namespace {

// Ademas del rectangulo simple de siempre, dos formas con "recortes"
// interiores: L (un rectangulo con una esquina faltante) y con pilares (un
// rectangulo con un par de obstaculos 2x2 adentro). Se implementan como
// recortes sobre el set de tiles de piso -- no como un tipo de dato
// distinto -- para no tener que tocar Habitacion (sigue siendo el
// bounding box de siempre, que es lo que usan CentroDeSala, la camara, la
// grilla y las paredes) ni ningun otro lugar del codigo que ya asume que
// una sala es "su rectangulo".
enum class FormaSala { Rectangular, LForma, ConPilares };

struct RoomTemplate {
    int ancho;
    int alto;
    FormaSala forma = FormaSala::Rectangular;
    int muescaAncho = 0;  // solo FormaSala::LForma
    int muescaAlto = 0;   // solo FormaSala::LForma
};

// Los "room templates" de los que se arma la mazmorra, para que el layout
// generado no sea siempre del mismo tamaño ni la misma forma. Ver
// docs/design.md, "Sistema de mazmorras".
//
// Las dos formas no rectangulares (LForma, ConPilares) recortan una parte
// del rectangulo, pero SIEMPRE dejan dos zonas garantizadas como piso: la
// esquina superior izquierda (ahi puede ir el cofre "de esquina" de la
// sala, ver CrearCofreEnEsquina en main.cpp) y un margen generoso alrededor
// del centro geometrico del bounding box (ahi arranca el grupo de
// enemigos, ver CrearGrupoDeSala en main.cpp, con un spread de hasta ~1.5
// tiles) -- ver GenerarTilesDeSala mas abajo para el detalle de por que
// las medidas elegidas cumplen las dos garantias.
constexpr RoomTemplate kTemplates[] = {
    {8, 8},                                     // chica
    {14, 10},                                   // grande
    {6, 14},                                    // alargada (vertical)
    {10, 10},                                   // mediana
    {14, 14, FormaSala::LForma, 4, 4},           // L grande (esquina inferior derecha recortada)
    {12, 12, FormaSala::ConPilares},             // con pilares
};
constexpr int kNumTemplates = 6;

constexpr int kNumSalas = 5;       // 1 inicial + 4 con contenido
constexpr int kAnchoPasillo = 3;   // en tiles

// True si la celda local (lx, ly) -- relativa a la esquina superior
// izquierda de una sala ConPilares de 'ancho' x 'alto' -- cae dentro de uno
// de los 4 pilares 2x2. Los 4 quedan simetricos y bien adentro del
// rectangulo (nunca tocan un borde ni la esquina superior izquierda), asi
// que un enemigo en el centro o un cofre en la esquina nunca caen arriba
// de uno, y tampoco puede haber un pasillo que los toque (los pasillos se
// conectan siempre centrados en un borde, nunca cerca de una esquina).
bool EsCeldaDePilar(int lx, int ly, int ancho, int alto) {
    int col1 = ancho / 4;
    int col2 = ancho - ancho / 4 - 1;
    int fil1 = alto / 4;
    int fil2 = alto - alto / 4 - 1;
    bool enColumna = (lx == col1 || lx == col1 + 1 || lx == col2 || lx == col2 + 1);
    bool enFila = (ly == fil1 || ly == fil1 + 1 || ly == fil2 || ly == fil2 + 1);
    return enColumna && enFila;
}

// Inserta en 'piso' los tiles de la sala 't' ubicada en (rx, ry), con el
// recorte que corresponda segun su forma (ver comentario de kTemplates).
void GenerarTilesDeSala(std::set<std::pair<int, int>>& piso, const RoomTemplate& t, int rx, int ry) {
    for (int x = rx; x < rx + t.ancho; ++x) {
        for (int y = ry; y < ry + t.alto; ++y) {
            if (t.forma == FormaSala::LForma) {
                bool enMuescaX = x >= rx + t.ancho - t.muescaAncho;
                bool enMuescaY = y >= ry + t.alto - t.muescaAlto;
                if (enMuescaX && enMuescaY) continue;  // esquina recortada: no es piso
            } else if (t.forma == FormaSala::ConPilares) {
                if (EsCeldaDePilar(x - rx, y - ry, t.ancho, t.alto)) continue;
            }
            piso.insert({x, y});
        }
    }
}

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

        GenerarTilesDeSala(piso, t, rx, ry);

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

Dungeon::Dungeon(std::vector<Habitacion> habitaciones, std::vector<Rect> paredes)
    : habitaciones_(std::move(habitaciones)), paredes_(std::move(paredes)) {}

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
