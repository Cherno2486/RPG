#include "menu_ui.h"
#include "raylib.h"
#include <cstdio>

namespace ui {

namespace {
constexpr const char* kNombresOpciones[kNumOpcionesMenuInicio] = {
    "Nueva partida", "Cargar", "Sobre mi", "Salir"
};

constexpr const char* kNombresOpcionesPausa[kNumOpcionesPausa] = {
    "Continuar", "Guardar", "Menu principal", "Salir"
};

// El titulo va siempre en el mismo lugar en las dos pantallas (menu y
// "Sobre mi") para que la transicion entre las dos no salte.
constexpr int kTamanoTitulo = 56;

void DibujarTitulo(int anchoVentana, int altoVentana) {
    const char* titulo = "RPG MAZMORRAS";
    int anchoTitulo = MeasureText(titulo, kTamanoTitulo);
    int yTitulo = altoVentana / 2 - 210;
    DrawText(titulo, (anchoVentana - anchoTitulo) / 2, yTitulo, kTamanoTitulo, Color{ 230, 190, 80, 255 });

    const char* subtitulo = "prototipo";
    int anchoSub = MeasureText(subtitulo, 18);
    DrawText(subtitulo, (anchoVentana - anchoSub) / 2, yTitulo + kTamanoTitulo + 8, 18, Color{ 160, 160, 170, 255 });
}
}  // namespace

void DibujarMenuInicio(int anchoVentana, int altoVentana, int opcionSeleccionada, bool hayPartidaGuardada) {
    // Overlay oscuro sobre la mazmorra ya dibujada de fondo (mismo recurso
    // visual que usa la pantalla de combate para "congelar" la escena).
    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 8, 8, 14, 225 });

    DibujarTitulo(anchoVentana, altoVentana);

    // Mismo tamano de fuente para todas las opciones (solo cambia el color y
    // el prefijo "> ") para que no salten de posicion al mover la
    // seleccion — un tamano distinto por seleccion haria que el texto
    // centrado se corra de lugar cada vez que el jugador aprieta una flecha.
    int tamanoOpcion = 28;
    int yOpciones = altoVentana / 2 - 30;
    for (int i = 0; i < kNumOpcionesMenuInicio; ++i) {
        bool esSeleccionada = (i == opcionSeleccionada);
        bool esCargarDeshabilitada = (static_cast<OpcionMenuInicio>(i) == OpcionMenuInicio::Cargar) && !hayPartidaGuardada;

        char texto[32];
        std::snprintf(texto, sizeof(texto), "%s%s", esSeleccionada ? "> " : "  ", kNombresOpciones[i]);

        Color color;
        if (esCargarDeshabilitada) {
            // Atenuada siempre, este o no seleccionada — el cursor puede
            // pararse ahi (por eso todavia lleva el prefijo "> "), pero el
            // texto en si nunca llega al dorado de una opcion activa, para
            // que se note a simple vista que no hace nada si se confirma.
            color = Color{ 95, 90, 80, 255 };
        } else if (esSeleccionada) {
            color = Color{ 255, 235, 180, 255 };
        } else {
            color = Color{ 140, 140, 150, 255 };
        }

        int ancho = MeasureText(texto, tamanoOpcion);
        DrawText(texto, (anchoVentana - ancho) / 2, yOpciones, tamanoOpcion, color);

        // Aclaracion chica debajo, solo mientras el cursor esta parado en
        // "Cargar" sin partida guardada — explica por que ENTER no hace
        // nada ahi en vez de dejar al jugador adivinando.
        if (esCargarDeshabilitada && esSeleccionada) {
            const char* aclaracion = "(no hay partida guardada)";
            int anchoAclaracion = MeasureText(aclaracion, 14);
            DrawText(aclaracion, (anchoVentana - anchoAclaracion) / 2, yOpciones + tamanoOpcion + 2, 14,
                     Color{ 120, 115, 100, 255 });
        }

        yOpciones += 50;
    }

    const char* prompt = "[flechas o W/S] moverse    [ENTER] confirmar";
    int anchoPrompt = MeasureText(prompt, 16);
    DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana - 60, 16, Color{ 130, 130, 140, 255 });
}

void DibujarSobreMi(int anchoVentana, int altoVentana) {
    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 8, 8, 14, 225 });

    DibujarTitulo(anchoVentana, altoVentana);

    const char* subtitulo = "Sobre mi";
    int anchoSub = MeasureText(subtitulo, 26);
    DrawText(subtitulo, (anchoVentana - anchoSub) / 2, altoVentana / 2 - 40, 26, Color{ 230, 190, 80, 255 });

    const char* cuerpo = "(proximamente)";
    int anchoCuerpo = MeasureText(cuerpo, 18);
    DrawText(cuerpo, (anchoVentana - anchoCuerpo) / 2, altoVentana / 2, 18, Color{ 160, 160, 170, 255 });

    const char* prompt = "[ESC o ENTER] Volver";
    int anchoPrompt = MeasureText(prompt, 16);
    DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana - 60, 16, Color{ 130, 130, 140, 255 });
}

void DibujarPausa(int anchoVentana, int altoVentana, int opcionSeleccionada, const std::string& mensajeGuardado) {
    // Mismo overlay que las otras pantallas de este archivo — la exploracion
    // ya dibujada de fondo se ve "congelada" detras.
    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 8, 8, 14, 225 });

    DibujarTitulo(anchoVentana, altoVentana);

    const char* subtitulo = "Pausa";
    int anchoSub = MeasureText(subtitulo, 22);
    DrawText(subtitulo, (anchoVentana - anchoSub) / 2, altoVentana / 2 - 90, 22, Color{ 230, 190, 80, 255 });

    // Mismo criterio que DibujarMenuInicio: tamano fijo por opcion, solo
    // cambia el color y el prefijo "> ", para que la seleccion no haga
    // saltar el texto centrado.
    int tamanoOpcion = 28;
    int yOpciones = altoVentana / 2 - 30;
    for (int i = 0; i < kNumOpcionesPausa; ++i) {
        bool esSeleccionada = (i == opcionSeleccionada);

        char texto[32];
        std::snprintf(texto, sizeof(texto), "%s%s", esSeleccionada ? "> " : "  ", kNombresOpcionesPausa[i]);

        Color color = esSeleccionada ? Color{ 255, 235, 180, 255 } : Color{ 140, 140, 150, 255 };

        int ancho = MeasureText(texto, tamanoOpcion);
        DrawText(texto, (anchoVentana - ancho) / 2, yOpciones, tamanoOpcion, color);

        yOpciones += 50;
    }

    // Resultado de la ultima vez que se eligio "Guardar" en esta pausa
    // (p.ej. "Partida guardada.") — vacio si todavia no se guardo nada
    // desde que se abrio esta pantalla. main.cpp decide cuando ponerlo y
    // cuando limpiarlo (ver EstadoJuego::Pausa).
    if (!mensajeGuardado.empty()) {
        int anchoMsg = MeasureText(mensajeGuardado.c_str(), 16);
        DrawText(mensajeGuardado.c_str(), (anchoVentana - anchoMsg) / 2, yOpciones + 10, 16,
                 Color{ 200, 200, 160, 255 });
    }

    const char* prompt = "[flechas o W/S] moverse    [ENTER] confirmar    [ESC] continuar";
    int anchoPrompt = MeasureText(prompt, 16);
    DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana - 60, 16, Color{ 130, 130, 140, 255 });
}

}  // namespace ui
