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
    "Continuar", "Guardar", "Volver a la ciudad", "Menu principal", "Salir"
};

// Nombre y descripcion corta de cada tema del mapa (paso 1), en el mismo
// orden fijo que game::Tema en main.cpp (Bosque=0, Carcel=1, Castillo=2) —
// este archivo no incluye ningun header de game/ a proposito (ver el
// comentario de DibujarMapaTema en menu_ui.h), asi que la correspondencia
// de indices es lo unico que los mantiene sincronizados.
constexpr const char* kNombresTemasMapa[kNumTemasMapa] = {
    "Bosque", "Carcel", "Castillo"
};
constexpr const char* kDescripcionesTemasMapa[kNumTemasMapa] = {
    "Lobos salvajes y arañas gigantes entre los árboles.",
    "Presos amotinados y guardias corruptos tras las rejas.",
    "Guardias reales y magos de la corte custodian el trono.",
};

// Nombre y descripcion corta de cada dificultad del mapa (paso 2, dentro de
// un tema ya elegido), en el mismo orden fijo que game::Dificultad en
// main.cpp (Facil=0, Media=1, Dificil=2).
constexpr const char* kNombresMazmorrasMapa[kNumMazmorrasMapa] = {
    "Facil", "Media", "Dificil"
};
constexpr const char* kDescripcionesMazmorrasMapa[kNumMazmorrasMapa] = {
    "Menos enemigos, enemigos mas debiles. Botin normal.",
    "El desafio de siempre.",
    "Mas enemigos, enemigos mas fuertes. Mejor botin.",
};

// Tarjeta "Volver a la ciudad", agregada al final de los dos pasos del mapa
// (ver kNumOpcionesMapaTema/kNumOpcionesMapaDificultad en menu_ui.h) — mismo
// texto en los dos pasos, asi que vive una sola vez aca.
constexpr const char* kNombreVolverACiudad = "Volver a la ciudad";
constexpr const char* kDescripcionVolverACiudad = "Volver a la Ciudad a comprar, aprender o descansar.";

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

namespace {

// Dibuja 'cantidad' tarjetas iguales centradas como grupo, con titulo y
// prompt propios — factoriza el layout que antes tenia la unica pantalla de
// mapa (DibujarMapa) y que ahora reusan sus dos pasos (DibujarMapaTema y
// DibujarMapaDificultad): mismo tamano fijo de tarjeta, mismo criterio de
// borde/fondo segun seleccion, mismo texto envuelto para la descripcion, y
// una etiqueta inferior opcional por tarjeta ("(Superada)" en el paso 2,
// "x/3 superadas" en el paso 1 — cada llamador arma el texto que le
// corresponde, esta funcion solo lo dibuja si no viene vacio).
void DibujarTarjetasDeSeleccion(int anchoVentana, int altoVentana, const char* subtitulo,
                                 int cantidad, const char* const* nombres, const char* const* descripciones,
                                 const std::string etiquetas[], int opcionSeleccionada, const char* prompt) {
    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 8, 8, 14, 225 });

    DibujarTitulo(anchoVentana, altoVentana);

    int anchoSub = MeasureText(subtitulo, 22);
    DrawText(subtitulo, (anchoVentana - anchoSub) / 2, altoVentana / 2 - 130, 22, Color{ 230, 190, 80, 255 });

    // Tamano fijo de tarjeta (solo cambia color/borde con la seleccion) para
    // que nada salte de lugar al mover el cursor.
    int anchoTarjeta = 260;
    int altoTarjeta = 170;
    int espacio = 30;
    int anchoTotal = cantidad * anchoTarjeta + (cantidad - 1) * espacio;
    int xInicio = (anchoVentana - anchoTotal) / 2;
    int yTarjeta = altoVentana / 2 - 80;

    for (int i = 0; i < cantidad; ++i) {
        bool esSeleccionada = (i == opcionSeleccionada);
        int x = xInicio + i * (anchoTarjeta + espacio);

        Color fondo = esSeleccionada ? Color{ 45, 45, 30, 230 } : Color{ 20, 20, 25, 200 };
        Color borde = esSeleccionada ? Color{ 230, 200, 90, 255 } : Color{ 80, 80, 90, 255 };
        DrawRectangle(x, yTarjeta, anchoTarjeta, altoTarjeta, fondo);
        DrawRectangleLines(x, yTarjeta, anchoTarjeta, altoTarjeta, borde);

        Color colorNombre = esSeleccionada ? Color{ 255, 235, 180, 255 } : Color{ 200, 200, 210, 255 };
        int anchoNombre = MeasureText(nombres[i], 20);
        DrawText(nombres[i], x + (anchoTarjeta - anchoNombre) / 2, yTarjeta + 18, 20, colorNombre);

        DibujarTextoEnvuelto(descripciones[i], x + 14, yTarjeta + 60,
                             anchoTarjeta - 28, 13, Color{ 190, 190, 195, 255 });

        if (!etiquetas[i].empty()) {
            int anchoEtiqueta = MeasureText(etiquetas[i].c_str(), 16);
            DrawText(etiquetas[i].c_str(), x + (anchoTarjeta - anchoEtiqueta) / 2, yTarjeta + altoTarjeta - 32, 16,
                     Color{ 120, 210, 130, 255 });
        }
    }

    int anchoPrompt = MeasureText(prompt, 16);
    DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana - 60, 16, Color{ 130, 130, 140, 255 });
}

}  // namespace

void DibujarMapaTema(int anchoVentana, int altoVentana, int opcionSeleccionada,
                      const int progresoPorTema[kNumTemasMapa]) {
    // Arma un array de kNumOpcionesMapaTema (temas reales + "Volver a la
    // ciudad" al final, ver el comentario de esa constante en menu_ui.h) en
    // vez de agrandar kNombresTemasMapa/kDescripcionesTemasMapa -- esos dos
    // siguen siendo solo los 3 temas reales, indexables directo por
    // game::Tema desde main.cpp (DibujarMapaDificultad los reusa asi).
    const char* nombres[kNumOpcionesMapaTema];
    const char* descripciones[kNumOpcionesMapaTema];
    std::string etiquetas[kNumOpcionesMapaTema];
    for (int i = 0; i < kNumTemasMapa; ++i) {
        nombres[i] = kNombresTemasMapa[i];
        descripciones[i] = kDescripcionesTemasMapa[i];
        if (progresoPorTema[i] > 0) {
            etiquetas[i] = std::to_string(progresoPorTema[i]) + "/" + std::to_string(kNumMazmorrasMapa) + " superadas";
        }
    }
    nombres[kNumTemasMapa] = kNombreVolverACiudad;
    descripciones[kNumTemasMapa] = kDescripcionVolverACiudad;

    DibujarTarjetasDeSeleccion(anchoVentana, altoVentana, "Elegi un tema", kNumOpcionesMapaTema,
                               nombres, descripciones, etiquetas, opcionSeleccionada,
                               "[flechas o A/D] moverse    [ENTER] elegir    [ESC] volver a la ciudad");
}

void DibujarMapaDificultad(int anchoVentana, int altoVentana, int temaElegido, int opcionSeleccionada,
                            const bool superada[kNumMazmorrasMapa]) {
    // Texto plano en vez de un simbolo tipo check: la fuente por defecto de
    // raylib no cubre glyphs fuera de ASCII (mismo motivo por el que el
    // resto del juego usa "?"/"CAIDO"/"DERROTADO" como texto en vez de
    // iconos).
    // Mismo criterio que DibujarMapaTema: array de kNumOpcionesMapaDificultad
    // (dificultades reales + "Volver a la ciudad" al final).
    const char* nombres[kNumOpcionesMapaDificultad];
    const char* descripciones[kNumOpcionesMapaDificultad];
    std::string etiquetas[kNumOpcionesMapaDificultad];
    for (int i = 0; i < kNumMazmorrasMapa; ++i) {
        nombres[i] = kNombresMazmorrasMapa[i];
        descripciones[i] = kDescripcionesMazmorrasMapa[i];
        if (superada[i]) etiquetas[i] = "(Superada)";
    }
    nombres[kNumMazmorrasMapa] = kNombreVolverACiudad;
    descripciones[kNumMazmorrasMapa] = kDescripcionVolverACiudad;

    int tema = ((temaElegido % kNumTemasMapa) + kNumTemasMapa) % kNumTemasMapa;
    std::string subtitulo = std::string(kNombresTemasMapa[tema]) + " - elegi la dificultad";
    DibujarTarjetasDeSeleccion(anchoVentana, altoVentana, subtitulo.c_str(), kNumOpcionesMapaDificultad,
                               nombres, descripciones, etiquetas, opcionSeleccionada,
                               "[flechas o A/D] moverse    [ENTER] entrar    [ESC] volver");
}

void DibujarComercio(int anchoVentana, int altoVentana, bool esHerreria, const std::vector<OfertaComercio>& ofertas,
                      int oro, const std::string& mensaje) {
    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 10, 8, 15, 235 });

    const char* titulo = esHerreria ? "Herreria" : "Tienda";
    DrawText(titulo, 40, 32, 28, RAYWHITE);

    char lineaOro[32];
    std::snprintf(lineaOro, sizeof(lineaOro), "Oro: %d", oro);
    int anchoOro = MeasureText(lineaOro, 22);
    DrawText(lineaOro, anchoVentana - anchoOro - 40, 38, 22, Color{ 230, 200, 90, 255 });

    // Lista numerada [1]-[9], mismo estilo de tarjeta que DibujarInventario
    // (fondo + borde + etiqueta dorada), pero con el precio a la derecha en
    // vez de una cantidad -- atenuado (nombre y precio en rojo) si el oro
    // actual no alcanza, para que se note de un vistazo que no se puede
    // comprar todavia sin tener que probar la tecla.
    int xLista = 40;
    int yLista = 96;
    int anchoTarjeta = anchoVentana - 80;
    for (size_t i = 0; i < ofertas.size() && i < 9; ++i) {
        const auto& oferta = ofertas[i];
        bool alcanza = oro >= oferta.precio;
        int y = yLista + (int)i * 52;

        DrawRectangle(xLista, y, anchoTarjeta, 46, Color{ 22, 22, 28, 220 });
        DrawRectangleLines(xLista, y, anchoTarjeta, 46, Color{ 80, 80, 90, 255 });

        char etiqueta[16];
        std::snprintf(etiqueta, sizeof(etiqueta), "[%zu]", i + 1);
        DrawText(etiqueta, xLista + 10, y + 14, 18, Color{ 230, 200, 90, 255 });

        Color colorNombre = alcanza ? RAYWHITE : Color{ 140, 100, 100, 255 };
        DrawText(oferta.nombre.c_str(), xLista + 56, y + 6, 16, colorNombre);
        DrawText(oferta.descripcion.c_str(), xLista + 56, y + 26, 12, LIGHTGRAY);

        char precioTexto[24];
        std::snprintf(precioTexto, sizeof(precioTexto), "%d oro", oferta.precio);
        int anchoPrecio = MeasureText(precioTexto, 16);
        Color colorPrecio = alcanza ? Color{ 230, 200, 90, 255 } : Color{ 190, 90, 90, 255 };
        DrawText(precioTexto, xLista + anchoTarjeta - anchoPrecio - 14, y + 15, 16, colorPrecio);
    }

    int yMensaje = yLista + (int)ofertas.size() * 52 + 12;
    if (!mensaje.empty()) {
        int anchoMsg = MeasureText(mensaje.c_str(), 16);
        DrawText(mensaje.c_str(), (anchoVentana - anchoMsg) / 2, yMensaje, 16, Color{ 200, 200, 160, 255 });
    }

    const char* prompt = "[1-9] Comprar    [ESC] Volver a la ciudad";
    int anchoPrompt = MeasureText(prompt, 18);
    DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana - 48, 18, Color{ 255, 235, 180, 255 });
}

void DibujarAcademia(int anchoVentana, int altoVentana, const std::vector<FilaAcademia>& filas, const std::string& mensaje) {
    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 10, 8, 15, 235 });

    DrawText("Academia", 40, 32, 28, RAYWHITE);

    // Misma tarjeta que DibujarComercio, pero con 3 estados en vez de 2
    // (alcanza/no alcanza): disponible (numero activo, texto normal),
    // aprendida (atenuada en verde, sin numero activo) y bloqueada por nivel
    // (atenuada en rojo, sin numero activo) -- no hay precio, el nivel
    // requerido ocupa ese lugar a la derecha.
    int xLista = 40;
    int yLista = 96;
    int anchoTarjeta = anchoVentana - 80;
    for (size_t i = 0; i < filas.size() && i < (size_t)kMaxFilasAcademia; ++i) {
        const auto& fila = filas[i];
        int y = yLista + (int)i * 52;

        DrawRectangle(xLista, y, anchoTarjeta, 46, Color{ 22, 22, 28, 220 });
        DrawRectangleLines(xLista, y, anchoTarjeta, 46, Color{ 80, 80, 90, 255 });

        if (fila.disponible) {
            char etiqueta[16];
            std::snprintf(etiqueta, sizeof(etiqueta), "[%zu]", i + 1);
            DrawText(etiqueta, xLista + 10, y + 14, 18, Color{ 230, 200, 90, 255 });
        }

        Color colorNombre = fila.disponible ? RAYWHITE
            : (fila.aprendida ? Color{ 120, 170, 120, 255 } : Color{ 140, 100, 100, 255 });
        DrawText(fila.nombrePersonaje.c_str(), xLista + 56, y + 6, 16, colorNombre);
        DrawText(fila.nombreHabilidad.c_str(), xLista + 56, y + 26, 12, LIGHTGRAY);

        char estadoTexto[32];
        Color colorEstado;
        if (fila.aprendida) {
            std::snprintf(estadoTexto, sizeof(estadoTexto), "Aprendida");
            colorEstado = Color{ 120, 200, 120, 255 };
        } else if (fila.disponible) {
            std::snprintf(estadoTexto, sizeof(estadoTexto), "Nv.%d", fila.nivelRequerido);
            colorEstado = Color{ 230, 200, 90, 255 };
        } else {
            std::snprintf(estadoTexto, sizeof(estadoTexto), "Requiere Nv.%d", fila.nivelRequerido);
            colorEstado = Color{ 190, 90, 90, 255 };
        }
        int anchoEstado = MeasureText(estadoTexto, 16);
        DrawText(estadoTexto, xLista + anchoTarjeta - anchoEstado - 14, y + 15, 16, colorEstado);
    }

    int yMensaje = yLista + (int)filas.size() * 52 + 12;
    if (!mensaje.empty()) {
        int anchoMsg = MeasureText(mensaje.c_str(), 16);
        DrawText(mensaje.c_str(), (anchoVentana - anchoMsg) / 2, yMensaje, 16, Color{ 200, 200, 160, 255 });
    }

    const char* prompt = "[1-8] Aprender    [ESC] Volver a la ciudad";
    int anchoPrompt = MeasureText(prompt, 18);
    DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana - 48, 18, Color{ 255, 235, 180, 255 });
}

}  // namespace ui
