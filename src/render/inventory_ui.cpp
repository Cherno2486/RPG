#include "inventory_ui.h"
#include "raylib.h"
#include <cstdio>

namespace ui {

namespace {

// Mismo criterio que ui.cpp/combat_ui.cpp: el retrato reusa el sprite de
// personaje del mapa a un tamaño chico, en vez de un circulo de color.
constexpr float kEscalaRetrato = 1.0f;

// Alto total de cada ficha: HP/recurso arriba, y las dos lineas de equipo
// (Arma/Accesorio) abajo — asi se ve de un vistazo que tiene puesto cada
// uno, sin tener que adivinar ni ir personaje por personaje.
constexpr int kAltoFicha = 78;

void DibujarFichaObjetivo(const game::Character& personaje, bool esObjetivo, int x, int y, int ancho,
                           const render::SpriteSet& sprites) {
    Color fondo = esObjetivo ? Color{ 45, 45, 30, 230 } : Color{ 20, 20, 25, 200 };
    Color borde = esObjetivo ? Color{ 230, 200, 90, 255 } : Color{ 80, 80, 90, 255 };
    DrawRectangle(x, y, ancho, kAltoFicha, fondo);
    DrawRectangleLines(x, y, ancho, kAltoFicha, borde);

    render::DibujarSpriteCentrado(sprites.Personaje(personaje.Rol()), Vector2{ (float)(x + 18), (float)(y + 18) }, kEscalaRetrato);
    if (!personaje.EstaVivo()) {
        DrawLine(x + 8, y + 8, x + 28, y + 28, Color{ 220, 60, 60, 255 });
    }

    char nombre[48];
    std::snprintf(nombre, sizeof(nombre), "%s%s", personaje.Nombre().c_str(), esObjetivo ? " <" : "");
    DrawText(nombre, x + 36, y + 6, 14, RAYWHITE);

    const auto& stats = personaje.GetStats();
    char linea[48];
    if (stats.recursoMax > 0) {
        std::snprintf(linea, sizeof(linea), "HP %d/%d  R %d/%d", stats.hp, stats.hpMax, stats.recurso, stats.recursoMax);
    } else {
        std::snprintf(linea, sizeof(linea), "HP %d/%d", stats.hp, stats.hpMax);
    }
    DrawText(linea, x + 36, y + 26, 12, LIGHTGRAY);

    // Equipo: una ranura de Arma y una de Accesorio por personaje (ver
    // Character::Equipar) — "-" cuando esta vacia.
    Color colorEquipo{ 190, 190, 160, 255 };
    const auto& arma = personaje.Arma();
    char lineaArma[64];
    std::snprintf(lineaArma, sizeof(lineaArma), "Arma: %s", arma.ocupado ? arma.item.nombre.c_str() : "-");
    DrawText(lineaArma, x + 10, y + 46, 11, colorEquipo);

    const auto& accesorio = personaje.Accesorio();
    char lineaAccesorio[64];
    std::snprintf(lineaAccesorio, sizeof(lineaAccesorio), "Accesorio: %s", accesorio.ocupado ? accesorio.item.nombre.c_str() : "-");
    DrawText(lineaAccesorio, x + 10, y + 62, 11, colorEquipo);
}

} // namespace

void DibujarInventario(const game::Party& party, size_t indiceObjetivo, const render::SpriteSet& sprites) {
    int anchoVentana = GetScreenWidth();
    int altoVentana = GetScreenHeight();

    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 10, 8, 15, 235 });

    const char* titulo = "Inventario";
    DrawText(titulo, 40, 32, 28, RAYWHITE);

    // --- Fila de miembros del party: a quien se le va a aplicar el item ---
    int xFicha = 40;
    int yFicha = 76;
    int anchoFicha = 220;
    const auto& miembros = party.Miembros();
    for (size_t i = 0; i < miembros.size(); ++i) {
        DibujarFichaObjetivo(miembros[i], i == indiceObjetivo, xFicha, yFicha, anchoFicha, sprites);
        xFicha += anchoFicha + 12;
    }

    // --- Lista de items ---
    int xLista = 40;
    int yLista = yFicha + kAltoFicha + 28;
    const auto& pilas = party.Inventario().Pilas();
    if (pilas.empty()) {
        DrawText("No tenes items todavia — buscá cofres y derrotá enemigos.", xLista, yLista, 18, LIGHTGRAY);
    } else {
        for (size_t i = 0; i < pilas.size() && i < 9; ++i) {
            const auto& pila = pilas[i];
            bool esMejora = pila.item.tipo == game::TipoItem::Mejora;
            int y = yLista + (int)i * 46;
            DrawRectangle(xLista, y, anchoVentana - 80, 40, Color{ 22, 22, 28, 220 });
            DrawRectangleLines(xLista, y, anchoVentana - 80, 40, Color{ 80, 80, 90, 255 });

            char etiqueta[16];
            std::snprintf(etiqueta, sizeof(etiqueta), "[%zu]", i + 1);
            DrawText(etiqueta, xLista + 10, y + 11, 18, Color{ 230, 200, 90, 255 });

            // Las Mejoras se equipan, no se "usan": se marcan con un tag de
            // que ranura ocupan, asi el jugador sabe que no se van a gastar
            // sumando sin limite — reemplazan lo que haya en esa ranura.
            char nombreYCantidad[112];
            if (esMejora) {
                const char* ranura = pila.item.ranura == game::RanuraEquipo::Arma ? "Arma" : "Accesorio";
                std::snprintf(nombreYCantidad, sizeof(nombreYCantidad), "%s x%d  [Equipar: %s]",
                              pila.item.nombre.c_str(), pila.cantidad, ranura);
            } else {
                std::snprintf(nombreYCantidad, sizeof(nombreYCantidad), "%s x%d", pila.item.nombre.c_str(), pila.cantidad);
            }
            DrawText(nombreYCantidad, xLista + 56, y + 4, 16, esMejora ? Color{ 210, 170, 90, 255 } : RAYWHITE);
            DrawText(pila.item.descripcion.c_str(), xLista + 56, y + 22, 12, LIGHTGRAY);
        }
    }

    // --- Instrucciones ---
    const char* nombreObjetivo = indiceObjetivo < miembros.size() ? miembros[indiceObjetivo].Nombre().c_str() : "?";
    char instrucciones[160];
    std::snprintf(instrucciones, sizeof(instrucciones),
                  "[1-9] Usar/equipar en %s    [TAB] Cambiar objetivo    [I] Cerrar", nombreObjetivo);
    int anchoTexto = MeasureText(instrucciones, 18);
    DrawText(instrucciones, (anchoVentana - anchoTexto) / 2, altoVentana - 48, 18, Color{ 255, 235, 180, 255 });
}

} // namespace ui
