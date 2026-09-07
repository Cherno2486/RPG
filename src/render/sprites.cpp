#include "sprites.h"

namespace render {

namespace {

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
// 9 en total: 2 comunes + 1 jefe por cada uno de los 3 temas (Bosque,
// Carcel, Castillo — ver game::TipoEnemigo en enemy.h, el orden de ahi es
// el que indexa enemigos_[] en SpriteSet).

// -- Bosque --

Image CrearLoboSalvaje() {
    // Cuadrupedo (mismo plan de cuerpo que ya usaba la vieja Rata Gigante
    // del prototipo: cabeza adelante-abajo, cuerpo redondo, cola curva
    // atras) pero mas estilizado y oscuro, para leerse como depredador del
    // bosque en vez de una alimaña. Es el "comun agresivo" de este tema
    // (persigue al jugador, ver game::EsAgresivo).
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color pelaje = { 92, 92, 102, 255 };
    Color pelajeOsc = { 58, 58, 68, 255 };
    Color panza = { 150, 145, 138, 255 };
    Color ojo = { 235, 210, 60, 255 };
    Color colmillo = { 240, 240, 235, 255 };
    Color cola = { 75, 75, 85, 255 };

    int cy = 17;

    ImageDrawTriangle(&img, Vector2{ 0, (float)(cy - 8) }, Vector2{ 4, (float)(cy - 11) }, Vector2{ 5, (float)(cy - 3) }, pelajeOsc);
    ImageDrawTriangle(&img, Vector2{ 5, (float)(cy - 8) }, Vector2{ 9, (float)(cy - 10) }, Vector2{ 7, (float)(cy - 3) }, pelajeOsc);

    ImageDrawCircle(&img, 10, cy, 7, pelaje);
    ImageDrawCircle(&img, 10, cy + 3, 4, panza);
    ImageDrawCircle(&img, 4, cy - 3, 4, pelaje);
    ImageDrawCircleLines(&img, 4, cy - 3, 4, pelajeOsc);
    ImageDrawCircle(&img, 2, cy - 4, 1, ojo);
    ImageDrawRectangle(&img, 0, cy - 2, 2, 2, colmillo);

    ImageDrawRectangle(&img, 5, cy + 6, 2, 4, pelajeOsc);
    ImageDrawRectangle(&img, 13, cy + 6, 2, 4, pelajeOsc);

    ImageDrawLineEx(&img, Vector2{ 17, (float)cy }, Vector2{ 20, (float)(cy - 8) }, 2, cola);

    return img;
}

Image CrearAranaGigante() {
    // "Comun especial" del Bosque: a veces aplica Veneno (ver
    // game::AtaqueEspecialDe). 8 patas finas en angulo desde un cuerpo de
    // dos lobulos (cabeza chica + abdomen grande), tonos violeta oscuro
    // para distinguirse claramente del lobo.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color cuerpo = { 45, 35, 55, 255 };
    Color cuerpoOsc = { 28, 22, 38, 255 };
    Color pata = { 35, 28, 45, 255 };
    Color ojo = { 210, 40, 40, 255 };
    Color marca = { 150, 40, 130, 255 };

    int cx = 10, cy = 16;

    for (int i = 0; i < 4; ++i) {
        float dy = -6.0f + i * 4.0f;
        ImageDrawLineEx(&img, Vector2{ (float)cx - 5, cy + dy * 0.3f }, Vector2{ 0, cy + dy }, 2, pata);
        ImageDrawLineEx(&img, Vector2{ (float)cx + 5, cy + dy * 0.3f }, Vector2{ 19, cy + dy }, 2, pata);
    }

    ImageDrawCircle(&img, cx, cy + 3, 6, cuerpo);
    ImageDrawCircle(&img, cx, cy + 5, 2, marca);
    ImageDrawCircle(&img, cx, cy - 4, 4, cuerpo);
    ImageDrawCircleLines(&img, cx, cy - 4, 4, cuerpoOsc);
    ImageDrawCircle(&img, cx - 2, cy - 5, 1, ojo);
    ImageDrawCircle(&img, cx + 2, cy - 5, 1, ojo);

    return img;
}

Image CrearAlfaDelBosque() {
    // Jefe del Bosque: version "alfa" del lobo, mas grande y oscura, con
    // melena y colmillos mas marcados — mismo criterio que ya usaba el
    // Capitan Bandido (version ornamentada del comun) pero manteniendo el
    // plan cuadrupedo en vez de humanoide.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color pelaje = { 40, 38, 45, 255 };
    Color pelajeOsc = { 25, 24, 30, 255 };
    Color melena = { 65, 58, 52, 255 };
    Color panza = { 120, 110, 100, 255 };
    Color ojo = { 230, 60, 40, 255 };
    Color colmillo = { 245, 245, 240, 255 };
    Color cola = { 35, 34, 40, 255 };

    int cy = 18;

    ImageDrawTriangle(&img, Vector2{ -1, (float)(cy - 10) }, Vector2{ 5, (float)(cy - 14) }, Vector2{ 6, (float)(cy - 4) }, melena);
    ImageDrawTriangle(&img, Vector2{ 5, (float)(cy - 11) }, Vector2{ 10, (float)(cy - 13) }, Vector2{ 8, (float)(cy - 4) }, melena);

    ImageDrawCircle(&img, 11, cy, 8, pelaje);
    ImageDrawCircle(&img, 11, cy + 4, 5, panza);
    ImageDrawCircle(&img, 4, cy - 4, 5, pelaje);
    ImageDrawCircleLines(&img, 4, cy - 4, 5, pelajeOsc);
    ImageDrawCircle(&img, 2, cy - 5, 1, ojo);
    ImageDrawRectangle(&img, 0, cy - 2, 3, 2, colmillo);

    ImageDrawRectangle(&img, 5, cy + 8, 3, 5, pelajeOsc);
    ImageDrawRectangle(&img, 14, cy + 8, 3, 5, pelajeOsc);

    ImageDrawLineEx(&img, Vector2{ 19, (float)cy }, Vector2{ 20, (float)(cy - 9) }, 2, cola);

    return img;
}

// -- Carcel --

Image CrearPresoAmotinado() {
    // "Comun agresivo" de la Carcel: uniforme de preso claro con rayas,
    // pelo desprolijo, un grillete roto en la muñeca (senal del motin) y un
    // shiv improvisado en la mano.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 190, 150, 120, 255 };
    Color pelo = { 40, 35, 30, 255 };
    Color tela = { 200, 195, 180, 255 };
    Color telaOsc = { 150, 145, 130, 255 };
    Color venda = { 180, 75, 60, 255 };
    Color shiv = { 170, 170, 180, 255 };
    Color cadena = { 90, 90, 95, 255 };
    Color bota = { 60, 55, 50, 255 };

    ImageDrawCircle(&img, 10, 8, 5, piel);
    ImageDrawRectangle(&img, 5, 3, 10, 4, pelo);
    ImageDrawRectangle(&img, 7, 9, 2, 1, venda);

    ImageDrawRectangle(&img, 4, 13, 12, 9, tela);
    ImageDrawRectangle(&img, 4, 13, 12, 2, telaOsc);
    ImageDrawRectangle(&img, 4, 17, 12, 2, telaOsc);

    ImageDrawRectangle(&img, 2, 18, 3, 2, cadena);

    ImageDrawLineEx(&img, Vector2{ 17, 21 }, Vector2{ 19, 15 }, 2, shiv);

    ImageDrawRectangle(&img, 6, 22, 3, 4, bota);
    ImageDrawRectangle(&img, 11, 22, 3, 4, bota);

    return img;
}

Image CrearGuardiaCorrupto() {
    // "Comun especial" de la Carcel: a veces aplica Aturdido (ver
    // game::AtaqueEspecialDe). Uniforme azul-grisaceo con gorra de visera,
    // porra, y una moneda robada prendida como si fuera una insignia (guiño
    // a que este guardia mira para otro lado a cambio de sobornos).
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 200, 160, 130, 255 };
    Color gorra = { 55, 60, 75, 255 };
    Color gorraOsc = { 38, 42, 55, 255 };
    Color uniforme = { 70, 75, 90, 255 };
    Color uniformeOsc = { 50, 54, 66, 255 };
    Color porra = { 90, 65, 40, 255 };
    Color moneda = { 215, 180, 90, 255 };

    ImageDrawCircle(&img, 10, 8, 5, piel);
    ImageDrawRectangle(&img, 4, 3, 12, 4, gorra);
    ImageDrawRectangle(&img, 4, 6, 12, 2, gorraOsc);
    ImageDrawRectangle(&img, 7, 9, 2, 1, Color{ 25, 25, 30, 255 });
    ImageDrawRectangle(&img, 11, 9, 2, 1, Color{ 25, 25, 30, 255 });

    ImageDrawRectangle(&img, 4, 13, 12, 9, uniforme);
    ImageDrawRectangle(&img, 4, 13, 12, 2, uniformeOsc);
    ImageDrawCircle(&img, 10, 18, 2, moneda);

    ImageDrawLineEx(&img, Vector2{ 17, 22 }, Vector2{ 16, 12 }, 3, porra);

    ImageDrawRectangle(&img, 6, 22, 3, 4, Color{ 35, 32, 38, 255 });
    ImageDrawRectangle(&img, 11, 22, 3, 4, Color{ 35, 32, 38, 255 });

    return img;
}

Image CrearAlcaide() {
    // Jefe de la Carcel: abrigo largo oscuro con charreteras rojas, aro de
    // llaves colgando del cinto y una maza pesada en vez del garrote/porra
    // de la tropa comun — se nota a simple vista que es el que manda.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 195, 155, 125, 255 };
    Color abrigo = { 45, 40, 48, 255 };
    Color abrigoOsc = { 30, 27, 33, 255 };
    Color charreteras = { 150, 40, 40, 255 };
    Color llaves = { 215, 180, 90, 255 };
    Color maza = { 95, 93, 97, 255 };
    Color mazaOsc = { 60, 58, 62, 255 };

    ImageDrawCircle(&img, 10, 8, 5, piel);
    ImageDrawRectangle(&img, 4, 2, 12, 5, Color{ 35, 32, 38, 255 });
    ImageDrawRectangle(&img, 4, 6, 12, 2, Color{ 20, 18, 22, 255 });
    ImageDrawRectangle(&img, 7, 9, 2, 2, Color{ 20, 20, 25, 255 });
    ImageDrawRectangle(&img, 11, 9, 2, 2, Color{ 20, 20, 25, 255 });

    ImageDrawRectangle(&img, 3, 13, 14, 9, abrigo);
    ImageDrawRectangle(&img, 3, 13, 14, 2, abrigoOsc);
    ImageDrawRectangle(&img, 3, 13, 3, 4, charreteras);
    ImageDrawRectangle(&img, 14, 13, 3, 4, charreteras);

    ImageDrawCircleLines(&img, 6, 20, 2, llaves);
    ImageDrawCircleLines(&img, 9, 21, 2, llaves);

    ImageDrawLineEx(&img, Vector2{ 18, 24 }, Vector2{ 18, 10 }, 2, mazaOsc);
    ImageDrawRectangle(&img, 15, 5, 7, 6, maza);
    ImageDrawRectangleLines(&img, Rectangle{ 15, 5, 7, 6 }, 1, mazaOsc);

    ImageDrawRectangle(&img, 5, 22, 4, 4, Color{ 30, 28, 32, 255 });
    ImageDrawRectangle(&img, 11, 22, 4, 4, Color{ 30, 28, 32, 255 });

    return img;
}

// -- Castillo --

Image CrearGuardiaReal() {
    // "Comun agresivo" del Castillo: armadura plateada con sobreveste azul
    // y ribete dorado, cresta de plumero en el casco, y una lanza larga en
    // vez de un arma cuerpo a cuerpo corta.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 225, 185, 150, 255 };
    Color plata = { 200, 205, 215, 255 };
    Color plataOsc = { 150, 155, 168, 255 };
    Color azul = { 60, 90, 170, 255 };
    Color oro = { 215, 180, 90, 255 };
    Color lanza = { 210, 210, 220, 255 };

    ImageDrawCircle(&img, 10, 8, 5, piel);
    ImageDrawRectangle(&img, 4, 2, 12, 6, plata);
    ImageDrawRectangle(&img, 4, 7, 12, 2, plataOsc);
    ImageDrawTriangle(&img, Vector2{ 8, 2 }, Vector2{ 12, 2 }, Vector2{ 10, -3 }, oro);
    ImageDrawRectangle(&img, 7, 9, 2, 2, Color{ 20, 20, 25, 255 });
    ImageDrawRectangle(&img, 11, 9, 2, 2, Color{ 20, 20, 25, 255 });

    ImageDrawRectangle(&img, 4, 13, 12, 8, plata);
    ImageDrawRectangle(&img, 5, 14, 10, 6, azul);
    ImageDrawRectangle(&img, 9, 14, 2, 6, oro);

    ImageDrawLineEx(&img, Vector2{ 18, 24 }, Vector2{ 18, 2 }, 2, lanza);
    ImageDrawTriangle(&img, Vector2{ 16, 2 }, Vector2{ 20, 2 }, Vector2{ 18, -4 }, plataOsc);

    ImageDrawRectangle(&img, 6, 21, 3, 5, plataOsc);
    ImageDrawRectangle(&img, 11, 21, 3, 5, plataOsc);

    return img;
}

Image CrearMagoDeLaCorte() {
    // "Comun especial" del Castillo: a veces aplica Debilitado (ver
    // game::AtaqueEspecialDe). Tunica violeta cortesana con franja dorada,
    // sombrero puntiagudo de ala ancha, baston con un orbe celeste.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 220, 180, 145, 255 };
    Color capucha = { 95, 55, 140, 255 };
    Color capuchaOsc = { 65, 38, 100, 255 };
    Color tunica = { 70, 40, 105, 255 };
    Color tunicaOsc = { 48, 28, 75, 255 };
    Color oro = { 215, 180, 90, 255 };
    Color orbe = { 120, 200, 220, 255 };
    Color baston = { 110, 80, 55, 255 };

    ImageDrawCircle(&img, 10, 8, 5, piel);
    ImageDrawTriangle(&img, Vector2{ 3, 8 }, Vector2{ 17, 8 }, Vector2{ 10, -6 }, capucha);
    ImageDrawRectangle(&img, 5, 7, 10, 2, capuchaOsc);
    ImageDrawRectangle(&img, 8, 9, 4, 2, piel);

    ImageDrawRectangle(&img, 5, 13, 10, 5, tunica);
    ImageDrawRectangle(&img, 3, 18, 14, 5, tunica);
    ImageDrawRectangle(&img, 1, 22, 18, 3, tunicaOsc);
    ImageDrawRectangle(&img, 9, 13, 2, 12, oro);

    ImageDrawLineEx(&img, Vector2{ 17, 23 }, Vector2{ 17, 5 }, 2, baston);
    ImageDrawCircle(&img, 17, 4, 3, orbe);

    return img;
}

Image CrearCapitanDeLaGuardia() {
    // Jefe del Castillo: armadura mas grande y ornamentada, cresta/capa
    // roja real, franja dorada al medio y una espada larga — mismo
    // criterio que ya distinguia al Capitan Bandido de la tropa comun.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color piel = { 220, 180, 145, 255 };
    Color plata = { 210, 215, 225, 255 };
    Color plataOsc = { 155, 160, 175, 255 };
    Color capa = { 150, 25, 35, 255 };
    Color capaOsc = { 105, 16, 24, 255 };
    Color oro = { 225, 190, 100, 255 };
    Color espada = { 215, 215, 225, 255 };
    Color empunadura = { 110, 80, 55, 255 };

    ImageDrawCircle(&img, 11, 8, 5, piel);
    ImageDrawRectangle(&img, 4, 2, 14, 6, plata);
    ImageDrawRectangle(&img, 4, 7, 14, 2, plataOsc);
    ImageDrawTriangle(&img, Vector2{ 7, 2 }, Vector2{ 15, 2 }, Vector2{ 11, -6 }, capa);
    ImageDrawRectangle(&img, 8, 9, 2, 2, Color{ 20, 20, 25, 255 });
    ImageDrawRectangle(&img, 12, 9, 2, 2, Color{ 20, 20, 25, 255 });

    ImageDrawRectangle(&img, 3, 13, 16, 9, plata);
    ImageDrawRectangle(&img, 3, 13, 16, 2, plataOsc);
    ImageDrawTriangle(&img, Vector2{ 2, 26 }, Vector2{ 19, 26 }, Vector2{ 11, 13 }, capaOsc);
    ImageDrawRectangle(&img, 10, 13, 2, 9, oro);

    ImageDrawLineEx(&img, Vector2{ 19, 24 }, Vector2{ 19, 3 }, 2, espada);
    ImageDrawRectangle(&img, 16, 15, 6, 2, empunadura);

    ImageDrawRectangle(&img, 5, 22, 4, 4, Color{ 35, 32, 38, 255 });
    ImageDrawRectangle(&img, 12, 22, 4, 4, Color{ 35, 32, 38, 255 });

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
// pieza individual. Las mismas 4 formas sirven para los 3 temas del mapa —
// solo cambia el tinte con el que se dibujan (ver TinteDecoracionPorTema).

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
// Se reusa igual en los 3 temas (una mazmorra de bosque o de castillo sigue
// siendo un interior con antorchas, no cambia el gameplay ni pide una
// variante propia).
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

// --- Tiles de mapa: un piso y una pared por tema ---
// Mismo enfoque de siempre (manchas sutiles en el piso, dos hiladas de
// ladrillo con aparejo en la pared) con la paleta y un detalle chico de
// decoracion propios de cada tema — el pedido explicito del usuario fue
// "reemplazo de paleta y decoracion" con la MISMA tecnica de generacion, no
// formas de tile nuevas.

Image CrearTilePisoBosque() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, Color{ 42, 48, 34, 255 });
    Color claro = { 58, 66, 42, 255 };
    Color oscuro = { 30, 36, 24, 255 };
    Color raiz = { 70, 55, 35, 255 };

    ImageDrawRectangle(&img, 1, 2, 4, 3, claro);
    ImageDrawRectangle(&img, 10, 9, 5, 4, oscuro);
    ImageDrawRectangle(&img, 3, 11, 3, 2, oscuro);
    ImageDrawRectangle(&img, 12, 2, 2, 2, claro);
    ImageDrawLineEx(&img, Vector2{ 0, 14 }, Vector2{ 6, 12 }, 1, raiz);  // raiz fina cruzando el piso

    return img;
}

Image CrearTileParedBosque() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, Color{ 55, 62, 46, 255 });
    Color piedraClara = { 72, 80, 58, 255 };
    Color mortero = { 34, 40, 28, 255 };
    Color musgo = { 60, 95, 50, 200 };

    ImageDrawRectangle(&img, 0, 0, 16, 1, mortero);
    ImageDrawRectangle(&img, 0, 7, 16, 2, mortero);
    ImageDrawRectangle(&img, 0, 15, 16, 1, mortero);
    ImageDrawRectangle(&img, 7, 0, 1, 8, mortero);
    ImageDrawRectangle(&img, 3, 8, 1, 8, mortero);
    ImageDrawRectangle(&img, 11, 8, 1, 8, mortero);

    ImageDrawRectangle(&img, 1, 1, 6, 6, piedraClara);
    ImageDrawRectangle(&img, 8, 1, 7, 6, piedraClara);
    ImageDrawRectangle(&img, 4, 9, 7, 6, piedraClara);
    ImageDrawRectangle(&img, 12, 9, 4, 6, piedraClara);
    ImageDrawRectangle(&img, 0, 9, 3, 6, piedraClara);

    ImageDrawCircle(&img, 3, 3, 2, musgo);   // parches de musgo colgando
    ImageDrawCircle(&img, 13, 12, 2, musgo);

    return img;
}

Image CrearTilePisoCarcel() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, Color{ 48, 48, 52, 255 });
    Color claro = { 60, 60, 65, 255 };
    Color oscuro = { 36, 36, 40, 255 };

    ImageDrawRectangle(&img, 1, 2, 4, 3, claro);
    ImageDrawRectangle(&img, 10, 9, 5, 4, oscuro);
    ImageDrawRectangle(&img, 3, 11, 3, 2, oscuro);
    ImageDrawRectangle(&img, 12, 2, 2, 2, claro);
    ImageDrawRectangleLines(&img, Rectangle{ 6, 6, 4, 4 }, 1, oscuro);  // rejilla de desague
    ImageDrawLine(&img, 6, 8, 10, 8, oscuro);

    return img;
}

Image CrearTileParedCarcel() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, Color{ 58, 58, 62, 255 });
    Color piedraClara = { 76, 76, 82, 255 };
    Color mortero = { 32, 32, 36, 255 };
    Color hierro = { 25, 25, 28, 255 };

    ImageDrawRectangle(&img, 0, 0, 16, 1, mortero);
    ImageDrawRectangle(&img, 0, 7, 16, 2, mortero);
    ImageDrawRectangle(&img, 0, 15, 16, 1, mortero);
    ImageDrawRectangle(&img, 7, 0, 1, 8, mortero);
    ImageDrawRectangle(&img, 3, 8, 1, 8, mortero);
    ImageDrawRectangle(&img, 11, 8, 1, 8, mortero);

    ImageDrawRectangle(&img, 1, 1, 6, 6, piedraClara);
    ImageDrawRectangle(&img, 8, 1, 7, 6, piedraClara);
    ImageDrawRectangle(&img, 4, 9, 7, 6, piedraClara);
    ImageDrawRectangle(&img, 12, 9, 4, 6, piedraClara);
    ImageDrawRectangle(&img, 0, 9, 3, 6, piedraClara);

    ImageDrawRectangle(&img, 5, 2, 1, 5, hierro);   // barrotes finos
    ImageDrawRectangle(&img, 9, 2, 1, 5, hierro);

    return img;
}

Image CrearTilePisoCastillo() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, Color{ 58, 54, 50, 255 });
    Color claro = { 72, 68, 62, 255 };
    Color oscuro = { 44, 40, 36, 255 };
    Color alfombra = { 120, 35, 40, 200 };

    ImageDrawRectangle(&img, 1, 2, 4, 3, claro);
    ImageDrawRectangle(&img, 10, 9, 5, 4, oscuro);
    ImageDrawRectangle(&img, 3, 11, 3, 2, oscuro);
    ImageDrawRectangle(&img, 12, 2, 2, 2, claro);
    ImageDrawRectangle(&img, 6, 0, 4, 16, alfombra);   // franja de alfombra

    return img;
}

Image CrearTileParedCastillo() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, Color{ 80, 70, 58, 255 });
    Color piedraClara = { 104, 92, 76, 255 };
    Color mortero = { 55, 46, 36, 255 };
    Color oro = { 190, 155, 80, 200 };

    ImageDrawRectangle(&img, 0, 0, 16, 1, mortero);
    ImageDrawRectangle(&img, 0, 7, 16, 2, mortero);
    ImageDrawRectangle(&img, 0, 15, 16, 1, mortero);
    ImageDrawRectangle(&img, 7, 0, 1, 8, mortero);
    ImageDrawRectangle(&img, 3, 8, 1, 8, mortero);
    ImageDrawRectangle(&img, 11, 8, 1, 8, mortero);

    ImageDrawRectangle(&img, 1, 1, 6, 6, piedraClara);
    ImageDrawRectangle(&img, 8, 1, 7, 6, piedraClara);
    ImageDrawRectangle(&img, 4, 9, 7, 6, piedraClara);
    ImageDrawRectangle(&img, 12, 9, 4, 6, piedraClara);
    ImageDrawRectangle(&img, 0, 9, 3, 6, piedraClara);

    ImageDrawRectangle(&img, 0, 7, 16, 1, oro);   // filete dorado entre hiladas

    return img;
}

// --- Ciudad: piso, pared y decoracion propios ---
// La primera version de la Ciudad reusaba TilePiso/TileParedCastillo (ver
// kTemaCiudad en sprites.h) -- descartado tras feedback directo del usuario
// ("parece una mazmorra mas, quiero que tenga estetica de ciudad, que sea
// un bioma mas"). Paleta calida de adoquin/tapia en vez de piedra de
// mazmorra, con un seto prolijo arriba de la pared en vez de otra hilada de
// piedra, que es lo que mas la distingue a simple vista.

Image CrearTilePisoCiudad() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, Color{ 152, 144, 130, 255 });
    Color claro = { 172, 164, 148, 255 };
    Color oscuro = { 118, 110, 96, 255 };
    Color pastito = { 92, 128, 58, 255 };

    ImageDrawRectangle(&img, 1, 1, 6, 5, claro);
    ImageDrawRectangle(&img, 9, 2, 6, 5, oscuro);
    ImageDrawRectangle(&img, 2, 9, 5, 6, oscuro);
    ImageDrawRectangle(&img, 10, 10, 5, 5, claro);
    ImageDrawLineEx(&img, Vector2{ 7, 14 }, Vector2{ 9, 12 }, 1, pastito);  // pastito creciendo en una junta

    return img;
}

Image CrearTileParedCiudad() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, Color{ 196, 162, 120, 255 });
    Color claro = { 214, 182, 138, 255 };
    Color mortero = { 158, 128, 92, 255 };
    Color seto = { 70, 110, 48, 255 };
    Color setoOsc = { 50, 84, 34, 255 };

    ImageDrawRectangle(&img, 0, 5, 16, 11, claro);
    ImageDrawRectangle(&img, 0, 10, 16, 1, mortero);
    ImageDrawRectangle(&img, 0, 5, 8, 1, mortero);
    ImageDrawRectangle(&img, 8, 5, 1, 6, mortero);

    // Seto arriba de la tapia -- lo que mas la distingue de una pared de
    // mazmorra a simple vista.
    ImageDrawRectangle(&img, 0, 0, 16, 6, seto);
    ImageDrawCircle(&img, 2, 4, 2, setoOsc);
    ImageDrawCircle(&img, 7, 3, 2, setoOsc);
    ImageDrawCircle(&img, 12, 4, 2, setoOsc);

    return img;
}

Image CrearPastoCiudad() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, BLANK);
    Color pasto = { 92, 140, 58, 255 };
    Color pastoOsc = { 66, 106, 40, 255 };

    ImageDrawLineEx(&img, Vector2{ 5, 14 }, Vector2{ 4, 8 }, 1, pasto);
    ImageDrawLineEx(&img, Vector2{ 7, 14 }, Vector2{ 8, 7 }, 1, pastoOsc);
    ImageDrawLineEx(&img, Vector2{ 9, 14 }, Vector2{ 10, 9 }, 1, pasto);
    ImageDrawLineEx(&img, Vector2{ 11, 14 }, Vector2{ 12, 8 }, 1, pastoOsc);

    return img;
}

Image CrearMacetaCiudad() {
    Image img = GenImageColor(kCanvasTile, kCanvasTile, BLANK);
    Color barro = { 168, 96, 62, 255 };
    Color barroOsc = { 128, 70, 44, 255 };
    Color hoja = { 80, 128, 54, 255 };
    Color flor = { 220, 90, 110, 255 };

    ImageDrawTriangle(&img, Vector2{ 4, 15 }, Vector2{ 12, 15 }, Vector2{ 10, 9 }, barro);
    ImageDrawTriangle(&img, Vector2{ 4, 15 }, Vector2{ 6, 9 }, Vector2{ 10, 9 }, barroOsc);
    ImageDrawRectangle(&img, 3, 8, 10, 2, barroOsc);

    ImageDrawCircle(&img, 8, 5, 4, hoja);
    ImageDrawCircle(&img, 6, 4, 2, flor);
    ImageDrawCircle(&img, 10, 3, 2, flor);

    return img;
}

// --- Deambulantes de la Ciudad (perro, pajaro, aldeanos) ---
// Puramente decorativos (ver game::Deambulante) -- pedido directo del
// usuario tras ver la primera version de la Ciudad ("quiero que tenga vida,
// que se mueva un perro, algunos pajaros").

Image CrearPerro() {
    // Mismo plan de cuerpo cuadrupedo que el Lobo Salvaje (ver
    // CrearLoboSalvaje) pero paleta calida, oreja caida (en vez de parada) y
    // cola curva hacia arriba (en vez de recta) -- para que se lea como
    // mascota a primera vista, no como depredador. Nunca aparece en combate.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color pelaje = { 158, 118, 74, 255 };
    Color pelajeOsc = { 120, 86, 50, 255 };
    Color panza = { 226, 205, 172, 255 };
    Color ojo = { 40, 30, 24, 255 };
    Color nariz = { 30, 24, 20, 255 };
    Color lengua = { 210, 110, 120, 255 };
    Color cola = { 140, 102, 62, 255 };

    int cy = 17;

    ImageDrawTriangle(&img, Vector2{ 2, (float)(cy - 6) }, Vector2{ 6, (float)(cy - 7) }, Vector2{ 3, (float)(cy + 1) }, pelajeOsc);

    ImageDrawCircle(&img, 10, cy, 7, pelaje);
    ImageDrawCircle(&img, 10, cy + 3, 4, panza);
    ImageDrawCircle(&img, 4, cy - 3, 4, pelaje);
    ImageDrawCircle(&img, 2, cy - 4, 1, ojo);
    ImageDrawCircle(&img, 0, cy - 2, 1, nariz);
    ImageDrawRectangle(&img, 1, cy - 1, 2, 2, lengua);

    ImageDrawRectangle(&img, 5, cy + 6, 2, 4, pelajeOsc);
    ImageDrawRectangle(&img, 13, cy + 6, 2, 4, pelajeOsc);

    ImageDrawLineEx(&img, Vector2{ 17, (float)cy }, Vector2{ 19, (float)(cy - 5) }, 2, cola);
    ImageDrawLineEx(&img, Vector2{ 19, (float)(cy - 5) }, Vector2{ 16, (float)(cy - 8) }, 2, cola);

    return img;
}

Image CrearPajaro() {
    // Paleta base neutra a proposito: se tine distinto por instancia al
    // dibujarlo (ver TintePajaroPorIndice en renderer.cpp) en vez de sumar
    // una textura por variante para solo 3 pajaros.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);

    Color cuerpo = { 210, 195, 175, 255 };
    Color cuerpoOsc = { 170, 155, 138, 255 };
    Color pico = { 235, 175, 60, 255 };
    Color ojo = { 30, 26, 22, 255 };
    Color pata = { 200, 150, 60, 255 };

    int cx = 10, cy = 20;

    ImageDrawCircle(&img, cx, cy, 5, cuerpo);
    ImageDrawCircle(&img, cx - 4, cy - 3, 3, cuerpo);
    ImageDrawTriangle(&img, Vector2{ (float)cx - 7, (float)(cy - 4) }, Vector2{ (float)cx - 11, (float)(cy - 3) }, Vector2{ (float)cx - 7, (float)(cy - 2) }, pico);
    ImageDrawCircle(&img, cx - 5, cy - 4, 1, ojo);
    ImageDrawTriangle(&img, Vector2{ (float)cx + 2, (float)(cy - 2) }, Vector2{ (float)cx + 8, (float)(cy - 5) }, Vector2{ (float)cx + 3, (float)(cy + 2) }, cuerpoOsc);
    ImageDrawRectangle(&img, cx - 1, cy + 4, 1, 3, pata);
    ImageDrawRectangle(&img, cx + 2, cy + 4, 1, 3, pata);

    return img;
}

Image CrearAldeanoA() {
    // Campesino: tunica verde, sin nada en la cabeza mas que el pelo.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);
    Color piel = { 222, 180, 140, 255 };
    Color pelo = { 90, 60, 35, 255 };
    Color tunica = { 96, 130, 70, 255 };
    Color tunicaOsc = { 68, 98, 48, 255 };
    Color pantalon = { 92, 74, 52, 255 };
    Color bota = { 55, 42, 30, 255 };

    ImageDrawCircle(&img, 10, 8, 5, piel);
    ImageDrawRectangle(&img, 5, 2, 10, 4, pelo);

    ImageDrawRectangle(&img, 4, 13, 12, 8, tunica);
    ImageDrawRectangle(&img, 4, 13, 12, 2, tunicaOsc);

    ImageDrawRectangle(&img, 6, 21, 3, 5, pantalon);
    ImageDrawRectangle(&img, 11, 21, 3, 5, pantalon);
    ImageDrawRectangle(&img, 6, 24, 3, 2, bota);
    ImageDrawRectangle(&img, 11, 24, 3, 2, bota);

    return img;
}

Image CrearAldeanoB() {
    // Comerciante: tunica roja con delantal claro.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);
    Color piel = { 210, 165, 125, 255 };
    Color pelo = { 40, 36, 34, 255 };
    Color tunica = { 168, 62, 54, 255 };
    Color tunicaOsc = { 128, 44, 40, 255 };
    Color delantal = { 214, 198, 170, 255 };
    Color pantalon = { 58, 52, 48, 255 };
    Color bota = { 40, 34, 30, 255 };

    ImageDrawCircle(&img, 10, 8, 5, piel);
    ImageDrawCircle(&img, 10, 5, 5, pelo);
    ImageDrawRectangle(&img, 6, 9, 8, 2, piel);

    ImageDrawRectangle(&img, 4, 13, 12, 8, tunica);
    ImageDrawRectangle(&img, 4, 13, 12, 2, tunicaOsc);
    ImageDrawRectangle(&img, 7, 15, 6, 6, delantal);

    ImageDrawRectangle(&img, 6, 21, 3, 5, pantalon);
    ImageDrawRectangle(&img, 11, 21, 3, 5, pantalon);
    ImageDrawRectangle(&img, 6, 24, 3, 2, bota);
    ImageDrawRectangle(&img, 11, 24, 3, 2, bota);

    return img;
}

Image CrearAldeanoC() {
    // Viajero con capucha azul-grisacea.
    Image img = GenImageColor(kCanvasPersonaje, kCanvasPersonajeAlto, BLANK);
    Color piel = { 200, 160, 122, 255 };
    Color capucha = { 78, 92, 108, 255 };
    Color capuchaOsc = { 54, 66, 80, 255 };
    Color capa = { 88, 100, 116, 255 };
    Color bota = { 42, 40, 42, 255 };

    ImageDrawCircle(&img, 10, 9, 5, piel);
    ImageDrawCircle(&img, 10, 7, 6, capucha);
    ImageDrawRectangle(&img, 5, 10, 10, 3, capuchaOsc);
    ImageDrawRectangle(&img, 7, 10, 6, 3, piel);

    ImageDrawRectangle(&img, 4, 13, 12, 9, capa);
    ImageDrawRectangle(&img, 4, 13, 12, 2, capuchaOsc);

    ImageDrawRectangle(&img, 6, 22, 3, 4, bota);
    ImageDrawRectangle(&img, 11, 22, 3, 4, bota);

    return img;
}

Texture2D CargarPixelPerfecto(Image img) {
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

}  // namespace

Color TinteDecoracionPorTema(int tema) {
    switch (((tema % kNumTemas) + kNumTemas) % kNumTemas) {
        case 0:  return Color{ 200, 230, 190, 255 };  // Bosque: verdoso
        case 1:  return Color{ 200, 200, 210, 255 };  // Carcel: gris frio
        default: return Color{ 230, 210, 170, 255 };  // Castillo: calido/dorado
    }
}

SpriteSet::SpriteSet() {
    personajes_[static_cast<int>(game::Role::Tanque)] = CargarPixelPerfecto(CrearTanque());
    personajes_[static_cast<int>(game::Role::Danio)] = CargarPixelPerfecto(CrearDanio());
    personajes_[static_cast<int>(game::Role::Soporte)] = CargarPixelPerfecto(CrearSoporte());
    personajes_[static_cast<int>(game::Role::Control)] = CargarPixelPerfecto(CrearControl());

    enemigos_[static_cast<int>(game::TipoEnemigo::LoboSalvaje)] = CargarPixelPerfecto(CrearLoboSalvaje());
    enemigos_[static_cast<int>(game::TipoEnemigo::AranaGigante)] = CargarPixelPerfecto(CrearAranaGigante());
    enemigos_[static_cast<int>(game::TipoEnemigo::AlfaDelBosque)] = CargarPixelPerfecto(CrearAlfaDelBosque());
    enemigos_[static_cast<int>(game::TipoEnemigo::PresoAmotinado)] = CargarPixelPerfecto(CrearPresoAmotinado());
    enemigos_[static_cast<int>(game::TipoEnemigo::GuardiaCorrupto)] = CargarPixelPerfecto(CrearGuardiaCorrupto());
    enemigos_[static_cast<int>(game::TipoEnemigo::Alcaide)] = CargarPixelPerfecto(CrearAlcaide());
    enemigos_[static_cast<int>(game::TipoEnemigo::GuardiaReal)] = CargarPixelPerfecto(CrearGuardiaReal());
    enemigos_[static_cast<int>(game::TipoEnemigo::MagoDeLaCorte)] = CargarPixelPerfecto(CrearMagoDeLaCorte());
    enemigos_[static_cast<int>(game::TipoEnemigo::CapitanDeLaGuardia)] = CargarPixelPerfecto(CrearCapitanDeLaGuardia());

    cofreCerrado_ = CargarPixelPerfecto(CrearCofre(false));
    cofreAbierto_ = CargarPixelPerfecto(CrearCofre(true));

    decoracionesPiso_[0] = CargarPixelPerfecto(CrearGrietaSuelo());
    decoracionesPiso_[1] = CargarPixelPerfecto(CrearMusgoSuelo());
    decoracionesPiso_[2] = CargarPixelPerfecto(CrearEscombrosSuelo());
    decoracionesPiso_[3] = CargarPixelPerfecto(CrearCharcoSuelo());
    antorcha_ = CargarPixelPerfecto(CrearAntorcha());

    trampas_[static_cast<int>(game::TipoTrampa::Fuego)] = CargarPixelPerfecto(CrearTrampaFuego());
    trampas_[static_cast<int>(game::TipoTrampa::Acido)] = CargarPixelPerfecto(CrearTrampaAcido());

    tilePiso_[0] = CargarPixelPerfecto(CrearTilePisoBosque());
    tilePiso_[1] = CargarPixelPerfecto(CrearTilePisoCarcel());
    tilePiso_[2] = CargarPixelPerfecto(CrearTilePisoCastillo());
    tilePared_[0] = CargarPixelPerfecto(CrearTileParedBosque());
    tilePared_[1] = CargarPixelPerfecto(CrearTileParedCarcel());
    tilePared_[2] = CargarPixelPerfecto(CrearTileParedCastillo());
    for (auto& tex : tilePiso_) SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    for (auto& tex : tilePared_) SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);

    tilePisoCiudad_ = CargarPixelPerfecto(CrearTilePisoCiudad());
    tileParedCiudad_ = CargarPixelPerfecto(CrearTileParedCiudad());
    SetTextureWrap(tilePisoCiudad_, TEXTURE_WRAP_REPEAT);
    SetTextureWrap(tileParedCiudad_, TEXTURE_WRAP_REPEAT);
    decoracionesCiudad_[0] = CargarPixelPerfecto(CrearPastoCiudad());
    decoracionesCiudad_[1] = CargarPixelPerfecto(CrearMacetaCiudad());

    deambulantes_[static_cast<int>(game::TipoDeambulante::Perro)] = CargarPixelPerfecto(CrearPerro());
    deambulantes_[static_cast<int>(game::TipoDeambulante::Pajaro)] = CargarPixelPerfecto(CrearPajaro());
    deambulantes_[static_cast<int>(game::TipoDeambulante::AldeanoA)] = CargarPixelPerfecto(CrearAldeanoA());
    deambulantes_[static_cast<int>(game::TipoDeambulante::AldeanoB)] = CargarPixelPerfecto(CrearAldeanoB());
    deambulantes_[static_cast<int>(game::TipoDeambulante::AldeanoC)] = CargarPixelPerfecto(CrearAldeanoC());
}

SpriteSet::~SpriteSet() {
    for (auto& tex : personajes_) UnloadTexture(tex);
    for (auto& tex : enemigos_) UnloadTexture(tex);
    UnloadTexture(cofreCerrado_);
    UnloadTexture(cofreAbierto_);
    for (auto& tex : tilePiso_) UnloadTexture(tex);
    for (auto& tex : tilePared_) UnloadTexture(tex);
    for (auto& tex : decoracionesPiso_) UnloadTexture(tex);
    UnloadTexture(antorcha_);
    for (auto& tex : trampas_) UnloadTexture(tex);

    UnloadTexture(tilePisoCiudad_);
    UnloadTexture(tileParedCiudad_);
    for (auto& tex : decoracionesCiudad_) UnloadTexture(tex);
    for (auto& tex : deambulantes_) UnloadTexture(tex);
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
