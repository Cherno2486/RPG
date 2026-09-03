#include "menu_ui.h"
#include "raylib.h"
#include <cstdio>

namespace ui {

namespace {
constexpr const char* kOpciones[kNumOpcionesMenuInicio] = { "Jugar", "Salir" };
}

void DibujarMenuInicio(int anchoVentana, int altoVentana, int opcionSeleccionada) {
    // Overlay oscuro sobre la mazmorra ya dibujada de fondo (mismo recurso
    // visual que usa la pantalla de combate para "congelar" la escena).
    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 8, 8, 14, 225 });

    const char* titulo = "RPG MAZMORRAS";
    int tamanoTitulo = 56;
    int anchoTitulo = MeasureText(titulo, tamanoTitulo);
    int yTitulo = altoVentana / 2 - 170;
    DrawText(titulo, (anchoVentana - anchoTitulo) / 2, yTitulo, tamanoTitulo, Color{ 230, 190, 80, 255 });

    const char* subtitulo = "prototipo";
    int anchoSub = MeasureText(subtitulo, 18);
    DrawText(subtitulo, (anchoVentana - anchoSub) / 2, yTitulo + tamanoTitulo + 8, 18, Color{ 160, 160, 170, 255 });

    // Mismo tamano de fuente para las dos opciones (solo cambia el color y
    // el prefijo "> ") para que no salten de posicion al mover la
    // seleccion — un tamano distinto por seleccion haria que el texto
    // centrado se corra de lugar cada vez que el jugador aprieta una flecha.
    int tamanoOpcion = 28;
    int yOpciones = altoVentana / 2 + 10;
    for (int i = 0; i < kNumOpcionesMenuInicio; ++i) {
        bool esSeleccionada = (i == opcionSeleccionada);
        char texto[32];
        std::snprintf(texto, sizeof(texto), "%s%s", esSeleccionada ? "> " : "  ", kOpciones[i]);
        Color color = esSeleccionada ? Color{ 255, 235, 180, 255 } : Color{ 140, 140, 150, 255 };
        int ancho = MeasureText(texto, tamanoOpcion);
        DrawText(texto, (anchoVentana - ancho) / 2, yOpciones, tamanoOpcion, color);
        yOpciones += 46;
    }

    const char* prompt = "[flechas o W/S] moverse    [ENTER] confirmar";
    int anchoPrompt = MeasureText(prompt, 16);
    DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana - 60, 16, Color{ 130, 130, 140, 255 });
}

}  // namespace ui
