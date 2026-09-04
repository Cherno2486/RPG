#include "menu_ui.h"
#include "raylib.h"
#include <cstdio>
#include <cstring>

namespace ui {

namespace {
constexpr const char* kNombresOpciones[kNumOpcionesMenuInicio] = {
    "Nueva partida", "Cargar", "Sobre mi", "Salir"
};

constexpr const char* kNombresOpcionesPausa[kNumOpcionesPausa] = {
    "Continuar", "Guardar", "Volver al mapa", "Menu principal", "Salir"
};

// Nombre y descripcion corta de cada mazmorra del mapa, en el mismo orden
// fijo que game::Dificultad en main.cpp (Facil=0, Media=1, Dificil=2) — este
// archivo no incluye ningun header de game/ a proposito (ver el comentario
// de DibujarMapa en menu_ui.h), asi que la correspondencia de indices es lo
// unico que los mantiene sincronizados.
constexpr const char* kNombresMazmorrasMapa[kNumMazmorrasMapa] = {
    "Mazmorra Facil", "Mazmorra Media", "Mazmorra Dificil"
};
constexpr const char* kDescripcionesMazmorrasMapa[kNumMazmorrasMapa] = {
    "Menos enemigos, enemigos mas debiles. Botin normal.",
    "El desafio de siempre.",
    "Mas enemigos, enemigos mas fuertes. Mejor botin.",
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

void DibujarSeleccionSlot(int anchoVentana, int altoVentana, int opcionSeleccionada, bool modoGuardar,
                           const bool ocupado[game::kNumSlots], const std::string& mensaje) {
    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 8, 8, 14, 225 });

    DibujarTitulo(anchoVentana, altoVentana);

    const char* subtitulo = modoGuardar ? "Guardar partida" : "Cargar partida";
    int anchoSub = MeasureText(subtitulo, 22);
    DrawText(subtitulo, (anchoVentana - anchoSub) / 2, altoVentana / 2 - 100, 22, Color{ 230, 190, 80, 255 });

    int tamanoOpcion = 26;
    int yOpciones = altoVentana / 2 - 30;
    for (int i = 0; i < kNumOpcionesSlot; ++i) {
        bool esVolver = (i == game::kNumSlots);
        bool esSeleccionada = (i == opcionSeleccionada);
        // En modo Cargar, un slot vacio no hace nada al confirmarlo (igual
        // que "Cargar" deshabilitada en DibujarMenuInicio) — se dibuja
        // atenuado para que se note sin tener que confirmarlo para
        // averiguarlo. En modo Guardar todos los slots estan siempre
        // habilitados (guardar en uno vacio lo crea; guardar en uno
        // ocupado lo pisa).
        bool deshabilitado = !esVolver && !modoGuardar && !ocupado[i];

        char texto[48];
        if (esVolver) {
            std::snprintf(texto, sizeof(texto), "%sVolver", esSeleccionada ? "> " : "  ");
        } else {
            std::snprintf(texto, sizeof(texto), "%sSlot %d %s", esSeleccionada ? "> " : "  ",
                          i + 1, ocupado[i] ? "(ocupado)" : "(vacio)");
        }

        Color color;
        if (deshabilitado) {
            color = Color{ 95, 90, 80, 255 };
        } else if (esSeleccionada) {
            color = Color{ 255, 235, 180, 255 };
        } else {
            color = Color{ 140, 140, 150, 255 };
        }

        int ancho = MeasureText(texto, tamanoOpcion);
        DrawText(texto, (anchoVentana - ancho) / 2, yOpciones, tamanoOpcion, color);

        if (deshabilitado && esSeleccionada) {
            const char* aclaracion = "(vacio — no hay nada para cargar)";
            int anchoAclaracion = MeasureText(aclaracion, 14);
            DrawText(aclaracion, (anchoVentana - anchoAclaracion) / 2, yOpciones + tamanoOpcion + 2, 14,
                     Color{ 120, 115, 100, 255 });
        }

        yOpciones += 46;
    }

    if (!mensaje.empty()) {
        int anchoMsg = MeasureText(mensaje.c_str(), 16);
        DrawText(mensaje.c_str(), (anchoVentana - anchoMsg) / 2, yOpciones + 6, 16, Color{ 200, 200, 160, 255 });
    }

    const char* prompt = "[flechas o W/S] moverse    [ENTER] confirmar    [ESC] volver";
    int anchoPrompt = MeasureText(prompt, 16);
    DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana - 60, 16, Color{ 130, 130, 140, 255 });
}

// Dibuja 'texto' partido en varias lineas para que ninguna supere
// 'anchoMaximo' pixels a este tamano de fuente (corta por espacios, nunca a
// mitad de palabra). Se agrego al verificar visualmente bajo Xvfb que la
// descripcion de "Mazmorra Facil" (la mas larga de las tres) se salia del
// ancho de su tarjeta con un DrawText de una sola linea — envolver a mano
// tres textos de largo distinto es fragil ante el mas minimo cambio de
// texto, asi que se resuelve con wrap automatico en vez de reintentar a ojo.
void DibujarTextoEnvuelto(const char* texto, int x, int y, int anchoMaximo, int tamano, Color color) {
    std::string linea;
    std::string palabra;
    int lineasImpresas = 0;
    auto ImprimirLinea = [&](const std::string& l) {
        DrawText(l.c_str(), x, y + lineasImpresas * (tamano + 4), tamano, color);
        lineasImpresas += 1;
    };

    size_t len = std::strlen(texto);
    for (size_t i = 0; i <= len; ++i) {
        if (i == len || texto[i] == ' ') {
            std::string candidata = linea.empty() ? palabra : (linea + " " + palabra);
            if (!linea.empty() && MeasureText(candidata.c_str(), tamano) > anchoMaximo) {
                ImprimirLinea(linea);
                linea = palabra;
            } else {
                linea = candidata;
            }
            palabra.clear();
        } else {
            palabra += texto[i];
        }
    }
    if (!linea.empty()) ImprimirLinea(linea);
}

void DibujarMapa(int anchoVentana, int altoVentana, int opcionSeleccionada, const bool superada[kNumMazmorrasMapa]) {
    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 8, 8, 14, 225 });

    DibujarTitulo(anchoVentana, altoVentana);

    const char* subtitulo = "Elegi una mazmorra";
    int anchoSub = MeasureText(subtitulo, 22);
    DrawText(subtitulo, (anchoVentana - anchoSub) / 2, altoVentana / 2 - 130, 22, Color{ 230, 190, 80, 255 });

    // 3 tarjetas una al lado de la otra, centradas como grupo — mismo
    // criterio de tamano fijo (solo cambia color/borde con la seleccion) que
    // el resto de las pantallas de este archivo, para que nada salte de
    // lugar al mover el cursor.
    int anchoTarjeta = 260;
    int altoTarjeta = 170;
    int espacio = 30;
    int anchoTotal = kNumMazmorrasMapa * anchoTarjeta + (kNumMazmorrasMapa - 1) * espacio;
    int xInicio = (anchoVentana - anchoTotal) / 2;
    int yTarjeta = altoVentana / 2 - 80;

    for (int i = 0; i < kNumMazmorrasMapa; ++i) {
        bool esSeleccionada = (i == opcionSeleccionada);
        int x = xInicio + i * (anchoTarjeta + espacio);

        Color fondo = esSeleccionada ? Color{ 45, 45, 30, 230 } : Color{ 20, 20, 25, 200 };
        Color borde = esSeleccionada ? Color{ 230, 200, 90, 255 } : Color{ 80, 80, 90, 255 };
        DrawRectangle(x, yTarjeta, anchoTarjeta, altoTarjeta, fondo);
        DrawRectangleLines(x, yTarjeta, anchoTarjeta, altoTarjeta, borde);

        Color colorNombre = esSeleccionada ? Color{ 255, 235, 180, 255 } : Color{ 200, 200, 210, 255 };
        int anchoNombre = MeasureText(kNombresMazmorrasMapa[i], 20);
        DrawText(kNombresMazmorrasMapa[i], x + (anchoTarjeta - anchoNombre) / 2, yTarjeta + 18, 20, colorNombre);

        DibujarTextoEnvuelto(kDescripcionesMazmorrasMapa[i], x + 14, yTarjeta + 60,
                             anchoTarjeta - 28, 13, Color{ 190, 190, 195, 255 });

        if (superada[i]) {
            // Texto plano en vez de un simbolo tipo check: la fuente por
            // defecto de raylib no cubre glyphs fuera de ASCII (mismo motivo
            // por el que el resto del juego usa "?"/"CAIDO"/"DERROTADO" como
            // texto en vez de iconos).
            const char* etiqueta = "(Superada)";
            int anchoEtiqueta = MeasureText(etiqueta, 16);
            DrawText(etiqueta, x + (anchoTarjeta - anchoEtiqueta) / 2, yTarjeta + altoTarjeta - 32, 16,
                     Color{ 120, 210, 130, 255 });
        }
    }

    const char* prompt = "[flechas o A/D] moverse    [ENTER] entrar    [ESC] pausa";
    int anchoPrompt = MeasureText(prompt, 16);
    DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana - 60, 16, Color{ 130, 130, 140, 255 });
}

}  // namespace ui
