#include "sprites.h"

namespace render {

namespace {

Color Bone()     { return Color{ 225, 220, 205, 255 }; }
Color BoneShade() { return Color{ 190, 185, 170, 255 }; }

// --- Personajes (party) ---
// Cada uno reusa el color de rol que ya definia ColorDeRol (ver
// renderer.cpp de versiones anteriores) para que la identidad de color por
// rol siga siendo la misma que ya conoce el jugador, ahora aplicada a una
// silueta pixel-art en vez de un circulo liso.

Image CrearTanque() {
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 235, 194, 150, 255 };
    Color acero = { 95, 110, 135, 255 };
    Color aceroOsc = { 60, 72, 92, 255 };
    Color armadura = { 90, 130, 220, 255 };   // mismo azul que ColorDeRol(Tanque)
    Color armaduraOsc = { 60, 90, 165, 255 };
    Color escudo = { 165, 170, 180, 255 };
    Color escudoOsc = { 110, 115, 125, 255 };
    Color bota = { 45, 45, 55, 255 };
    Color visor = { 25, 25, 30, 255 };

    // Cabeza (piel) + casco (banda de acero arriba, dejando la cara abajo)
    ImageDrawCircle(&img, 10, 8, 5, piel);
    ImageDrawRectangle(&img, 4, 2, 12, 6, acero);
    ImageDrawRectangle(&img, 4, 7, 12, 2, aceroOsc);
    ImageDrawRectangle(&img, 4, 2, 12, 1, Color{ 140, 150, 165, 255 });
    ImageDrawRectangle(&img, 7, 9, 6, 2, visor);

    // Torso (armadura)
    ImageDrawRectangle(&img, 4, 13, 12, 8, armadura);
    ImageDrawRectangle(&img, 4, 13, 12, 2, armaduraOsc);
    ImageDrawRectangle(&img, 9, 13, 2, 8, armaduraOsc);

    // Escudo (izquierda)
    ImageDrawCircle(&img, 3, 17, 5, escudoOsc);
    ImageDrawCircle(&img, 3, 17, 4, escudo);
    ImageDrawCircle(&img, 3, 17, 1, escudoOsc);

    // Botas
    ImageDrawRectangle(&img, 6, 21, 3, 5, bota);
    ImageDrawRectangle(&img, 11, 21, 3, 5, bota);

    return img;
}

Image CrearDanio() {
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 235, 194, 150, 255 };
    Color pelo = { 60, 40, 30, 255 };
    Color ropa = { 220, 90, 90, 255 };  // mismo rojo que ColorDeRol(Danio)
    Color ropaOsc = { 165, 60, 60, 255 };
    Color cinto = { 70, 45, 30, 255 };
    Color hoja = { 205, 205, 215, 255 };
    Color bota = { 55, 40, 35, 255 };

    ImageDrawCircle(&img, 10, 7, 5, piel);
    ImageDrawRectangle(&img, 4, 2, 12, 5, pelo);
    ImageDrawCircle(&img, 10, 6, 5, pelo);
    ImageDrawRectangle(&img, 4, 6, 12, 3, piel);

    ImageDrawRectangle(&img, 5, 13, 10, 8, ropa);
    ImageDrawRectangle(&img, 5, 13, 10, 2, ropaOsc);
    ImageDrawRectangle(&img, 4, 18, 12, 2, cinto);

    // Daga/espada corta en diagonal a la derecha
    ImageDrawLineEx(&img, Vector2{ 16, 10 }, Vector2{ 19, 4 }, 2, hoja);
    ImageDrawLineEx(&img, Vector2{ 16, 10 }, Vector2{ 14, 13 }, 2, cinto);

    ImageDrawRectangle(&img, 6, 21, 3, 5, bota);
    ImageDrawRectangle(&img, 11, 21, 3, 5, bota);

    return img;
}

Image CrearSoporte() {
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 235, 194, 150, 255 };
    Color capucha = { 100, 210, 130, 255 };  // mismo verde que ColorDeRol(Soporte)
    Color capuchaOsc = { 65, 155, 90, 255 };
    Color tunica = { 80, 180, 110, 255 };
    Color tunicaOsc = { 55, 130, 80, 255 };
    Color baston = { 130, 95, 55, 255 };
    Color gema = { 230, 220, 140, 255 };

    ImageDrawCircle(&img, 10, 8, 5, piel);
    ImageDrawCircle(&img, 10, 6, 6, capucha);
    ImageDrawRectangle(&img, 5, 8, 10, 3, piel);

    ImageDrawRectangle(&img, 6, 13, 8, 4, tunica);
    ImageDrawRectangle(&img, 4, 17, 12, 4, tunica);
    ImageDrawRectangle(&img, 2, 21, 16, 3, tunicaOsc);
    ImageDrawRectangle(&img, 6, 13, 8, 1, capuchaOsc);

    // Baston con gema, del lado derecho
    ImageDrawLineEx(&img, Vector2{ 17, 22 }, Vector2{ 17, 6 }, 2, baston);
    ImageDrawCircle(&img, 17, 5, 2, gema);

    return img;
}

Image CrearControl() {
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 225, 185, 150, 255 };
    Color capucha = { 210, 170, 90, 255 };  // mismo dorado que ColorDeRol(Control)
    Color capuchaOsc = { 160, 125, 60, 255 };
    Color tunica = { 60, 55, 70, 255 };     // oscura, contraste con la capucha dorada
    Color tunicaOsc = { 40, 37, 48, 255 };
    Color orbe = { 150, 110, 220, 255 };
    Color orbeBrillo = { 210, 180, 250, 255 };

    ImageDrawCircle(&img, 10, 9, 4, Color{ 20, 18, 25, 255 });  // sombra bajo la capucha
    ImageDrawCircle(&img, 10, 6, 6, capucha);
    ImageDrawTriangle(&img, Vector2{ 4, 6 }, Vector2{ 16, 6 }, Vector2{ 10, 14 }, capuchaOsc);
    ImageDrawRectangle(&img, 8, 8, 4, 3, piel);

    ImageDrawRectangle(&img, 5, 14, 10, 7, tunica);
    ImageDrawTriangle(&img, Vector2{ 2, 26 }, Vector2{ 18, 26 }, Vector2{ 10, 14 }, tunicaOsc);
    ImageDrawRectangle(&img, 5, 14, 10, 2, capuchaOsc);

    // Orbe flotante en la mano
    ImageDrawCircle(&img, 16, 16, 3, orbeBrillo);
    ImageDrawCircle(&img, 16, 16, 2, orbe);

    return img;
}

// --- Enemigos ---
// Mismo criterio de color que ColorDeEnemigo (ver renderer.cpp de versiones
// anteriores) preservado por tipo.

Image CrearEsqueleto() {
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color hueso = Bone();
    Color huesoOsc = BoneShade();
    Color cuenca = { 30, 25, 25, 255 };
    Color brillo = { 220, 60, 50, 255 };

    ImageDrawCircle(&img, 10, 7, 5, hueso);
    ImageDrawCircle(&img, 8, 7, 2, cuenca);
    ImageDrawCircle(&img, 12, 7, 2, cuenca);
    ImageDrawCircle(&img, 8, 7, 1, brillo);
    ImageDrawCircle(&img, 12, 7, 1, brillo);
    ImageDrawRectangle(&img, 8, 10, 4, 2, huesoOsc);  // mandibula

    // Caja toracica: columna central + costillas horizontales
    ImageDrawRectangle(&img, 9, 13, 2, 9, hueso);
    for (int y = 14; y <= 19; y += 2) {
        ImageDrawLine(&img, 6, y, 14, y, huesoOsc);
    }
    ImageDrawRectangle(&img, 6, 13, 2, 8, hueso);
    ImageDrawRectangle(&img, 12, 13, 2, 8, hueso);

    // Brazos finos colgando
    ImageDrawRectangle(&img, 3, 14, 2, 7, hueso);
    ImageDrawRectangle(&img, 15, 14, 2, 7, hueso);

    // Piernas finas
    ImageDrawRectangle(&img, 7, 21, 2, 5, hueso);
    ImageDrawRectangle(&img, 11, 21, 2, 5, hueso);

    return img;
}

Image CrearRata() {
    // Cuadrupedo, mas bajo y ancho que los humanoides. Version retocada
    // respecto del primer prototipo (proto2.png): las orejas ahora tienen
    // un color interior propio (rosado) que contrasta con el pelaje en vez
    // de un tono casi identico, y un contorno separa la cabeza del cuerpo —
    // a la escala chica de juego (1.8x) el primer intento se leia como una
    // "bolita marrON" sin rasgos; con este contraste extra se distinguen
    // las orejas y la cabeza incluso de lejos. La cola tambien quedo un
    // poco mas larga y clara para que no se pierda contra el piso oscuro.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color pelaje = { 150, 110, 55, 255 };
    Color pelajeOsc = { 105, 74, 36, 255 };
    Color panza = { 205, 172, 120, 255 };
    Color diente = { 240, 235, 220, 255 };
    Color ojo = { 20, 15, 15, 255 };
    Color orejaInterior = { 210, 150, 140, 255 };  // rosado, contrasta con el pelaje
    Color cola = { 185, 140, 110, 255 };

    int cy = 17;  // centro vertical del cuerpo, mas abajo que los humanoides

    // Orejas primero (quedan detras de la cabeza, asoman arriba)
    ImageDrawTriangle(&img, Vector2{ 0, (float)(cy - 7) }, Vector2{ 4, (float)(cy - 10) }, Vector2{ 5, (float)(cy - 3) }, pelajeOsc);
    ImageDrawTriangle(&img, Vector2{ 1, (float)(cy - 6) }, Vector2{ 4, (float)(cy - 8) }, Vector2{ 4, (float)(cy - 3) }, orejaInterior);
    ImageDrawTriangle(&img, Vector2{ 5, (float)(cy - 7) }, Vector2{ 9, (float)(cy - 9) }, Vector2{ 7, (float)(cy - 3) }, pelajeOsc);
    ImageDrawTriangle(&img, Vector2{ 6, (float)(cy - 6) }, Vector2{ 8, (float)(cy - 7) }, Vector2{ 7, (float)(cy - 3) }, orejaInterior);

    ImageDrawCircle(&img, 10, cy, 7, pelaje);              // cuerpo
    ImageDrawCircle(&img, 10, cy + 3, 4, panza);           // panza clara abajo
    ImageDrawCircle(&img, 4, cy - 3, 4, pelaje);           // cabeza adelante
    ImageDrawCircleLines(&img, 4, cy - 3, 4, pelajeOsc);   // separa la cabeza del cuerpo
    ImageDrawCircle(&img, 2, cy - 3, 1, ojo);
    ImageDrawRectangle(&img, 0, cy - 2, 2, 2, diente);     // diente asomando

    // Patas cortas
    ImageDrawRectangle(&img, 5, cy + 6, 2, 4, pelajeOsc);
    ImageDrawRectangle(&img, 13, cy + 6, 2, 4, pelajeOsc);

    // Cola curva atras, un poco mas larga que la version original
    ImageDrawLineEx(&img, Vector2{ 17, (float)cy }, Vector2{ 20, (float)(cy - 7) }, 2, cola);

    return img;
}

Image CrearBandido() {
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 200, 160, 130, 255 };
    Color capucha = { 130, 55, 150, 255 };  // mismo violeta que ColorDeEnemigo(BanditoAturdidor)
    Color capuchaOsc = { 90, 38, 105, 255 };
    Color tela = { 70, 65, 75, 255 };
    Color garrote = { 90, 65, 40, 255 };
    Color metal = { 150, 150, 160, 255 };

    ImageDrawCircle(&img, 10, 8, 5, piel);
    ImageDrawRectangle(&img, 4, 3, 12, 6, capucha);
    ImageDrawRectangle(&img, 4, 8, 12, 2, capuchaOsc);
    ImageDrawRectangle(&img, 7, 9, 2, 2, Color{ 20, 20, 25, 255 });   // ojos entrecerrados
    ImageDrawRectangle(&img, 11, 9, 2, 2, Color{ 20, 20, 25, 255 });

    // Torso mas corpulento
    ImageDrawRectangle(&img, 3, 13, 14, 9, tela);
    ImageDrawRectangle(&img, 3, 13, 14, 2, capuchaOsc);

    // Garrote con remaches, en diagonal
    ImageDrawLineEx(&img, Vector2{ 17, 22 }, Vector2{ 15, 9 }, 3, garrote);
    ImageDrawCircle(&img, 15, 9, 2, metal);

    ImageDrawRectangle(&img, 5, 22, 4, 4, Color{ 40, 38, 45, 255 });
    ImageDrawRectangle(&img, 11, 22, 4, 4, Color{ 40, 38, 45, 255 });

    return img;
}

Image CrearCapitan() {
    // Version "jefe" del Bandido: mismo lienzo, mas ornamentado (corona en
    // vez de capucha lisa, capa, espada en vez de garrote) para que se note
    // a simple vista que es distinto apenas se lo ve — coherente con el
    // anillo dorado que ya lo distinguia en la exploracion.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 200, 160, 130, 255 };
    Color capa = { 60, 15, 20, 255 };  // rojo sangre oscuro, no el violeta de la tropa comun
    Color capaOsc = { 40, 10, 14, 255 };
    Color tela = { 45, 40, 48, 255 };
    Color oro = { 215, 180, 90, 255 };
    Color espada = { 205, 205, 215, 255 };
    Color empunadura = { 90, 65, 40, 255 };

    ImageDrawCircle(&img, 10, 8, 5, piel);
    ImageDrawRectangle(&img, 3, 2, 14, 6, capa);
    ImageDrawRectangle(&img, 3, 7, 14, 2, capaOsc);
    // Corona: tres picos dorados sobre la capucha
    ImageDrawTriangle(&img, Vector2{ 5, 3 }, Vector2{ 7, 3 }, Vector2{ 6, 0 }, oro);
    ImageDrawTriangle(&img, Vector2{ 9, 3 }, Vector2{ 11, 3 }, Vector2{ 10, 0 }, oro);
    ImageDrawTriangle(&img, Vector2{ 13, 3 }, Vector2{ 15, 3 }, Vector2{ 14, 0 }, oro);
    ImageDrawRectangle(&img, 7, 9, 2, 2, Color{ 20, 20, 25, 255 });
    ImageDrawRectangle(&img, 11, 9, 2, 2, Color{ 20, 20, 25, 255 });

    // Torso corpulento con capa y ribete dorado
    ImageDrawRectangle(&img, 3, 13, 14, 9, tela);
    ImageDrawRectangle(&img, 3, 13, 14, 2, capaOsc);
    ImageDrawRectangle(&img, 3, 13, 2, 9, capa);
    ImageDrawRectangle(&img, 15, 13, 2, 9, capa);
    ImageDrawRectangle(&img, 9, 13, 2, 9, oro);  // franja dorada al medio

    // Espada larga en vez de garrote
    ImageDrawLineEx(&img, Vector2{ 18, 23 }, Vector2{ 18, 3 }, 2, espada);
    ImageDrawRectangle(&img, 15, 15, 6, 2, empunadura);

    ImageDrawRectangle(&img, 5, 22, 4, 4, Color{ 35, 32, 38, 255 });
    ImageDrawRectangle(&img, 11, 22, 4, 4, Color{ 35, 32, 38, 255 });

    return img;
}

// --- Objetos del mapa ---

Image CrearCofre(bool abierto) {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, BLANK);
    Color madera = { 120, 85, 50, 255 };
    Color maderaOsc = { 85, 58, 32, 255 };
    Color metal = { 200, 165, 60, 255 };
    Color metalOsc = { 145, 115, 40, 255 };
    Color interior = { 30, 26, 20, 255 };

    if (!abierto) {
        ImageDrawRectangle(&img, 1, 5, 14, 10, madera);
        ImageDrawRectangle(&img, 1, 5, 14, 2, maderaOsc);
        ImageDrawRectangle(&img, 1, 12, 14, 1, maderaOsc);
        ImageDrawRectangle(&img, 0, 4, 16, 2, metal);
        ImageDrawRectangle(&img, 0, 10, 16, 2, metal);
        ImageDrawRectangle(&img, 7, 8, 2, 3, metalOsc);  // cerradura
    } else {
        ImageDrawRectangle(&img, 1, 7, 14, 8, maderaOsc);
        ImageDrawRectangle(&img, 1, 12, 14, 1, Color{ 60, 42, 24, 255 });
        ImageDrawRectangle(&img, 2, 8, 12, 6, interior);
        ImageDrawRectangle(&img, 1, 1, 14, 3, madera);   // tapa abierta hacia atras
        ImageDrawRectangle(&img, 1, 1, 14, 1, maderaOsc);
        ImageDrawRectangle(&img, 0, 9, 16, 2, metalOsc);
    }

    return img;
}

// --- Decoracion suelta de piso ---
// Cuatro variantes chicas, en el mismo lienzo 16x16 que un tile, con canales
// alfa parciales (no opacas del todo) para que se vean como parte del piso
// en vez de un sticker pegado encima — renderer.cpp las reparte disperso
// por tile con un hash determinístico (ver "Sprites pixel-art generados por
// código" en docs/design.md), asi que no hace falta que cada una sea muy
// elaborada: la variedad sale de la mezcla + la dispersion, no de cada
// pieza individual.

Image CrearGrietaSuelo() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, BLANK);
    Color grieta = { 20, 18, 24, 200 };
    Color grietaTenue = { 20, 18, 24, 110 };
    ImageDrawLine(&img, 2, 3, 7, 8, grieta);
    ImageDrawLine(&img, 7, 8, 6, 13, grieta);
    ImageDrawLine(&img, 7, 8, 12, 10, grietaTenue);
    ImageDrawLine(&img, 6, 13, 3, 15, grietaTenue);
    return img;
}

Image CrearMusgoSuelo() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, BLANK);
    Color musgoOsc = { 45, 70, 40, 160 };
    Color musgoClaro = { 80, 120, 60, 140 };
    ImageDrawCircle(&img, 4, 12, 3, musgoOsc);
    ImageDrawCircle(&img, 7, 13, 3, musgoOsc);
    ImageDrawCircle(&img, 5, 11, 2, musgoClaro);
    ImageDrawCircle(&img, 8, 12, 2, musgoClaro);
    return img;
}

Image CrearEscombrosSuelo() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, BLANK);
    Color piedra = { 70, 66, 62, 220 };
    Color piedraClara = { 95, 90, 84, 220 };
    Color hueso = { 210, 205, 190, 210 };
    ImageDrawCircle(&img, 5, 11, 2, piedra);
    ImageDrawCircle(&img, 9, 12, 2, piedraClara);
    ImageDrawCircle(&img, 11, 9, 1, piedra);
    // Hueso: un palito fino con un "nudo" en cada punta.
    ImageDrawLineEx(&img, Vector2{ 3, 5 }, Vector2{ 8, 4 }, 1, hueso);
    ImageDrawCircle(&img, 3, 5, 1, hueso);
    ImageDrawCircle(&img, 8, 4, 1, hueso);
    return img;
}

Image CrearCharcoSuelo() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, BLANK);
    Color agua = { 35, 45, 60, 170 };
    Color aguaOsc = { 22, 30, 42, 170 };
    Color brillo = { 130, 160, 190, 120 };
    ImageDrawCircle(&img, 8, 9, 5, agua);
    ImageDrawCircleLines(&img, 8, 9, 5, aguaOsc);
    ImageDrawLineEx(&img, Vector2{ 5, 7 }, Vector2{ 9, 6 }, 1, brillo);
    return img;
}

// --- Trampas de piso ---
// A diferencia de la decoracion suelta (grieta/musgo/escombros/charco, mas
// arriba), estas SI afectan el gameplay -- van bien opacas y de colores
// llamativos (fuego = naranja/rojo sobre tierra quemada, acido = verde
// brillante sobre roca) para que se lean como "peligro" a simple vista, en
// vez de mezclarse con el resto del piso como la decoracion cosmetica.

Image CrearTrampaFuego() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, BLANK);
    Color tierraQuemada = { 42, 26, 20, 255 };
    Color brasa = { 90, 32, 16, 255 };
    Color fuegoOsc = { 200, 70, 30, 255 };
    Color fuegoMed = { 235, 140, 40, 255 };
    Color fuegoClaro = { 250, 210, 90, 255 };

    ImageDrawRectangle(&img, 0, 0, kCanvasTile, kCanvasTile, tierraQuemada);
    ImageDrawCircle(&img, 8, 12, 5, brasa);
    ImageDrawTriangle(&img, Vector2{ 3, 13 }, Vector2{ 13, 13 }, Vector2{ 8, 2 }, fuegoOsc);
    ImageDrawTriangle(&img, Vector2{ 5, 13 }, Vector2{ 11, 13 }, Vector2{ 8, 5 }, fuegoMed);
    ImageDrawTriangle(&img, Vector2{ 6, 13 }, Vector2{ 10, 13 }, Vector2{ 8, 7 }, fuegoClaro);

    return img;
}

Image CrearTrampaAcido() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, BLANK);
    Color roca = { 44, 48, 40, 255 };
    Color acidoOsc = { 70, 130, 40, 235 };
    Color acido = { 120, 200, 60, 235 };
    Color acidoClaro = { 190, 240, 110, 220 };
    Color burbuja = { 225, 250, 185, 210 };

    ImageDrawRectangle(&img, 0, 0, kCanvasTile, kCanvasTile, roca);
    ImageDrawCircle(&img, 8, 8, 7, acidoOsc);
    ImageDrawCircle(&img, 8, 8, 5, acido);
    ImageDrawCircle(&img, 6, 6, 2, acidoClaro);
    ImageDrawCircle(&img, 5, 10, 1, burbuja);
    ImageDrawCircle(&img, 11, 7, 1, burbuja);

    return img;
}

// --- Antorcha de pared ---
// Lienzo propio, mas angosto y alto que un tile (12x20): soporte + palo +
// llama en capas de color (de afuera hacia adentro, mas clara al medio).
// El parpadeo no se hornea en la textura — renderer.cpp varia la escala del
// dibujado cuadro a cuadro con GetTime(), asi que una sola imagen estatica
// alcanza (ver "Sprites pixel-art generados por código" en docs/design.md).
constexpr int kCanvasAntorchaAncho = 12;
constexpr int kCanvasAntorchaAlto = 20;

Image CrearAntorcha() {
    Image img = GenImageColor(kCanvasAntorchaAncho, kCanvasAntorchaAlto, BLANK);
    Color soporte = { 45, 40, 38, 255 };
    Color palo = { 90, 65, 40, 255 };
    Color fuegoOsc = { 200, 70, 30, 255 };
    Color fuegoMed = { 235, 140, 40, 255 };
    Color fuegoClaro = { 250, 210, 90, 255 };

    ImageDrawRectangle(&img, 3, 15, 6, 3, soporte);       // soporte de pared
    ImageDrawRectangle(&img, 5, 6, 2, 10, palo);          // palo
    ImageDrawTriangle(&img, Vector2{ 2, 7 }, Vector2{ 10, 7 }, Vector2{ 6, 0 }, fuegoOsc);
    ImageDrawTriangle(&img, Vector2{ 3, 7 }, Vector2{ 9, 7 }, Vector2{ 6, 2 }, fuegoMed);
    ImageDrawTriangle(&img, Vector2{ 4, 7 }, Vector2{ 8, 7 }, Vector2{ 6, 3 }, fuegoClaro);

    return img;
}

// --- Tiles de mapa ---

Image CrearTilePiso() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, Color{ 40, 38, 45, 255 });
    Color claro = { 46, 44, 52, 255 };
    Color oscuro = { 34, 32, 38, 255 };

    // Solo un par de manchas sutiles (sin lineas rectas que crucen el tile
    // entero) — con lineas, el patron se notaba demasiado al repetirse por
    // toda una sala grande; asi lee mas a "piedra desgastada" que a grilla.
    ImageDrawRectangle(&img, 1, 2, 4, 3, claro);
    ImageDrawRectangle(&img, 10, 9, 5, 4, oscuro);
    ImageDrawRectangle(&img, 3, 11, 3, 2, oscuro);
    ImageDrawRectangle(&img, 12, 2, 2, 2, claro);

    return img;
}

Image CrearTilePared() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, Color{ 76, 58, 50, 255 });
    Color ladrilloClaro = { 100, 78, 66, 255 };
    Color mortero = { 55, 42, 36, 255 };

    // Dos hiladas de ladrillos (8px cada una), la de abajo corrida la mitad
    // (aparejo tipico) para que se note el patron al repetir el tile.
    ImageDrawRectangle(&img, 0, 0, 16, 1, mortero);
    ImageDrawRectangle(&img, 0, 7, 16, 2, mortero);
    ImageDrawRectangle(&img, 0, 15, 16, 1, mortero);
    ImageDrawRectangle(&img, 7, 0, 1, 8, mortero);
    ImageDrawRectangle(&img, 3, 8, 1, 8, mortero);
    ImageDrawRectangle(&img, 11, 8, 1, 8, mortero);

    ImageDrawRectangle(&img, 1, 1, 6, 6, ladrilloClaro);
    ImageDrawRectangle(&img, 8, 1, 7, 6, ladrilloClaro);
    ImageDrawRectangle(&img, 4, 9, 7, 6, ladrilloClaro);
    ImageDrawRectangle(&img, 12, 9, 4, 6, ladrilloClaro);
    ImageDrawRectangle(&img, 0, 9, 3, 6, ladrilloClaro);

    return img;
}

Texture2D CargarPixelPerfecto(Image img) {
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

}  // namespace

SpriteSet::SpriteSet() {
    personajes_[static_cast<int>(game::Role::Tanque)] = CargarPixelPerfecto(CrearTanque());
    personajes_[static_cast<int>(game::Role::Danio)] = CargarPixelPerfecto(CrearDanio());
    personajes_[static_cast<int>(game::Role::Soporte)] = CargarPixelPerfecto(CrearSoporte());
    personajes_[static_cast<int>(game::Role::Control)] = CargarPixelPerfecto(CrearControl());

    enemigos_[static_cast<int>(game::TipoEnemigo::EsqueletoErrante)] = CargarPixelPerfecto(CrearEsqueleto());
    enemigos_[static_cast<int>(game::TipoEnemigo::RataGigante)] = CargarPixelPerfecto(CrearRata());
    enemigos_[static_cast<int>(game::TipoEnemigo::BanditoAturdidor)] = CargarPixelPerfecto(CrearBandido());
    enemigos_[static_cast<int>(game::TipoEnemigo::CapitanBandido)] = CargarPixelPerfecto(CrearCapitan());

    cofreCerrado_ = CargarPixelPerfecto(CrearCofre(false));
    cofreAbierto_ = CargarPixelPerfecto(CrearCofre(true));

    decoracionesPiso_[0] = CargarPixelPerfecto(CrearGrietaSuelo());
    decoracionesPiso_[1] = CargarPixelPerfecto(CrearMusgoSuelo());
    decoracionesPiso_[2] = CargarPixelPerfecto(CrearEscombrosSuelo());
    decoracionesPiso_[3] = CargarPixelPerfecto(CrearCharcoSuelo());
    antorcha_ = CargarPixelPerfecto(CrearAntorcha());

    trampas_[static_cast<int>(game::TipoTrampa::Fuego)] = CargarPixelPerfecto(CrearTrampaFuego());
    trampas_[static_cast<int>(game::TipoTrampa::Acido)] = CargarPixelPerfecto(CrearTrampaAcido());

    tilePiso_ = CargarPixelPerfecto(CrearTilePiso());
    tilePared_ = CargarPixelPerfecto(CrearTilePared());
    SetTextureWrap(tilePiso_, TEXTURE_WRAP_REPEAT);
    SetTextureWrap(tilePared_, TEXTURE_WRAP_REPEAT);
}

SpriteSet::~SpriteSet() {
    for (auto& tex : personajes_) UnloadTexture(tex);
    for (auto& tex : enemigos_) UnloadTexture(tex);
    UnloadTexture(cofreCerrado_);
    UnloadTexture(cofreAbierto_);
    UnloadTexture(tilePiso_);
    UnloadTexture(tilePared_);
    for (auto& tex : decoracionesPiso_) UnloadTexture(tex);
    UnloadTexture(antorcha_);
    for (auto& tex : trampas_) UnloadTexture(tex);
}

void DibujarSpritePlantado(const Texture2D& textura, Vector2 posicionPies, float escala, Color tinte) {
    Rectangle src{ 0, 0, (float)textura.width, (float)textura.height };
    Rectangle dst{
        posicionPies.x - textura.width * escala * 0.5f,
        posicionPies.y - textura.height * escala,
        textura.width * escala,
        textura.height * escala
    };
    DrawTexturePro(textura, src, dst, Vector2{ 0, 0 }, 0.0f, tinte);
}

void DibujarSpriteCentrado(const Texture2D& textura, Vector2 centro, float escala, Color tinte) {
    Rectangle src{ 0, 0, (float)textura.width, (float)textura.height };
    Rectangle dst{
        centro.x - textura.width * escala * 0.5f,
        centro.y - textura.height * escala * 0.5f,
        textura.width * escala,
        textura.height * escala
    };
    DrawTexturePro(textura, src, dst, Vector2{ 0, 0 }, 0.0f, tinte);
}

void DibujarTileado(const Texture2D& textura, Rectangle destinoMundo, float escalaTile) {
    Rectangle src{
        destinoMundo.x / escalaTile,
        destinoMundo.y / escalaTile,
        destinoMundo.width / escalaTile,
        destinoMundo.height / escalaTile
    };
    DrawTexturePro(textura, src, destinoMundo, Vector2{ 0, 0 }, 0.0f, WHITE);
}

} // namespace render
