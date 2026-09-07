#include "raylib.h"
#include <vector>
#include <utility>
#include <memory>
#include <string>
#include <algorithm>

#include "game/character.h"
#include "game/party.h"
#include "game/dungeon.h"
#include "game/enemy.h"
#include "game/combat.h"
#include "game/dice.h"
#include "game/item.h"
#include "game/edificio.h"
#include "game/deambulante.h"
#include "game/save.h"
#include "render/renderer.h"
#include "render/input.h"
#include "render/combat_ui.h"
#include "render/inventory_ui.h"
#include "render/character_sheet_ui.h"
#include "render/audio.h"
#include "render/menu_ui.h"

namespace {

// Dificultad de una mazmorra del mapa (ver EstadoJuego::MapaDificultad mas
// abajo). El orden de este enum importa: coincide por CONVENCION (no por
// dependencia de header — render/menu_ui.h no incluye main.cpp) con el
// orden fijo Facil/Media/Dificil que dibuja ui::DibujarMapaDificultad, y es
// el "resto" (indice % game::kNumDificultadesMapa) del indice combinado que
// se guarda en mazmorraSuperada/mazmorraActivaIndice (ver game/save.h).
enum class Dificultad { Facil, Media, Dificil };

// Tema de una mazmorra del mapa (ver EstadoJuego::MapaTema/MapaDificultad
// mas abajo). Igual que Dificultad, el orden de este enum importa: coincide
// por CONVENCION (no por dependencia de header — render/menu_ui.h no
// incluye main.cpp) con el orden fijo Bosque/Carcel/Castillo que dibuja
// ui::DibujarMapaTema, y con el "tercio" (indice / kNumDificultadesMapa) del
// indice combinado que se guarda en mazmorraSuperada/mazmorraActivaIndice
// (ver game::kNumCombinacionesMapa en save.h). Tambien coincide con el
// orden de game::TipoEnemigo en enemy.h (los primeros 3 valores son de
// Bosque, los siguientes 3 de Carcel, los ultimos 3 de Castillo) — asi que
// static_cast<int>(tema) es directamente el "tercio" de enemigos de ese
// tema, ver CrearEnemigoDeTipo/TipoComunAleatorio/GenerarMazmorra.
enum class Tema { Bosque, Carcel, Castillo };
constexpr int kNumTemas = 3;

// Indice combinado tema*game::kNumDificultadesMapa + dificultad, el mismo
// que usa game::DatosPartida::mazmorraActivaIndice/mazmorraSuperada (ver
// save.h) para identificar una de las 9 mazmorras del mapa con un unico
// entero -1..8.
int IndiceCombinado(Tema tema, Dificultad dificultad) {
    return static_cast<int>(tema) * game::kNumDificultadesMapa + static_cast<int>(dificultad);
}
Tema TemaDeIndiceCombinado(int indice) { return static_cast<Tema>(indice / game::kNumDificultadesMapa); }
Dificultad DificultadDeIndiceCombinado(int indice) { return static_cast<Dificultad>(indice % game::kNumDificultadesMapa); }

// Multiplicadores de hp/ataque de un enemigo (incluido el jefe) segun la
// dificultad elegida en el mapa — defensa y velocidad quedan sin escalar a
// proposito: "mas dificil" tenia que sentirse en cuanto aguantan y pegan los
// enemigos, no en que esquiven distinto o se muevan a otra velocidad.
struct EscalaDificultad { float hp; float ataque; };
EscalaDificultad EscalaDe(Dificultad dificultad) {
    switch (dificultad) {
        case Dificultad::Facil:   return EscalaDificultad{0.8f, 0.85f};
        case Dificultad::Dificil: return EscalaDificultad{1.25f, 1.15f};
        default:                  return EscalaDificultad{1.0f, 1.0f};  // Media, igual que antes de Round G
    }
}

game::Party CrearPartyDeEjemplo(game::Vec2 posicionInicial) {
    using game::Character;
    using game::Role;
    using game::Stats;

    std::vector<Character> miembros;
    // recursoMax ya no es 0: ahora Tanque tambien gasta Resistencia al usar
    // Golpe Provocador (ver combat.cpp::EjecutarHabilidadDeRol) — antes era
    // gratis porque no habia economia de recurso para este rol.
    miembros.emplace_back("Bruna", Role::Tanque,
        Stats{ /*hpMax*/30, /*hp*/30, /*recursoMax*/20, /*recurso*/20, /*ataque*/4, /*defensa*/6, /*velocidad*/90.0f },
        posicionInicial);
    miembros.emplace_back("Kael", Role::Danio,
        Stats{ /*hpMax*/18, /*hp*/18, /*recursoMax*/20, /*recurso*/20, /*ataque*/9, /*defensa*/2, /*velocidad*/120.0f },
        posicionInicial);
    miembros.emplace_back("Sara", Role::Soporte,
        Stats{ /*hpMax*/16, /*hp*/16, /*recursoMax*/25, /*recurso*/25, /*ataque*/3, /*defensa*/3, /*velocidad*/100.0f },
        posicionInicial);
    miembros.emplace_back("Milo", Role::Control,
        Stats{ /*hpMax*/15, /*hp*/15, /*recursoMax*/18, /*recurso*/18, /*ataque*/5, /*defensa*/3, /*velocidad*/95.0f },
        posicionInicial);

    return game::Party(std::move(miembros));
}

// Arma un party de partida (mismos 4 personajes de siempre, stats base de
// nivel 1) pero conservando el nivel/experiencia que ya tuviera cada uno
// en 'progresoPrevio' (mismo orden que CrearPartyDeEjemplo: Bruna/Kael/
// Sara/Milo, emparejados por posicion) -- el nivel es progreso PERMANENTE
// del jugador (sobrevive tanto a un Game Over como a elegir "Nueva
// partida" a proposito, ver "Sistema de niveles" en docs/design.md), a
// diferencia del equipo, el inventario y el HP en curso, que si se
// resetean del todo en los dos casos. Reaplica el crecimiento de nivel
// via game::Character::GanarExperiencia (con el total de experiencia
// acumulada de cada personaje) en vez de copiar stats_ directo, para no
// duplicar la tabla de crecimiento por rol que ya vive en character.cpp.
// Las habilidades de la Academia (ver game::kNivelMejoraHabilidad/
// kNivelHabilidadNueva) son el mismo tipo de progreso permanente que el
// nivel -- se copian aparte porque, a diferencia del nivel, no hay forma de
// "reaplicarlas" solo con la experiencia total (no tocan ningun stat).
game::Party CrearPartyConProgreso(game::Vec2 posicionInicial, const game::Party& progresoPrevio) {
    game::Party nuevo = CrearPartyDeEjemplo(posicionInicial);
    auto& miembrosNuevos = nuevo.Miembros();
    const auto& miembrosPrevios = progresoPrevio.Miembros();
    for (size_t i = 0; i < miembrosNuevos.size() && i < miembrosPrevios.size(); ++i) {
        int xpTotal = miembrosPrevios[i].ExperienciaAcumuladaTotal();
        if (xpTotal > 0) miembrosNuevos[i].GanarExperiencia(xpTotal);
        miembrosNuevos[i].CargarHabilidadesGuardado(
            miembrosPrevios[i].MejoraHabilidadAprendida(), miembrosPrevios[i].HabilidadNuevaAprendida());
    }
    return nuevo;
}

// Uno de los 2 tipos "comunes" (agresivo o especial, rol local 0 o 1 — ver
// game::RolLocalDeEnemigo en enemy.h) del tema pedido, con la misma chance
// cada uno.
game::TipoEnemigo TipoComunAleatorio(Tema tema) {
    int base = static_cast<int>(tema) * game::kEnemigosPorTema;
    return static_cast<game::TipoEnemigo>(base + (game::Roll(2) - 1));  // +0 o +1
}

// El jefe (rol local 2) del tema pedido.
game::TipoEnemigo JefeDe(Tema tema) {
    int base = static_cast<int>(tema) * game::kEnemigosPorTema;
    return static_cast<game::TipoEnemigo>(base + 2);
}

// Arma un enemigo del tipo pedido, con sus stats de siempre, ubicado en
// 'posicion' y etiquetado con la sala a la que pertenece (para poder
// agrupar a todos los de una sala en un solo encuentro al engancharlos).
// 'ocurrencia' es el numero de orden de este enemigo entre los de su mismo
// tipo dentro de la sala (1, 2, 3...) — si hay mas de uno del mismo tipo en
// la misma sala, se le agrega un sufijo al nombre (" II", " III") para que
// se puedan distinguir en el log y en las fichas de combate. 'dificultad'
// escala hp/ataque (ver EscalaDe) — los numeros comentados en cada caso de
// abajo son los de referencia en Media (multiplicador 1.0, igual que antes
// de que existiera el mapa de dificultades). Los 9 tipos (2 comunes + 1
// jefe por cada uno de los 3 temas, ver game::TipoEnemigo en enemy.h) estan
// pensados con una tabla de stats pareja entre temas: el "comun agresivo"
// de cada uno es el mas duro de los dos comunes (mismo criterio que antes
// tenia el unico agresivo, el Bandido Aturdidor), y los 3 jefes quedan
// todos en un rango similar de hp/ataque (52-56 de referencia en Media).
game::Enemy CrearEnemigoDeTipo(game::TipoEnemigo tipo, game::Vec2 posicion, int salaIndice, int ocurrencia,
                                Dificultad dificultad) {
    const char* sufijo = (ocurrencia == 2) ? " II" : (ocurrencia == 3) ? " III" : "";
    EscalaDificultad escala = EscalaDe(dificultad);
    auto Escalado = [&](int hpBase, int ataqueBase, int defensa, float velocidad) {
        int hp = static_cast<int>(hpBase * escala.hp + 0.5f);
        int ataque = static_cast<int>(ataqueBase * escala.ataque + 0.5f);
        return game::Stats{ /*hpMax*/hp, /*hp*/hp, /*recursoMax*/0, /*recurso*/0, ataque, defensa, velocidad };
    };

    switch (tipo) {
        // --- Bosque ---
        case game::TipoEnemigo::LoboSalvaje:
            return game::Enemy(std::string("Lobo Salvaje") + sufijo, tipo,
                Escalado(/*hp*/18, /*ataque*/8, /*defensa*/2, /*velocidad*/130.0f),
                posicion, salaIndice);
        case game::TipoEnemigo::AranaGigante:
            return game::Enemy(std::string("Araña Gigante") + sufijo, tipo,
                Escalado(/*hp*/20, /*ataque*/6, /*defensa*/2, /*velocidad*/90.0f),
                posicion, salaIndice);
        case game::TipoEnemigo::AlfaDelBosque:
            return game::Enemy("Alfa del Bosque", tipo,
                Escalado(/*hp*/54, /*ataque*/8, /*defensa*/5, /*velocidad*/100.0f),
                posicion, salaIndice);
        // --- Carcel ---
        case game::TipoEnemigo::PresoAmotinado:
            return game::Enemy(std::string("Preso Amotinado") + sufijo, tipo,
                Escalado(/*hp*/22, /*ataque*/7, /*defensa*/3, /*velocidad*/95.0f),
                posicion, salaIndice);
        case game::TipoEnemigo::GuardiaCorrupto:
            return game::Enemy(std::string("Guardia Corrupto") + sufijo, tipo,
                Escalado(/*hp*/24, /*ataque*/6, /*defensa*/4, /*velocidad*/80.0f),
                posicion, salaIndice);
        case game::TipoEnemigo::Alcaide:
            return game::Enemy("Alcaide", tipo,
                Escalado(/*hp*/54, /*ataque*/8, /*defensa*/5, /*velocidad*/85.0f),
                posicion, salaIndice);
        // --- Castillo ---
        case game::TipoEnemigo::GuardiaReal:
            return game::Enemy(std::string("Guardia Real") + sufijo, tipo,
                Escalado(/*hp*/26, /*ataque*/7, /*defensa*/5, /*velocidad*/88.0f),
                posicion, salaIndice);
        case game::TipoEnemigo::MagoDeLaCorte:
            return game::Enemy(std::string("Mago de la Corte") + sufijo, tipo,
                Escalado(/*hp*/16, /*ataque*/6, /*defensa*/2, /*velocidad*/95.0f),
                posicion, salaIndice);
        default:  // CapitanDeLaGuardia
            return game::Enemy("Capitan de la Guardia", tipo,
                Escalado(/*hp*/56, /*ataque*/9, /*defensa*/6, /*velocidad*/90.0f),
                posicion, salaIndice);
    }
}

// Arma el grupo de enemigos de una sala con contenido: antes eran 1 a 3
// (sentia "monotono", segun feedback del usuario); ahora son 3 a 5, de
// tipos elegidos al azar (pueden repetirse), repartidos en una grilla de
// hasta 3 columnas por fila (2 filas si hay mas de 3) para que no queden
// superpuestos ni se acerquen demasiado a la pared, incluso en la sala mas
// chica (la alargada, 6 tiles = 288px de ancho). 'dificultad' tambien decide
// cuantos enemigos entran: Facil 2-3 (mazmorra "de entrada"), Media 3-5
// (igual que siempre), Dificil 5-7 (la grilla de hasta 3 columnas soporta
// hasta 3 filas sin problema).
std::vector<game::Enemy> CrearGrupoDeSala(game::Vec2 centro, int salaIndice, Tema tema, Dificultad dificultad) {
    int cantidad;
    switch (dificultad) {
        case Dificultad::Facil:   cantidad = 1 + game::Roll(2); break;  // 2 o 3
        case Dificultad::Dificil: cantidad = 4 + game::Roll(3); break;  // 5, 6 o 7
        default:                  cantidad = 2 + game::Roll(3); break; // 3, 4 o 5 (Media)
    }
    std::vector<game::Enemy> grupo;
    int vistos[9] = {0};  // contador por TipoEnemigo (indice del enum), para el sufijo

    constexpr int kMaxPorFila = 3;
    constexpr float kSeparacionX = 65.0f;
    constexpr float kSeparacionY = 55.0f;

    for (int j = 0; j < cantidad; ++j) {
        game::TipoEnemigo tipo = TipoComunAleatorio(tema);
        int& ocurrencias = vistos[static_cast<int>(tipo)];
        ocurrencias += 1;

        int fila = j / kMaxPorFila;
        int columnasEnEstaFila = std::min(kMaxPorFila, cantidad - fila * kMaxPorFila);
        int columna = j % kMaxPorFila;
        float dx = (columna - (columnasEnEstaFila - 1) / 2.0f) * kSeparacionX;
        float dy = (cantidad > kMaxPorFila) ? (fila - 0.5f) * kSeparacionY : 0.0f;
        game::Vec2 posicion = centro + game::Vec2{dx, dy};

        grupo.push_back(CrearEnemigoDeTipo(tipo, posicion, salaIndice, ocurrencias, dificultad));
    }
    return grupo;
}

// Cuantas salas tiene la cadena de una mazmorra segun su Dificultad (ver
// game::Dungeon::Dungeon en dungeon.h/cpp) -- 1 inicial + N de combate + la
// del jefe (siempre la ultima). Antes era un numero fijo (5) para las 3
// dificultades; ahora escala igual que ya escala la cantidad de enemigos por
// sala en CrearGrupoDeSala: Facil se mantiene tan corta como la mazmorra
// original ("de entrada"), Media y Dificil se estiran cada vez mas. La
// escalera quedo pareja a proposito -- 2 salas de combate mas por escalon
// (3 -> 5 -> 7) -- despues de feedback de que Facil y Media casi no se
// notaban distintas en duracion con el primer ajuste (6/7/9, un escalon de
// 1 sala y otro de 2). Ver "Mazmorras mas largas: cantidad de salas por
// Dificultad" en docs/design.md.
int SalasPorDificultad(Dificultad dificultad) {
    switch (dificultad) {
        case Dificultad::Facil:   return 5;  // 1 inicial + 3 de combate + jefe
        case Dificultad::Dificil: return 9;  // 1 inicial + 7 de combate + jefe
        default:                  return 7;  // Media: 1 inicial + 5 de combate + jefe
    }
}

// Chance (de 10) de que una sala con contenido tenga ADEMAS un cofre aparte
// de su grupo de enemigos, ubicado en una esquina de la sala (con margen de
// la pared) para no superponerse con ellos.
constexpr int kChanceCofrePorSalaDe10 = 4;

// Arma el cofre de una sala, ubicado en una esquina (con margen de la
// pared) para no pisar a los enemigos, que suelen estar cerca del centro.
game::Cofre CrearCofreEnEsquina(const game::Habitacion& sala, game::Item contenido) {
    game::Vec2 posicion{
        (sala.x + 1.5f) * game::kTileSize,
        (sala.y + 1.5f) * game::kTileSize
    };
    return game::Cofre{posicion, std::move(contenido), false};
}

// Envoltorios de game::ItemAleatorioDeCofre()/TirarLootDeEnemigo() que le dan
// mas empuje al loot en Dificil, sin tocar la API portable de game/item.cpp
// (mismo criterio que el resto de este archivo: contenido de juego que no
// necesita vivir en game/ va aca). Viven en main.cpp, no en item.cpp, porque
// dependen de Dificultad.
game::Item ItemDeCofrePorDificultad(Dificultad dificultad) {
    // Dificil: 50% de forzar una Mejora (equipo permanente) en vez de dejar
    // el sorteo normal (40% pocion / 20% elixir / 20% consumible de combate
    // / 20% mejora, ver item.cpp) — se vuelve a tirar del todo en vez de
    // "convertir" el item ya sorteado, mas simple y el resultado es el mismo.
    if (dificultad == Dificultad::Dificil && game::Roll(2) == 1) {
        return game::MejoraAleatoria();
    }
    return game::ItemAleatorioDeCofre();
}

game::ResultadoLoot LootDeEnemigoPorDificultad(game::TipoEnemigo tipo, Dificultad dificultad) {
    game::ResultadoLoot loot = game::TirarLootDeEnemigo(tipo);
    // Dificil: un enemigo que no soltaba nada todavia tiene 25% de dejar al
    // menos una Pocion de Curacion Menor como consuelo — la mazmorra mas
    // dura es tambien la que mas recursos consume, asi que conviene que
    // reponga un poco mas seguido.
    if (!loot.hay && dificultad == Dificultad::Dificil && game::Roll(4) == 1) {
        loot.hay = true;
        loot.item = game::PocionCuracionMenor();
    }
    return loot;
}

// Oro que suelta un enemigo derrotado (ver "Fuente del oro" y "Sistema de
// oro" en docs/design.md) -- escala con la Dificultad igual que hp/ataque de
// los enemigos (ver EscalaDe), asi que una mazmorra Dificil no solo pega mas
// fuerte, tambien paga mas. El jefe deja bastante mas que un comun, para que
// se sienta como el premio grande de la mazmorra ademas de la Mejora
// garantizada que ya suelta (ver TirarLootDeEnemigo en item.cpp).
int OroDeEnemigoPorDificultad(game::TipoEnemigo tipo, Dificultad dificultad) {
    int base = game::EsJefe(tipo) ? 25 : 8;
    return static_cast<int>(base * EscalaDe(dificultad).ataque + 0.5f);
}

// Oro de un cofre, independiente del item que tambien suelta -- mismo
// criterio de escalado que arriba.
int OroDeCofrePorDificultad(Dificultad dificultad) {
    return static_cast<int>(10 * EscalaDe(dificultad).ataque + 0.5f);
}

// Todo lo que hace falta para generar una mazmorra jugable de la dificultad
// pedida: la mazmorra procedural en si, el punto donde aparece el party al
// entrar, y los enemigos/cofres repartidos por sus salas. A diferencia de la
// vieja PartidaNueva/GenerarPartidaNueva (antes de Round G), esto YA NO
// incluye al party — ahora el party es independiente del mapa (persiste con
// su HP/inventario/equipo actual al pasar de una mazmorra a otra, "sigue con
// el desgaste" — ver EstadoJuego::MapaTema/MapaDificultad) y se crea/
// resetea aparte, solo en "Nueva partida" o tras un Game Over (reinicio
// total de la run).
struct MazmorraGenerada {
    game::Dungeon mazmorra;
    game::Vec2 posicionInicial;
    std::vector<game::Enemy> enemigos;
    std::vector<game::Cofre> cofres;
};

MazmorraGenerada GenerarMazmorra(Tema tema, Dificultad dificultad) {
    // Mazmorra procedural: una cadena de salas conectadas por pasillos (ver
    // game/dungeon.cpp). La sala 0 es donde arranca el party, sin enemigos;
    // las salas intermedias tienen un grupo de enemigos comunes del tema
    // elegido (cuantos, segun 'dificultad' — ver CrearGrupoDeSala), que se
    // enganchan todos juntos en un mismo combate; la ultima sala, en
    // cambio, tiene un unico jefe (el de 'tema' — ver JefeDe), que al caer
    // la marca como "Superada" esa combinacion tema+dificultad en el mapa
    // (ver "Balance" en docs/design.md y CombatEncounter::Actualizar para
    // su IA especial). La CANTIDAD de salas de la cadena tambien depende de
    // 'dificultad' — ver SalasPorDificultad.
    game::Dungeon mazmorra(SalasPorDificultad(dificultad));
    game::Vec2 posicionInicial = mazmorra.CentroDeSala(0);

    std::vector<game::Enemy> enemigos;
    const auto& salas = mazmorra.Habitaciones();
    size_t indiceSalaJefe = salas.size() - 1;
    for (size_t i = 1; i < salas.size(); ++i) {
        if (i == indiceSalaJefe) {
            enemigos.push_back(CrearEnemigoDeTipo(JefeDe(tema),
                mazmorra.CentroDeSala(i), static_cast<int>(i), 1, dificultad));
            continue;
        }
        std::vector<game::Enemy> grupo = CrearGrupoDeSala(mazmorra.CentroDeSala(i), static_cast<int>(i), tema, dificultad);
        for (auto& e : grupo) enemigos.push_back(std::move(e));
    }

    // Cofres: uno garantizado en la sala inicial (para que el sistema se vea
    // sin depender del azar) y, ademas, una chance por cada sala con
    // contenido de tener uno extra aparte de su grupo de enemigos.
    std::vector<game::Cofre> cofres;
    cofres.push_back(CrearCofreEnEsquina(salas[0], ItemDeCofrePorDificultad(dificultad)));
    for (size_t i = 1; i < salas.size(); ++i) {
        if (game::Roll(10) <= kChanceCofrePorSalaDe10) {
            cofres.push_back(CrearCofreEnEsquina(salas[i], ItemDeCofrePorDificultad(dificultad)));
        }
    }

    return MazmorraGenerada{ std::move(mazmorra), posicionInicial, std::move(enemigos), std::move(cofres) };
}

// Paleta de tiles que usa la Ciudad para dibujarse: la suya propia (ver
// render::SpriteSet::TilePisoCiudad/TileParedCiudad), NO una de las 3
// paletas de mazmorra. La primera version de esta vuelta reusaba la de
// Castillo (mismo indice, "kTemaCiudad = 2") para no sumar una cuarta
// paleta solo para 3 edificios estaticos -- descartado tras feedback
// directo del usuario ("la ciudad parece una mazmorra mas... quiero que
// tenga estetica de ciudad, que sea un bioma mas"). render::kTemaCiudad
// (sprites.h) es un valor fuera del rango 0..2 de game::Tema a proposito,
// para que el renderer distinga los dos casos sin ambiguedad — ver el
// comentario de esa constante.

// Todo lo que hace falta para dibujar y explorar la Ciudad: la mazmorra en
// si (varias zonas fijas conectadas por una calle, armada a mano en vez de
// procedural — ver mas abajo), sus 3 edificios interactuables, la fuente
// decorativa de la plaza, y el perro/pajaros/aldeanos que la recorren (ver
// game::Deambulante). A diferencia de MazmorraGenerada, esto NO varia entre
// visitas: la Ciudad es siempre el mismo layout, asi que ConstruirCiudad()
// no usa game::Roll para elegirlo (si lo usa, indirectamente, para el
// primer objetivo de cada deambulante — ver CrearDeambulante).
struct CiudadGenerada {
    game::Dungeon mazmorra;
    game::Vec2 posicionInicial;
    std::vector<game::Edificio> edificios;
    std::vector<game::Vec2> fuentes;
    std::vector<game::Deambulante> deambulantes;
};

// Arma la Ciudad a mano con el constructor de datos-ya-resueltos de
// game::Dungeon (el mismo que usa game/save.h para reconstruir una mazmorra
// guardada, ver el comentario de ese constructor en dungeon.h): dos salas —
// la Plaza Central (donde esta la Entrada a las mazmorras y arranca el
// party) y la Calle de Comercios (Herreria + Tienda) — unidas por un tramo
// de calle mas angosto, en vez de la unica sala chica de la primera version
// (descartada por sentirse "como una mazmorra mas", ver el comentario de
// arriba). Paredes de borde calculadas con el mismo criterio de siempre
// (cualquier tile del bounding box combinado, con 1 de margen, que no sea
// parte de NINGUNA de las 3 salas es pared) — generalizacion directa del
// caso de una sola sala que ya usaba Dungeon::Dungeon(int). Da colision y
// camara gratis sin escribir logica de movimiento nueva para la Ciudad.
CiudadGenerada ConstruirCiudad() {
    // Coordenadas en tiles. La Plaza (0,0)-(14,10) y la Calle de Comercios
    // (17,2)-(27,8) quedan centradas en la misma fila (y=5 tiles en las
    // dos), asi el tramo de calle que las conecta (14,4)-(17,8) es un
    // pasillo recto sin quiebres. 4 tiles de alto (no 3): con 3, el borde
    // de la calle quedaba justo en el mismo pixel que 'posicionInicial' de
    // mas abajo (spawn 2 tiles abajo del centro de la plaza) y el lider
    // quedaba trabado contra esa pared apenas cruzaba a la Calle de
    // Comercios -- encontrado y corregido tras probar el cruce bajo Xvfb.
    const game::Habitacion plaza{ 0, 0, 14, 10 };
    const game::Habitacion calle{ 14, 4, 3, 4 };
    const game::Habitacion comercio{ 17, 2, 10, 6 };
    std::vector<game::Habitacion> habitaciones{ plaza, calle, comercio };

    auto esPiso = [&](int x, int y) {
        for (const auto& h : habitaciones) {
            if (x >= h.x && x < h.x + h.ancho && y >= h.y && y < h.y + h.alto) return true;
        }
        return false;
    };
    int minX = plaza.x, minY = plaza.y, maxX = plaza.x + plaza.ancho, maxY = plaza.y + plaza.alto;
    for (const auto& h : habitaciones) {
        minX = std::min(minX, h.x);
        minY = std::min(minY, h.y);
        maxX = std::max(maxX, h.x + h.ancho);
        maxY = std::max(maxY, h.y + h.alto);
    }

    std::vector<game::Rect> paredes;
    for (int y = minY - 1; y <= maxY; ++y) {
        for (int x = minX - 1; x <= maxX; ++x) {
            if (esPiso(x, y)) continue;
            paredes.push_back(game::Rect{
                x * game::kTileSize, y * game::kTileSize, game::kTileSize, game::kTileSize
            });
        }
    }

    game::Dungeon mazmorra(std::move(habitaciones), std::move(paredes), /*trampas*/{});
    game::Vec2 centroPlaza = mazmorra.CentroDeSala(0);
    game::Vec2 centroComercio = mazmorra.CentroDeSala(2);

    // El party arranca abajo de la plaza, mirando "hacia arriba" a la
    // fuente y a la Entrada a las mazmorras — mismo criterio de composicion
    // que un hub tipico (todo a la vista apenas se entra, nada escondido
    // detras del punto de spawn). 2 tiles (no 1, como en la primera
    // version) para dejar despejado el circulo de la fuente, que esta
    // justo en el centro de la plaza -- la camara sigue al lider (ver
    // Renderer::DibujarEscenarioSinUI), asi que la distancia que importa
    // para que no se corte el cartel de la Entrada es la del edificio AL
    // PUNTO DE SPAWN, no al centro de la sala (verificado bajo Xvfb que 2+3
    // tiles de distancia total siguen dejando el cartel completo en
    // pantalla).
    game::Vec2 posicionInicial{ centroPlaza.x, centroPlaza.y + 2.0f * game::kTileSize };

    std::vector<game::Edificio> edificios{
        game::Edificio{ game::TipoEdificio::EntradaMazmorras, game::Vec2{ centroPlaza.x, centroPlaza.y - 3.0f * game::kTileSize } },
        game::Edificio{ game::TipoEdificio::Herreria, game::Vec2{ centroComercio.x - 3.0f * game::kTileSize, centroComercio.y - 1.0f * game::kTileSize } },
        // Academia (nueva, ver TipoEdificio::Academia): centrada entre
        // Herreria y Tienda, misma fila -- pedido directo del usuario tras
        // terminar la Ciudad ("aprender habilidades", parte del pedido
        // original que habia quedado pendiente, ver "La Ciudad" en
        // docs/design.md).
        game::Edificio{ game::TipoEdificio::Academia, game::Vec2{ centroComercio.x, centroComercio.y - 1.0f * game::kTileSize } },
        game::Edificio{ game::TipoEdificio::Tienda, game::Vec2{ centroComercio.x + 3.0f * game::kTileSize, centroComercio.y - 1.0f * game::kTileSize } },
    };

    // Fuente al centro de la plaza -- puramente decorativa (ver
    // render::DibujarFuente), sin colision, mismo criterio que los
    // edificios.
    std::vector<game::Vec2> fuentes{ centroPlaza };

    // Perro, pajaros y aldeanos deambulando (ver game::Deambulante) --
    // pedido directo del usuario ("quiero que tenga vida, que se mueva un
    // perro, algunos pajaros"). Anclajes elegidos a mano dentro de cada
    // sala, lejos de las paredes y de la linea recta spawn-Entrada para que
    // no se sientan "en el medio del paso". Puramente decorativos, no se
    // guardan (ver el comentario de game::Deambulante).
    std::vector<game::Deambulante> deambulantes{
        game::CrearDeambulante(game::TipoDeambulante::Perro,
            game::Vec2{ centroPlaza.x - 2.0f * game::kTileSize, centroPlaza.y + 2.0f * game::kTileSize }, 90.0f, 55.0f),
        game::CrearDeambulante(game::TipoDeambulante::Pajaro,
            game::Vec2{ centroPlaza.x - 4.5f * game::kTileSize, centroPlaza.y - 1.0f * game::kTileSize }, 70.0f, 45.0f),
        game::CrearDeambulante(game::TipoDeambulante::Pajaro,
            game::Vec2{ centroPlaza.x + 4.5f * game::kTileSize, centroPlaza.y + 2.5f * game::kTileSize }, 70.0f, 45.0f),
        game::CrearDeambulante(game::TipoDeambulante::AldeanoA,
            game::Vec2{ centroPlaza.x + 3.0f * game::kTileSize, centroPlaza.y + 1.5f * game::kTileSize }, 70.0f, 22.0f),
        game::CrearDeambulante(game::TipoDeambulante::AldeanoB,
            game::Vec2{ centroComercio.x - 1.5f * game::kTileSize, centroComercio.y + 1.5f * game::kTileSize }, 60.0f, 20.0f),
        game::CrearDeambulante(game::TipoDeambulante::AldeanoC,
            game::Vec2{ centroComercio.x + 1.5f * game::kTileSize, centroComercio.y + 1.5f * game::kTileSize }, 60.0f, 20.0f),
        game::CrearDeambulante(game::TipoDeambulante::Pajaro,
            game::Vec2{ centroComercio.x, centroComercio.y - 2.0f * game::kTileSize }, 60.0f, 45.0f),
    };

    return CiudadGenerada{ std::move(mazmorra), posicionInicial, std::move(edificios), std::move(fuentes), std::move(deambulantes) };
}

// Catalogo de precios de la Herreria (Mejoras, ver game/item.h) y la Tienda
// (Consumibles) -- vive aca, no en game/item.cpp, por el mismo motivo que
// ItemDeCofrePorDificultad/LootDeEnemigoPorDificultad mas arriba: es
// contenido de juego especifico de este prototipo, no algo que game/ deba
// conocer. 'fabricar' apunta directo a la funcion de fabrica del catalogo
// (ver game/item.h) — comprar llama a esta funcion para obtener el Item real
// (nombre/descripcion incluidos, para no duplicar ese texto a mano en la UI,
// ver render::ui::OfertaComercio) y lo agrega al inventario igual que
// cualquier otro item encontrado.
struct OfertaComercio {
    int precio;
    game::Item (*fabricar)();
};

constexpr OfertaComercio kOfertasHerreria[] = {
    { 40, game::PiedraDeFuerza },
    { 40, game::AmuletoDeProteccion },
    { 45, game::DagaVeloz },
    { 45, game::TalismanDeVitalidad },
};
constexpr int kNumOfertasHerreria = 4;

constexpr OfertaComercio kOfertasTienda[] = {
    { 15, game::PocionCuracionMenor },
    { 15, game::ElixirDeEnergia },
    { 20, game::BombaDeVeneno },
    { 20, game::FrascoDeEscudo },
    { 12, game::Antidoto },
};
constexpr int kNumOfertasTienda = 5;

// Frases sueltas de los 3 aldeanos de la Ciudad (ver game::TipoDeambulante,
// game::EsAldeano) al hablarles con [E] desde EstadoJuego::Ciudad -- puro
// flavor text, sin arbol de dialogo ni eleccion del jugador: una linea al
// azar (game::Roll) de un pool chico por aldeano cada vez, mostrada con el
// mismo mensajeFlotante/timerMensaje que ya usa el cartel de un cofre. Vive
// aca, no en game/deambulante.h, mismo criterio de siempre: contenido
// concreto de ESTE juego (como el catalogo de comercio de arriba), no
// mecanica generica de game/.
const char* FraseDeAldeano(game::TipoDeambulante tipo) {
    static const char* kFrasesA[] = {
        "Lindo dia para salir a una mazmorra, ¿no?",
        "Cuidado con los lobos si van al Bosque.",
        "Ojala encuentren buen botin ahi afuera.",
    };
    static const char* kFrasesB[] = {
        "Si les sobra oro, denle una vuelta a la Tienda.",
        "La Herreria tiene mejoras que valen la pena.",
        "Los precios subieron un poco... pero vale la pena.",
    };
    static const char* kFrasesC[] = {
        "Vengo de lejos. La Carcel no es un lugar lindo.",
        "El Castillo tiene un jefe durisimo, avisados estan.",
        "Cada mazmorra que superan, esta ciudad se ve mas tranquila.",
    };
    const char** frases = kFrasesA;
    int cantidad = 3;
    if (tipo == game::TipoDeambulante::AldeanoB) { frases = kFrasesB; cantidad = 3; }
    else if (tipo == game::TipoDeambulante::AldeanoC) { frases = kFrasesC; cantidad = 3; }
    return frases[game::Roll(cantidad) - 1];
}

// MenuInicio es el estado inicial: pantalla de titulo con las 4 opciones de
// ui::OpcionMenuInicio (ver render/menu_ui.h) antes de largar a explorar.
// SobreMi es la pantalla placeholder de esa opcion (ver ui::DibujarSobreMi) —
// un estado propio, no un sub-estado de MenuInicio, para que se dibuje y se
// lea el input igual que cualquier otra pantalla de la maquina de estados.
// Ciudad es el hub central donde se prepara la run (ver ConstruirCiudad):
// una plaza explorable igual que una mazmorra, pero estatica y sin
// enemigos/cofres, con 3 edificios interactuables (ver game::Edificio) —
// Herreria y Tienda (ver mas abajo) y la Entrada a las mazmorras, que lleva
// a MapaTema. Se llega aca al elegir "Nueva partida", al elegir "Volver a
// la ciudad" en la pausa (ver ui::OpcionPausa::VolverAlMapa — el nombre del
// enum quedo igual, solo cambio el texto y el destino), al ganarle al jefe
// de una mazmorra, tras un Game Over, o al cargar una partida guardada
// parada en la Ciudad.
// Herreria y Tienda son las pantallas de comercio de esos dos edificios
// (ver ui::DibujarComercio) — Herreria vende Mejoras (equipo permanente),
// Tienda vende Consumibles (ver kOfertasHerreria/kOfertasTienda mas arriba).
// Las dos comparten el mismo bloque de logica en el loop principal (misma
// forma de comprar, solo cambia el catalogo) y ESC en cualquiera de las dos
// vuelve a Ciudad. Aprender habilidades (pedido tambien por el usuario)
// queda para una vuelta futura — ver Roadmap en docs/design.md.
// MapaTema y MapaDificultad son los dos pasos de la pantalla de seleccion de
// mazmorra (ver ui::DibujarMapaTema/DibujarMapaDificultad): primero se
// elige un tema (Bosque/Carcel/Castillo) y despues, dentro de ese tema, una
// dificultad — 9 combinaciones en total. A MapaTema ya NO se llega directo
// desde ningun menu — solo caminando hasta la Entrada a las mazmorras
// dentro de la Ciudad, o cargando una partida guardada parada ahi. Desde
// MapaTema, ENTER pasa a MapaDificultad (con el tema ya fijado en
// 'temaMapaElegido'); desde ahi, ENTER genera la mazmorra de esa combinacion
// y entra a Exploracion. ESC en MapaDificultad vuelve a MapaTema (no a la
// pausa); ESC en MapaTema abre la pausa (igual que antes solo tenia el paso
// unico).
// Pausa es la pantalla que abre ESC durante la exploracion o la Ciudad (ver
// ui::DibujarPausa) — desde ahi se puede volver a jugar, guardar, volver a
// la ciudad, volver a MenuInicio sin cerrar el juego, o salir. No existe
// durante Combate (ESC no hace nada ahi, igual que F5 tampoco guarda en
// combate) ni durante Herreria/Tienda (ESC ahi vuelve directo a Ciudad, sin
// pasar por la pausa).
// SeleccionSlot es la pantalla de elegir en que slot guardar o de cual
// cargar (ver ui::DibujarSeleccionSlot) — se llega desde F5, desde
// "Guardar" en la pausa, o desde "Cargar" en el menu de inicio;
// 'estadoAlCancelarSlot' (mas abajo) guarda a cual de esos tres volver con
// ESC/"Volver".
enum class EstadoJuego {
    MenuInicio, SobreMi, Ciudad, Herreria, Tienda, Academia, MapaTema, MapaDificultad, Exploracion, Pausa, Combate, SeleccionSlot
};

// Distancia (en pixeles) a la que hay que estar del interactuable mas
// cercano (enemigo o cofre) para poder engancharlo/abrirlo con [E].
constexpr float kDistanciaInteraccion = 90.0f;

// Enemigos agresivos (ver game::EsAgresivo): radio en el que "notan" al
// lider y arrancan a perseguirlo, y radio de contacto (mas chico, tiene que
// alcanzarlo de verdad) en el que fuerzan el combate sin esperar [E]. Un
// poco mas lentos que el jugador (kFactorVelocidadAgresivo < 1) para que
// siempre haya chance de escapar corriendo en vez de que sea inevitable.
constexpr float kRadioDeteccionAgresivo = 220.0f;
constexpr float kRadioContactoAgresivo = 40.0f;
constexpr float kFactorVelocidadAgresivo = 0.85f;

// Multiplicador de velocidad de movimiento durante la exploracion (NO toca
// el stat 'velocidad' en si, que tambien se usa para el orden de turno en
// combate -- ver el comentario en game/character.h -- asi que subirlo aca
// en vez de subir las stats de base hace que la exploracion se sienta mas
// agil sin desbalancear quien pega primero en combate). Se aplica por igual
// al lider Y a la persecucion de los enemigos agresivos (mas abajo) para
// que la relacion de velocidades entre los dos -- y con ella la sensacion
// de "se puede escapar corriendo" de kFactorVelocidadAgresivo -- se mantenga
// igual que antes, solo que todo mas rapido. Valor elegido por feedback del
// usuario ("se mueve un poco lento, se vuelve aburrido"); 1.35 es un salto
// que se nota caminando sin sentirse "resbaloso" ni descontrolado.
constexpr float kFactorVelocidadExploracion = 1.35f;

// --- Trampas de piso (ver game::Trampa en game/dungeon.h) ---
// Cooldown (segundos) que le queda a una entidad despues de recibir un tick
// de dano de cada tipo, antes de poder recibir otro (evita un tick por
// frame mientras se queda parada/persiguiendo arriba). Fuego: cooldown
// largo pero dado grande (1d8) -- "quedate quieto ahi y te va a doler
// bastante". Acido: cooldown corto pero dado chico (1d4) -- "sali ya, cada
// instante mas ahi suma". El dano por segundo promedio de las dos queda
// parecido (fuego ~5.6/seg, acido ~5/seg); lo que cambia es el ritmo.
constexpr float kCooldownTrampaFuego = 0.8f;
constexpr float kCooldownTrampaAcido = 0.45f;

// Aplica el dano de trampa correspondiente a 'tipo' y reinicia el cooldown
// de esa entidad -- 'aplicarDano' es RecibirDano de Character o de Enemy
// (misma firma int(int) en las dos clases), y 'reiniciarCooldown' su
// ReiniciarCooldownTrampa. Una sola funcion para no repetir el switch
// fuego/acido en los dos loops (lider y enemigos) de mas abajo.
template <typename AplicarDano, typename ReiniciarCooldown>
void AplicarDanoDeTrampa(game::TipoTrampa tipo, AplicarDano aplicarDano, ReiniciarCooldown reiniciarCooldown) {
    if (tipo == game::TipoTrampa::Fuego) {
        aplicarDano(game::RollDados(1, 8));
        reiniciarCooldown(kCooldownTrampaFuego);
    } else {
        aplicarDano(game::RollDados(1, 4));
        reiniciarCooldown(kCooldownTrampaAcido);
    }
}

// Cuanto tiempo (segundos) queda en pantalla un mensaje flotante (botin de
// un cofre o de un combate ganado) antes de desaparecer solo.
constexpr float kDuracionMensaje = 3.0f;

// True si ALGUNO de los kNumSlots slots tiene partida guardada — el menu de
// inicio lo usa para decidir si "Cargar" lleva a la pantalla de seleccion
// de slot o se queda deshabilitada (ver ui::DibujarMenuInicio).
bool AlgunaPartidaGuardada(const bool hayGuardado[game::kNumSlots]) {
    for (int i = 0; i < game::kNumSlots; ++i) {
        if (hayGuardado[i]) return true;
    }
    return false;
}

// Devuelve el indice (0-8) de la tecla numerica 1-9 apretada este frame, o
// -1 si no se apreto ninguna. Se usa para elegir que item usar del
// inventario (ver DibujarInventario, que muestra "[N]" al lado de cada uno).
int NumeroPresionado() {
    static const int teclas[9] = {
        KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE
    };
    for (int i = 0; i < 9; ++i) {
        if (IsKeyPressed(teclas[i])) return i;
    }
    return -1;
}

} // namespace

int main() {
    const int anchoVentana = 1280;
    const int altoVentana = 720;

    // Arranca con un party y una mazmorra generados de cero — la mazmorra
    // sirve solo de fondo "congelado" del menu de inicio/mapa hasta que el
    // jugador elija que hacer (mismo truco visual de siempre); no hay
    // ninguna mazmorra "activa" todavia (mazmorraActivaIndice = -1). Si mas
    // tarde el jugador elige "Nueva partida" (desde el menu la primera vez,
    // o volviendo por la pausa despues) se resetea todo de nuevo — ver el
    // case NuevaPartida mas abajo — y entrar a una mazmorra desde el mapa
    // llama a GenerarMazmorra(tema, dificultad) para reemplazar esto.
    // 'temaMazmorraCargada' es el tema de LO QUE HAY CARGADO ahora mismo en
    // mazmorra/enemigos/cofres (para que el renderer sepa que paleta de
    // piso/pared usar de fondo, ver renderer.DibujarEscenarioSinUI mas
    // abajo) — no necesariamente el tema "en curso" de la run (que vive en
    // mazmorraActivaIndice); se actualiza cada vez que se llama a
    // GenerarMazmorra o se carga una partida.
    game::Party party = CrearPartyDeEjemplo(game::Vec2{0.0f, 0.0f});
    Tema temaMazmorraCargada = Tema::Bosque;
    MazmorraGenerada fondoInicial = GenerarMazmorra(temaMazmorraCargada, Dificultad::Media);
    game::Dungeon mazmorra = std::move(fondoInicial.mazmorra);
    game::Vec2 posicionInicial = fondoInicial.posicionInicial;
    std::vector<game::Enemy> enemigos = std::move(fondoInicial.enemigos);
    std::vector<game::Cofre> cofres = std::move(fondoInicial.cofres);
    party.ReiniciarFormacion(posicionInicial);

    render::Renderer renderer(anchoVentana, altoVentana, "RPG Mazmorras - Prototipo");
    render::Audio audio;

    // Se chequea una vez al arrancar el programa para cada uno de los
    // kNumSlots slots; despues se mantiene al dia a mano (nunca se vuelve a
    // llamar a HayPartidaGuardada) cada vez que se guarda con exito desde
    // la pantalla de seleccion de slot. Controla si "Cargar" se dibuja
    // habilitada en el menu de inicio (con que un slot tenga partida
    // alcanza — ver AlgunaPartidaGuardada) y que slots se ven "ocupados" en
    // ui::DibujarSeleccionSlot.
    bool hayGuardado[game::kNumSlots];
    for (int i = 0; i < game::kNumSlots; ++i) hayGuardado[i] = game::HayPartidaGuardada(i);

    EstadoJuego estado = EstadoJuego::MenuInicio;
    std::unique_ptr<game::CombatEncounter> encuentro;
    // El panel de arriba a la izquierda queda siempre en modo compacto (ver
    // ui::DibujarPanelParty) — TAB ya no lo alterna a una version expandida
    // sobre el mapa; ahora abre/cierra la ficha de personajes a pantalla
    // completa (ver fichaAbierta abajo y "Ficha de personajes" en
    // docs/design.md).
    bool fichaAbierta = false;       // TAB la abre/cierra durante exploracion
    bool inventarioAbierto = false;  // [I] lo abre/cierra durante exploracion
    bool lootRepartido = false;      // evita repartir el botin mas de una vez por combate
    bool derrotaSonada = false;      // evita repetir el sonido de derrota mientras se ve el Game Over
    size_t objetivoInventario = 0;   // a quien se le aplica el proximo item usado
    bool menuItemCombateAbierto = false;   // [3] durante el turno del aliado abre/cierra este sub-menu
    size_t objetivoAliadoItemCombate = 0;  // a que aliado se le aplica el proximo item de combate (TAB lo cicla)
    std::string mensajeFlotante;     // botin de cofre/combate, visible unos segundos
    float timerMensaje = 0.0f;
    int opcionMenuSeleccionada = 0;  // indice sobre ui::OpcionMenuInicio (ver render/menu_ui.h)
    int opcionPausaSeleccionada = 0; // indice sobre ui::OpcionPausa (ver render/menu_ui.h)
    bool salirDelJuego = false;      // "Salir" (del menu de inicio o de la pausa) lo pone en true

    // --- Mapa de mazmorras (ver EstadoJuego::MapaTema/MapaDificultad) ---
    bool mazmorraSuperada[game::kNumCombinacionesMapa] = {};  // que combinaciones tema+dificultad se ganaron esta run
    int mazmorraActivaIndice = -1;   // indice combinado (0..8, ver IndiceCombinado) de la mazmorra en curso, o -1
    int opcionMapaTemaSeleccionada = 0;        // indice sobre ui::kNumTemasMapa (paso 1)
    Tema temaMapaElegido = Tema::Bosque;       // tema fijado al pasar de MapaTema a MapaDificultad (paso 2)
    int opcionMapaDificultadSeleccionada = 0;  // indice sobre ui::kNumMazmorrasMapa (paso 2)
    // Adonde vuelve "Continuar"/ESC en la pausa — se pisa cada vez que se
    // entra a Pausa (desde Exploracion o desde MapaTema), ver mas abajo.
    EstadoJuego estadoPrevioAPausa = EstadoJuego::Exploracion;

    // --- Seleccion de slot (ver EstadoJuego::SeleccionSlot) ---
    int opcionSlotSeleccionada = 0;      // indice sobre ui::kNumOpcionesSlot
    bool modoGuardarSlot = true;         // true = eligiendo donde guardar; false = eligiendo que cargar
    EstadoJuego estadoAlCancelarSlot = EstadoJuego::MenuInicio;  // adonde vuelve ESC/"Volver"
    std::string mensajeSlot;             // resultado de la ultima accion en esta pantalla

    // --- Ciudad (ver EstadoJuego::Ciudad/Herreria/Tienda, ConstruirCiudad) ---
    // True mientras 'mazmorra'/'enemigos'/'cofres' de arriba tienen cargada
    // la Ciudad en vez de una mazmorra real o el fondo generico del mapa —
    // hace falta aparte de mazmorraActivaIndice (que solo distingue "mapa" de
    // "mazmorra en curso") para que el guardado sepa distinguir la Ciudad de
    // MapaTema, que las dos tienen mazmorraActivaIndice == -1 (ver
    // game::kPantallaCiudad/kPantallaMapa en save.h).
    bool enCiudad = false;
    std::vector<game::Edificio> edificiosCiudad;  // vacio salvo mientras enCiudad es true
    // Fuente(s) decorativa(s) y deambulantes (perro/pajaros/aldeanos) de la
    // Ciudad -- mismo criterio que edificiosCiudad arriba (vacios salvo
    // mientras enCiudad es true, se llenan/vacian siempre junto con el).
    std::vector<game::Vec2> fuentesCiudad;
    std::vector<game::Deambulante> deambulantesCiudad;

    // Reemplaza 'mazmorra'/'enemigos'/'cofres'/'posicionInicial'/
    // 'temaMazmorraCargada' por los de la Ciudad y entra a explorarla — la
    // usan los 4 puntos donde el jugador "vuelve a casa" a prepararse:
    // Nueva partida, Volver a la ciudad (pausa), ganarle al jefe de una
    // mazmorra, y Game Over (ver cada uno mas abajo). NO toca 'party' ni
    // 'mazmorraActivaIndice'/'mazmorraSuperada' — cada llamador decide por
    // su cuenta si corresponde resetearlos antes de llamar a esto.
    auto EntrarALaCiudad = [&]() {
        CiudadGenerada generada = ConstruirCiudad();
        mazmorra = std::move(generada.mazmorra);
        posicionInicial = generada.posicionInicial;
        enemigos.clear();
        cofres.clear();
        edificiosCiudad = std::move(generada.edificios);
        fuentesCiudad = std::move(generada.fuentes);
        deambulantesCiudad = std::move(generada.deambulantes);
        temaMazmorraCargada = static_cast<Tema>(render::kTemaCiudad);
        enCiudad = true;
        party.ReiniciarFormacion(posicionInicial);
        estado = EstadoJuego::Ciudad;
    };

    while (!WindowShouldClose() && !salirDelJuego) {
        float dt = GetFrameTime();
        // Clamp defensivo: si el frame tarda mucho (ventana minimizada, breakpoint,
        // etc.), un dt gigante podria mover al personaje lo suficiente como para
        // atravesar una pared fina en un solo salto. Con esto el movimiento maximo
        // por frame queda acotado.
        if (dt > 1.0f / 30.0f) dt = 1.0f / 30.0f;

        // Musica: avanza el streaming siempre (lo necesita raylib todos los
        // frames) y cambia sola de pista si cambio el estado del juego.
        audio.Actualizar(estado == EstadoJuego::Combate);

        if (estado == EstadoJuego::MenuInicio) {
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                opcionMenuSeleccionada = (opcionMenuSeleccionada + 1) % ui::kNumOpcionesMenuInicio;
            } else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                opcionMenuSeleccionada = (opcionMenuSeleccionada + ui::kNumOpcionesMenuInicio - 1) % ui::kNumOpcionesMenuInicio;
            }

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                switch (static_cast<ui::OpcionMenuInicio>(opcionMenuSeleccionada)) {
                    case ui::OpcionMenuInicio::NuevaPartida: {
                        // Reinicio de la RUN: party de cero en equipo/
                        // inventario/HP/oro y el mapa entero sin ninguna
                        // mazmorra superada — asi el codigo no tiene que
                        // distinguir entre el party/mapa "de fondo" del menu
                        // y una vuelta aca despues de haber jugado (via
                        // Pausa -> Menu principal, ver EstadoJuego::Pausa):
                        // "Nueva partida" siempre arranca una run nueva de
                        // cero. Ya no entra directo al mapa de mazmorras —
                        // ahora primero hay que pasar por la Ciudad (ver
                        // EstadoJuego::Ciudad) y caminar hasta la Entrada a
                        // las mazmorras. El NIVEL de cada personaje, en
                        // cambio, NO se resetea aca — es progreso permanente
                        // del jugador (ver CrearPartyConProgreso arriba y
                        // "Sistema de niveles" en docs/design.md).
                        party = CrearPartyConProgreso(game::Vec2{0.0f, 0.0f}, party);
                        for (int i = 0; i < game::kNumCombinacionesMapa; ++i) mazmorraSuperada[i] = false;
                        mazmorraActivaIndice = -1;
                        opcionMapaTemaSeleccionada = 0;
                        EntrarALaCiudad();
                        break;
                    }
                    case ui::OpcionMenuInicio::Cargar: {
                        // Deshabilitada (ver ui::DibujarMenuInicio) mientras
                        // ningun slot tenga guardado — confirmarla en ese
                        // caso no hace nada, el jugador se queda en el menu.
                        // Con al menos un slot ocupado, en vez de cargar
                        // directo (como antes, con un unico slot posible)
                        // se pasa a elegir cual de los tres — ver
                        // EstadoJuego::SeleccionSlot.
                        if (!AlgunaPartidaGuardada(hayGuardado)) break;
                        modoGuardarSlot = false;
                        estadoAlCancelarSlot = EstadoJuego::MenuInicio;
                        opcionSlotSeleccionada = 0;
                        mensajeSlot.clear();
                        estado = EstadoJuego::SeleccionSlot;
                        break;
                    }
                    case ui::OpcionMenuInicio::SobreMi:
                        estado = EstadoJuego::SobreMi;
                        break;
                    case ui::OpcionMenuInicio::Salir:
                        // Corta el bucle principal en la proxima vuelta (ver
                        // la condicion del while) — el resto de main()
                        // (return 0) ya deja que Renderer/Audio se desarmen
                        // solos por RAII al salir de scope, igual que pasa
                        // al cerrar la ventana con la X.
                        salirDelJuego = true;
                        break;
                }
            }

            BeginDrawing();
            // La mazmorra ya esta generada (se arma antes del loop, mas
            // arriba) — se dibuja "congelada" de fondo para que el menu no
            // arranque sobre una pantalla vacia, mismo truco visual que usa
            // la pantalla de combate.
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada));
            ui::DibujarMenuInicio(anchoVentana, altoVentana, opcionMenuSeleccionada, AlgunaPartidaGuardada(hayGuardado));
            EndDrawing();
        } else if (estado == EstadoJuego::SobreMi) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                estado = EstadoJuego::MenuInicio;
            }

            BeginDrawing();
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada));
            ui::DibujarSobreMi(anchoVentana, altoVentana);
            EndDrawing();
        } else if (estado == EstadoJuego::MapaTema) {
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                opcionMapaTemaSeleccionada = (opcionMapaTemaSeleccionada + 1) % ui::kNumTemasMapa;
            } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                opcionMapaTemaSeleccionada = (opcionMapaTemaSeleccionada + ui::kNumTemasMapa - 1) % ui::kNumTemasMapa;
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                // Vuelve directo a la Ciudad -- mismo criterio de "ESC ==
                // volver a la ciudad" que ya usan Herreria/Tienda/Academia,
                // sin pasar por la pausa. Antes abria la pausa y obligaba a
                // navegar hasta "Volver a la ciudad" para lograr lo mismo;
                // pedido directo del usuario ("no tengo forma de volver
                // atras... si me olvide de comprar algo"). Todavia no se
                // genero ninguna mazmorra en este paso, asi que no hace
                // falta tocar mazmorraActivaIndice (sigue en -1). Para
                // Guardar/Menu principal/Salir, la pausa sigue accesible
                // apretando ESC de nuevo una vez de vuelta en la Ciudad.
                EntrarALaCiudad();
            } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                // Fija el tema elegido y pasa al paso 2 (elegir dificultad
                // dentro de ese tema, ver EstadoJuego::MapaDificultad) — no
                // genera ninguna mazmorra todavia.
                temaMapaElegido = static_cast<Tema>(opcionMapaTemaSeleccionada);
                opcionMapaDificultadSeleccionada = 0;
                estado = EstadoJuego::MapaDificultad;
            }

            // Progreso por tema (cuantas de las 3 dificultades ya estan
            // superadas), para el cartel "x/3 superadas" de cada tarjeta —
            // ver ui::DibujarMapaTema.
            int progresoPorTema[game::kNumTemasMapa] = {};
            for (int t = 0; t < game::kNumTemasMapa; ++t) {
                for (int d = 0; d < game::kNumDificultadesMapa; ++d) {
                    if (mazmorraSuperada[IndiceCombinado(static_cast<Tema>(t), static_cast<Dificultad>(d))]) {
                        progresoPorTema[t] += 1;
                    }
                }
            }

            BeginDrawing();
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada));
            ui::DibujarMapaTema(anchoVentana, altoVentana, opcionMapaTemaSeleccionada, progresoPorTema);
            EndDrawing();
        } else if (estado == EstadoJuego::MapaDificultad) {
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                opcionMapaDificultadSeleccionada = (opcionMapaDificultadSeleccionada + 1) % ui::kNumMazmorrasMapa;
            } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                opcionMapaDificultadSeleccionada =
                    (opcionMapaDificultadSeleccionada + ui::kNumMazmorrasMapa - 1) % ui::kNumMazmorrasMapa;
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                // Vuelve al paso 1 (elegir tema), no a la pausa — la pausa
                // solo se abre desde MapaTema o desde la exploracion.
                estado = EstadoJuego::MapaTema;
            } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                // Entrar genera SIEMPRE una mazmorra nueva (layout fresco),
                // incluso si esta combinacion ya se habia superado antes —
                // las 9 combinaciones son rejugables sin limite (pedido
                // explicito: "dificultad creciente, cada una rejugable
                // indefinidamente con un layout regenerado"). El party NO se
                // toca — sigue con el HP/inventario/equipo que traia del
                // mapa ("sigue con el desgaste" entre mazmorras).
                Dificultad dificultad = static_cast<Dificultad>(opcionMapaDificultadSeleccionada);
                MazmorraGenerada generada = GenerarMazmorra(temaMapaElegido, dificultad);
                mazmorra = std::move(generada.mazmorra);
                posicionInicial = generada.posicionInicial;
                enemigos = std::move(generada.enemigos);
                cofres = std::move(generada.cofres);
                temaMazmorraCargada = temaMapaElegido;
                mazmorraActivaIndice = IndiceCombinado(temaMapaElegido, dificultad);
                edificiosCiudad.clear();
                fuentesCiudad.clear();
                deambulantesCiudad.clear();
                enCiudad = false;
                party.ReiniciarFormacion(posicionInicial);
                estado = EstadoJuego::Exploracion;
            }

            bool superadaDeEsteTema[game::kNumDificultadesMapa];
            for (int d = 0; d < game::kNumDificultadesMapa; ++d) {
                superadaDeEsteTema[d] = mazmorraSuperada[IndiceCombinado(temaMapaElegido, static_cast<Dificultad>(d))];
            }

            BeginDrawing();
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada));
            ui::DibujarMapaDificultad(anchoVentana, altoVentana, static_cast<int>(temaMapaElegido),
                                       opcionMapaDificultadSeleccionada, superadaDeEsteTema);
            EndDrawing();
        } else if (estado == EstadoJuego::Exploracion) {
            if (timerMensaje > 0.0f) {
                timerMensaje -= dt;
                if (timerMensaje <= 0.0f) {
                    timerMensaje = 0.0f;
                    mensajeFlotante.clear();
                }
            }

            // ESC cierra primero lo que este encima (mismo criterio "cerrar
            // lo de encima antes" que un juego tipico): inventario, despues
            // la ficha de personajes, y recien con los dos cerrados abre la
            // pausa. Arranca siempre en "Continuar" (indice 0) para que un
            // ESC sin querer, seguido de un ENTER sin querer, no dispare
            // "Guardar" ni "Salir".
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (inventarioAbierto) {
                    inventarioAbierto = false;
                } else if (fichaAbierta) {
                    fichaAbierta = false;
                } else {
                    opcionPausaSeleccionada = 0;
                    estadoPrevioAPausa = EstadoJuego::Exploracion;
                    estado = EstadoJuego::Pausa;
                }
            }

            // [I] y F5 quedan sin efecto mientras la ficha de personajes esta
            // abierta (mismo criterio que ya evita abrir el inventario a
            // medias de otra pantalla) — primero hay que cerrarla con TAB o
            // ESC.
            if (!fichaAbierta && IsKeyPressed(KEY_I)) inventarioAbierto = !inventarioAbierto;

            // F5 abre la seleccion de slot en cualquier momento de la
            // exploracion (con el inventario abierto o no) — nunca durante
            // combate, ni siquiera apretando la tecla por error, porque
            // este bloque es EstadoJuego::Exploracion nomas. Antes guardaba
            // directo a un unico archivo; ahora hay que elegir en cual de
            // los kNumSlots guardar (ver EstadoJuego::SeleccionSlot).
            if (!fichaAbierta && IsKeyPressed(KEY_F5)) {
                modoGuardarSlot = true;
                estadoAlCancelarSlot = EstadoJuego::Exploracion;
                opcionSlotSeleccionada = 0;
                mensajeSlot.clear();
                estado = EstadoJuego::SeleccionSlot;
            }

            std::string prompt;

            if (inventarioAbierto) {
                // Con el inventario abierto se congela la exploracion: TAB
                // cicla a quien se le va a aplicar el proximo item, y 1-9 lo
                // usa sobre ese objetivo (ver ui::DibujarInventario).
                auto& miembros = party.Miembros();
                if (IsKeyPressed(KEY_TAB) && !miembros.empty()) {
                    objetivoInventario = (objetivoInventario + 1) % miembros.size();
                }
                int indice = NumeroPresionado();
                if (indice >= 0 && objetivoInventario < miembros.size()) {
                    const auto& pilas = party.Inventario().Pilas();
                    if (static_cast<size_t>(indice) < pilas.size()) {
                        // Consumibles se usan directo (se gastan al toque);
                        // Mejoras se equipan en su ranura (Arma/Accesorio) en
                        // vez de aplicarse instantaneo — asi no se pueden
                        // acumular sin limite en el mismo personaje.
                        bool esConsumibleDeCombate = pilas[indice].item.efecto == game::EfectoItem::AplicarEstado
                            || pilas[indice].item.efecto == game::EfectoItem::CurarEstados;
                        if (esConsumibleDeCombate) {
                            // Bomba de Veneno/Frasco de Escudo/Antidoto solo
                            // tienen sentido en combate (ver [3] Usar item
                            // durante FaseCombate::TurnoAliado) -- usarlos
                            // desde aca no hace nada (game::UsarItem no
                            // maneja estos efectos), asi que se avisa en vez
                            // de gastar la tecla en silencio.
                            mensajeFlotante = pilas[indice].item.nombre + " solo se puede usar en combate.";
                            timerMensaje = kDuracionMensaje;
                        } else if (pilas[indice].item.tipo == game::TipoItem::Consumible) {
                            game::ResultadoUsoItem resultado = party.Inventario().Usar(
                                static_cast<size_t>(indice), miembros[objetivoInventario]);
                            if (resultado.exitoso) {
                                mensajeFlotante = resultado.texto;
                                timerMensaje = kDuracionMensaje;
                            }
                        } else {
                            game::ResultadoEquipar resultado = party.Inventario().Equipar(
                                static_cast<size_t>(indice), miembros[objetivoInventario]);
                            if (resultado.exitoso) {
                                mensajeFlotante = resultado.texto;
                                timerMensaje = kDuracionMensaje;
                            }
                        }
                    }
                }
            } else if (fichaAbierta) {
                // Con la ficha de personajes abierta se congela la
                // exploracion igual que con el inventario — solo TAB (o ESC,
                // ya manejado arriba) la cierra.
                if (IsKeyPressed(KEY_TAB)) fichaAbierta = false;
            } else {
                // Arma el combate contra TODOS los enemigos vivos de
                // 'salaIndice' de una sola vez (un combate por sala, no por
                // enemigo individual) — la usan tanto el enganche manual
                // ([E], mas abajo) como el enganche automatico de un
                // enemigo agresivo que alcanza al lider.
                auto EngancharCombateConSala = [&](int salaIndice) {
                    std::vector<game::Enemy*> grupo;
                    for (auto& e : enemigos) {
                        if (!e.Vencido() && e.Sala() == salaIndice) grupo.push_back(&e);
                    }
                    encuentro = std::make_unique<game::CombatEncounter>(party, std::move(grupo));
                    ui::ReiniciarFeedbackVisual();
                    audio.ReiniciarCombate();
                    estado = EstadoJuego::Combate;
                    lootRepartido = false;
                    derrotaSonada = false;
                    menuItemCombateAbierto = false;
                    objetivoAliadoItemCombate = 0;
                };

                game::Vec2 direccion = input::LeerDireccionMovimiento();
                game::Character& lider = party.Lider();

                float velocidadPxPorSeg = lider.GetStats().velocidad * kFactorVelocidadExploracion;
                game::Vec2 posicionActual = lider.Posicion();
                game::Vec2 posicionDeseada = posicionActual + direccion * (velocidadPxPorSeg * dt);

                game::Vec2 posicionResuelta = mazmorra.ResolverColision(
                    lider.Colisionador(), posicionActual, posicionDeseada);
                lider.SetPosicion(posicionResuelta);

                party.ActualizarFormacion(dt);

                // Enemigos agresivos (ver game::EsAgresivo): persiguen al
                // lider si esta a menos de kRadioDeteccionAgresivo, con
                // colision contra las paredes igual que el jugador (sin
                // pathfinding — si el lider se les esconde detras de una
                // pared quedan trabados contra ella, aceptable para el
                // prototipo). Si alguno llega a kRadioContactoAgresivo,
                // fuerza el combate de su sala sin esperar [E].
                int salaAgresorEnContacto = -1;
                for (auto& e : enemigos) {
                    if (e.Vencido() || !game::EsAgresivo(e.Tipo())) continue;
                    game::Vec2 haciaLider = lider.Posicion() - e.Posicion();
                    float distancia = game::Length(haciaLider);
                    if (distancia > kRadioDeteccionAgresivo) continue;

                    if (distancia > 1.0f) {
                        game::Vec2 direccionPersecucion = game::Normalize(haciaLider);
                        float velocidadEnemigo = e.GetStats().velocidad * kFactorVelocidadAgresivo * kFactorVelocidadExploracion;
                        game::Vec2 posDeseadaEnemigo = e.Posicion() + direccionPersecucion * (velocidadEnemigo * dt);
                        game::Vec2 posResueltaEnemigo = mazmorra.ResolverColision(
                            e.Colisionador(), e.Posicion(), posDeseadaEnemigo);
                        e.SetPosicion(posResueltaEnemigo);
                    }

                    if (distancia <= kRadioContactoAgresivo) {
                        salaAgresorEnContacto = e.Sala();
                    }
                }
                if (salaAgresorEnContacto >= 0) {
                    EngancharCombateConSala(salaAgresorEnContacto);
                }

                // Trampas de piso (ver game::Trampa): se chequean DESPUES de
                // resolver todo el movimiento de este frame (lider y
                // enemigos agresivos ya en su posicion final), asi que el
                // area de contacto es la que realmente se ve en pantalla.
                // Afectan por igual al lider y a los enemigos -- pedido
                // explicito del usuario, para poder atraer a un perseguidor
                // agresivo sobre una trampa como jugada tactica.
                lider.ActualizarCooldownTrampa(dt);
                if (lider.CooldownTrampa() <= 0.0f) {
                    for (const auto& trampa : mazmorra.Trampas()) {
                        if (!game::CheckCollision(lider.Colisionador(), trampa.area)) continue;
                        AplicarDanoDeTrampa(
                            trampa.tipo,
                            [&](int dano) { lider.RecibirDano(dano); },
                            [&](float duracion) { lider.ReiniciarCooldownTrampa(duracion); });
                        // Las trampas desgastan pero no matan fuera de
                        // combate (no hay pantalla de Game Over fuera de un
                        // CombatEncounter) -- rematar al lider queda para un
                        // enemigo, no para el escenario.
                        if (lider.GetStats().hp <= 0) lider.GetStatsMut().hp = 1;
                        break;  // una trampa por chequeo alcanza
                    }
                }

                for (auto& e : enemigos) {
                    if (e.Vencido()) continue;
                    e.ActualizarCooldownTrampa(dt);
                    if (e.CooldownTrampa() > 0.0f) continue;
                    for (const auto& trampa : mazmorra.Trampas()) {
                        if (!game::CheckCollision(e.Colisionador(), trampa.area)) continue;
                        AplicarDanoDeTrampa(
                            trampa.tipo,
                            [&](int dano) { e.RecibirDano(dano); },
                            [&](float duracion) { e.ReiniciarCooldownTrampa(duracion); });
                        // A un enemigo si lo puede rematar una trampa -- es
                        // justamente el atractivo tactico de atraer a un
                        // agresivo hasta una (ya soportado: Vencido() lo saca
                        // de la persecucion y del dibujado sin tocar nada
                        // mas).
                        if (!e.EstaVivo()) e.MarcarVencido();
                        break;
                    }
                }

                if (IsKeyPressed(KEY_TAB)) fichaAbierta = true;

                // Interactuable mas cercano: el enemigo vivo o el cofre sin
                // abrir mas cercano, si esta a distancia de interaccion —
                // [E] enganchar combate o abrir cofre, segun cual sea.
                game::Enemy* enemigoCercano = nullptr;
                game::Cofre* cofreCercano = nullptr;
                float distanciaCercana = kDistanciaInteraccion;
                for (auto& e : enemigos) {
                    if (e.Vencido()) continue;
                    float distancia = game::Length(lider.Posicion() - e.Posicion());
                    if (distancia < distanciaCercana) {
                        distanciaCercana = distancia;
                        enemigoCercano = &e;
                        cofreCercano = nullptr;
                    }
                }
                for (auto& c : cofres) {
                    if (c.abierto) continue;
                    float distancia = game::Length(lider.Posicion() - c.posicion);
                    if (distancia < distanciaCercana) {
                        distanciaCercana = distancia;
                        cofreCercano = &c;
                        enemigoCercano = nullptr;
                    }
                }

                if (enemigoCercano != nullptr) {
                    prompt = "[E] Atacar";
                } else if (cofreCercano != nullptr) {
                    prompt = "[E] Abrir cofre";
                }

                if (IsKeyPressed(KEY_E)) {
                    if (enemigoCercano != nullptr) {
                        EngancharCombateConSala(enemigoCercano->Sala());
                    } else if (cofreCercano != nullptr) {
                        cofreCercano->abierto = true;
                        party.Inventario().Agregar(cofreCercano->contenido);
                        // Oro (ver "Fuente del oro" en docs/design.md):
                        // ademas del item de siempre, cada cofre deja oro
                        // segun la Dificultad de la mazmorra activa.
                        Dificultad dificultadDelCofre = (mazmorraActivaIndice >= 0)
                            ? DificultadDeIndiceCombinado(mazmorraActivaIndice) : Dificultad::Media;
                        int oroDelCofre = OroDeCofrePorDificultad(dificultadDelCofre);
                        party.GanarOro(oroDelCofre);
                        mensajeFlotante = "Encontraste: " + cofreCercano->contenido.nombre
                            + "  +" + std::to_string(oroDelCofre) + " oro";
                        timerMensaje = kDuracionMensaje;
                    }
                }
            }

            if (inventarioAbierto) {
                BeginDrawing();
                renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada));
                ui::DibujarInventario(party, objetivoInventario, renderer.Sprites());
                EndDrawing();
            } else if (fichaAbierta) {
                BeginDrawing();
                renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada));
                ui::DibujarFichaPersonajes(party, renderer.Sprites());
                EndDrawing();
            } else {
                // El panel de siempre queda fijo en modo compacto (ver el
                // comentario de fichaAbierta mas arriba) — 'false' hardcodeado
                // en vez de una variable que ya no existe.
                renderer.DibujarFrame(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada),
                                       /*panelExpandido*/false, prompt, mensajeFlotante);
            }
        } else if (estado == EstadoJuego::Ciudad) {
            // Mismo timer que ya usa mensajeFlotante en Exploracion.
            if (timerMensaje > 0.0f) {
                timerMensaje -= dt;
                if (timerMensaje <= 0.0f) {
                    timerMensaje = 0.0f;
                    mensajeFlotante.clear();
                }
            }

            // Mismo orden "cerrar lo de encima antes" que en Exploracion:
            // inventario, ficha, y recien con los dos cerrados abre la
            // pausa.
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (inventarioAbierto) {
                    inventarioAbierto = false;
                } else if (fichaAbierta) {
                    fichaAbierta = false;
                } else {
                    opcionPausaSeleccionada = 0;
                    estadoPrevioAPausa = EstadoJuego::Ciudad;
                    estado = EstadoJuego::Pausa;
                }
            }

            if (!fichaAbierta && IsKeyPressed(KEY_I)) inventarioAbierto = !inventarioAbierto;

            // F5 tambien guarda desde la Ciudad, igual que en Exploracion —
            // ver game::kPantallaCiudad en el bloque de SeleccionSlot mas
            // abajo, que es quien decide que se guarda "parado en la
            // Ciudad" en vez de "parado en el mapa".
            if (!fichaAbierta && IsKeyPressed(KEY_F5)) {
                modoGuardarSlot = true;
                estadoAlCancelarSlot = EstadoJuego::Ciudad;
                opcionSlotSeleccionada = 0;
                mensajeSlot.clear();
                estado = EstadoJuego::SeleccionSlot;
            }

            std::string prompt;

            if (inventarioAbierto) {
                // Mismo bloque que Exploracion (opera solo sobre el party,
                // no depende de que haya una mazmorra "real" cargada).
                auto& miembros = party.Miembros();
                if (IsKeyPressed(KEY_TAB) && !miembros.empty()) {
                    objetivoInventario = (objetivoInventario + 1) % miembros.size();
                }
                int indice = NumeroPresionado();
                if (indice >= 0 && objetivoInventario < miembros.size()) {
                    const auto& pilas = party.Inventario().Pilas();
                    if (static_cast<size_t>(indice) < pilas.size()) {
                        bool esConsumibleDeCombate = pilas[indice].item.efecto == game::EfectoItem::AplicarEstado
                            || pilas[indice].item.efecto == game::EfectoItem::CurarEstados;
                        if (esConsumibleDeCombate) {
                            mensajeFlotante = pilas[indice].item.nombre + " solo se puede usar en combate.";
                            timerMensaje = kDuracionMensaje;
                        } else if (pilas[indice].item.tipo == game::TipoItem::Consumible) {
                            game::ResultadoUsoItem resultado = party.Inventario().Usar(
                                static_cast<size_t>(indice), miembros[objetivoInventario]);
                            if (resultado.exitoso) {
                                mensajeFlotante = resultado.texto;
                                timerMensaje = kDuracionMensaje;
                            }
                        } else {
                            game::ResultadoEquipar resultado = party.Inventario().Equipar(
                                static_cast<size_t>(indice), miembros[objetivoInventario]);
                            if (resultado.exitoso) {
                                mensajeFlotante = resultado.texto;
                                timerMensaje = kDuracionMensaje;
                            }
                        }
                    }
                }
            } else if (fichaAbierta) {
                if (IsKeyPressed(KEY_TAB)) fichaAbierta = false;
            } else {
                // Movimiento + colision: mismo codigo que Exploracion, pero
                // sin persecucion de agresivos ni trampas de piso (la
                // Ciudad no tiene ninguno de los dos — ver ConstruirCiudad).
                game::Vec2 direccion = input::LeerDireccionMovimiento();
                game::Character& lider = party.Lider();

                float velocidadPxPorSeg = lider.GetStats().velocidad * kFactorVelocidadExploracion;
                game::Vec2 posicionActual = lider.Posicion();
                game::Vec2 posicionDeseada = posicionActual + direccion * (velocidadPxPorSeg * dt);

                game::Vec2 posicionResuelta = mazmorra.ResolverColision(
                    lider.Colisionador(), posicionActual, posicionDeseada);
                lider.SetPosicion(posicionResuelta);

                party.ActualizarFormacion(dt);

                // Perro/pajaros/aldeanos siguen deambulando mientras el
                // jugador esta parado en la Ciudad (ver game::Deambulante) —
                // puramente visual (el perro y los pajaros), salvo los
                // aldeanos, a los que se les puede hablar con [E] (ver mas
                // abajo) — ninguno participa de la colision.
                for (auto& deambulante : deambulantesCiudad) {
                    game::ActualizarDeambulante(deambulante, dt);
                }

                if (IsKeyPressed(KEY_TAB)) fichaAbierta = true;

                // Interactuable mas cercano: un edificio o un aldeano (ver
                // game::EsAldeano — el perro y los pajaros no cuentan),
                // el que este mas cerca gana el prompt de abajo. Mismo
                // criterio de distancia que un enemigo/cofre en Exploracion.
                game::Edificio* edificioCercano = nullptr;
                game::Deambulante* aldeanoCercano = nullptr;
                float distanciaCercana = kDistanciaInteraccion;
                for (auto& edificio : edificiosCiudad) {
                    float distancia = game::Length(lider.Posicion() - edificio.posicion);
                    if (distancia < distanciaCercana) {
                        distanciaCercana = distancia;
                        edificioCercano = &edificio;
                        aldeanoCercano = nullptr;
                    }
                }
                for (auto& deambulante : deambulantesCiudad) {
                    if (!game::EsAldeano(deambulante.tipo)) continue;
                    float distancia = game::Length(lider.Posicion() - deambulante.posicion);
                    if (distancia < distanciaCercana) {
                        distanciaCercana = distancia;
                        aldeanoCercano = &deambulante;
                        edificioCercano = nullptr;
                    }
                }

                if (edificioCercano != nullptr) {
                    // La Entrada a las mazmorras ya lleva "Entrada" en su
                    // propio nombre (ver game::NombreDeEdificio) — anteponer
                    // "Entrar a" ahi sonaba redundante ("Entrar a Entrada a
                    // las mazmorras"), asi que ese caso arma su propio texto.
                    prompt = (edificioCercano->tipo == game::TipoEdificio::EntradaMazmorras)
                        ? "[E] Ir a la Entrada a las mazmorras"
                        : std::string("[E] Entrar a ") + game::NombreDeEdificio(edificioCercano->tipo);
                } else if (aldeanoCercano != nullptr) {
                    prompt = "[E] Hablar";
                }

                if (IsKeyPressed(KEY_E) && edificioCercano != nullptr) {
                    if (edificioCercano->tipo == game::TipoEdificio::Herreria) {
                        mensajeFlotante.clear();
                        timerMensaje = 0.0f;
                        estado = EstadoJuego::Herreria;
                    } else if (edificioCercano->tipo == game::TipoEdificio::Tienda) {
                        mensajeFlotante.clear();
                        timerMensaje = 0.0f;
                        estado = EstadoJuego::Tienda;
                    } else if (edificioCercano->tipo == game::TipoEdificio::Academia) {
                        mensajeFlotante.clear();
                        timerMensaje = 0.0f;
                        estado = EstadoJuego::Academia;
                    } else {  // EntradaMazmorras
                        // Mismo fondo "de entrada" que el juego mostraba
                        // antes de que existiera la Ciudad (ver
                        // 'fondoInicial' al arrancar main()) — un tema y
                        // dificultad fijos, solo para tener un fondo
                        // variado detras del mapa (el party todavia no
                        // entra ahi, ver EstadoJuego::MapaTema).
                        MazmorraGenerada fondo = GenerarMazmorra(Tema::Bosque, Dificultad::Media);
                        mazmorra = std::move(fondo.mazmorra);
                        posicionInicial = fondo.posicionInicial;
                        enemigos = std::move(fondo.enemigos);
                        cofres = std::move(fondo.cofres);
                        temaMazmorraCargada = Tema::Bosque;
                        edificiosCiudad.clear();
                        fuentesCiudad.clear();
                        deambulantesCiudad.clear();
                        enCiudad = false;
                        opcionMapaTemaSeleccionada = 0;
                        estado = EstadoJuego::MapaTema;
                    }
                } else if (IsKeyPressed(KEY_E) && aldeanoCercano != nullptr) {
                    // Flavor text nomas — ver FraseDeAldeano mas arriba. Usa
                    // el mismo mensajeFlotante/timerMensaje que ya muestra
                    // el botin de un cofre.
                    mensajeFlotante = FraseDeAldeano(aldeanoCercano->tipo);
                    timerMensaje = kDuracionMensaje;
                }
            }

            if (inventarioAbierto) {
                BeginDrawing();
                renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada),
                                                edificiosCiudad, deambulantesCiudad, fuentesCiudad);
                ui::DibujarInventario(party, objetivoInventario, renderer.Sprites());
                EndDrawing();
            } else if (fichaAbierta) {
                BeginDrawing();
                renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada),
                                                edificiosCiudad, deambulantesCiudad, fuentesCiudad);
                ui::DibujarFichaPersonajes(party, renderer.Sprites());
                EndDrawing();
            } else {
                renderer.DibujarFrame(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada),
                                       /*panelExpandido*/false, prompt, mensajeFlotante,
                                       edificiosCiudad, deambulantesCiudad, fuentesCiudad);
            }
        } else if (estado == EstadoJuego::Herreria || estado == EstadoJuego::Tienda) {
            if (timerMensaje > 0.0f) {
                timerMensaje -= dt;
                if (timerMensaje <= 0.0f) {
                    timerMensaje = 0.0f;
                    mensajeFlotante.clear();
                }
            }

            bool esHerreria = (estado == EstadoJuego::Herreria);
            const OfertaComercio* tabla = esHerreria ? kOfertasHerreria : kOfertasTienda;
            int numOfertas = esHerreria ? kNumOfertasHerreria : kNumOfertasTienda;

            if (IsKeyPressed(KEY_ESCAPE)) {
                mensajeFlotante.clear();
                timerMensaje = 0.0f;
                estado = EstadoJuego::Ciudad;
            } else {
                int indice = NumeroPresionado();
                if (indice >= 0 && indice < numOfertas) {
                    game::Item item = tabla[indice].fabricar();
                    if (party.Oro() >= tabla[indice].precio) {
                        party.GanarOro(-tabla[indice].precio);
                        party.Inventario().Agregar(item);
                        mensajeFlotante = "Compraste: " + item.nombre + ".";
                    } else {
                        mensajeFlotante = "No te alcanza el oro para " + item.nombre + ".";
                    }
                    timerMensaje = kDuracionMensaje;
                }
            }

            // Se arma de nuevo cada frame a partir del catalogo de precios
            // (kOfertasHerreria/kOfertasTienda) -- son a lo sumo 5 items,
            // costo insignificante, y asi el nombre/descripcion mostrados
            // siempre salen del game::Item real (una sola fuente de verdad
            // para ese texto, ver ui::OfertaComercio en menu_ui.h).
            std::vector<ui::OfertaComercio> ofertasUi;
            ofertasUi.reserve(numOfertas);
            for (int i = 0; i < numOfertas; ++i) {
                game::Item muestra = tabla[i].fabricar();
                ofertasUi.push_back(ui::OfertaComercio{ muestra.nombre, muestra.descripcion, tabla[i].precio });
            }

            BeginDrawing();
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada),
                                            edificiosCiudad, deambulantesCiudad, fuentesCiudad);
            ui::DibujarComercio(anchoVentana, altoVentana, esHerreria, ofertasUi, party.Oro(), mensajeFlotante);
            EndDrawing();
        } else if (estado == EstadoJuego::Academia) {
            // Tercer edificio de la Ciudad (ver ConstruirCiudad): aprender
            // las habilidades de game::kNivelMejoraHabilidad/
            // kNivelHabilidadNueva -- 2 filas por personaje (mejora, nueva),
            // orden fijo [1]-[8] = (personaje 0 mejora, personaje 0 nueva,
            // personaje 1 mejora, ...), mismo criterio numerico simple que
            // ya usan Herreria/Tienda con su catalogo.
            if (timerMensaje > 0.0f) {
                timerMensaje -= dt;
                if (timerMensaje <= 0.0f) {
                    timerMensaje = 0.0f;
                    mensajeFlotante.clear();
                }
            }

            auto& miembrosAcademia = party.Miembros();

            if (IsKeyPressed(KEY_ESCAPE)) {
                mensajeFlotante.clear();
                timerMensaje = 0.0f;
                estado = EstadoJuego::Ciudad;
            } else {
                int indice = NumeroPresionado();
                if (indice >= 0 && (size_t)indice < miembrosAcademia.size() * 2) {
                    size_t indicePersonaje = (size_t)indice / 2;
                    bool esNueva = (indice % 2) == 1;
                    game::Character& personaje = miembrosAcademia[indicePersonaje];
                    if (esNueva && personaje.HabilidadNuevaDisponible()) {
                        personaje.AprenderHabilidadNueva();
                        mensajeFlotante = personaje.Nombre() + " aprendio " + game::NombreHabilidadNueva(personaje.Rol()) + ".";
                        timerMensaje = kDuracionMensaje;
                    } else if (!esNueva && personaje.MejoraHabilidadDisponible()) {
                        personaje.AprenderMejoraHabilidad();
                        mensajeFlotante = personaje.Nombre() + " mejoro " + std::string(game::NombreHabilidadDeRol(personaje.Rol())) + ".";
                        timerMensaje = kDuracionMensaje;
                    }
                    // Si la fila no esta disponible (ya aprendida, o nivel
                    // insuficiente) no hace nada -- mismo criterio que
                    // Comercio con oro insuficiente, pero sin mensaje de
                    // rechazo (el estado ya se ve claro en la lista misma).
                }
            }

            std::vector<ui::FilaAcademia> filasAcademia;
            filasAcademia.reserve(miembrosAcademia.size() * 2);
            for (auto& personaje : miembrosAcademia) {
                std::string nombrePersonaje = personaje.Nombre() + " (" + game::RoleName(personaje.Rol()) + ") - Nv."
                    + std::to_string(personaje.Nivel());

                ui::FilaAcademia filaMejora;
                filaMejora.nombrePersonaje = nombrePersonaje;
                filaMejora.nombreHabilidad = std::string("Mejorar ") + game::NombreHabilidadDeRol(personaje.Rol());
                filaMejora.nivelRequerido = game::kNivelMejoraHabilidad;
                filaMejora.aprendida = personaje.MejoraHabilidadAprendida();
                filaMejora.disponible = personaje.MejoraHabilidadDisponible();
                filasAcademia.push_back(filaMejora);

                ui::FilaAcademia filaNueva;
                filaNueva.nombrePersonaje = nombrePersonaje;
                filaNueva.nombreHabilidad = std::string("Aprender ") + game::NombreHabilidadNueva(personaje.Rol());
                filaNueva.nivelRequerido = game::kNivelHabilidadNueva;
                filaNueva.aprendida = personaje.HabilidadNuevaAprendida();
                filaNueva.disponible = personaje.HabilidadNuevaDisponible();
                filasAcademia.push_back(filaNueva);
            }

            BeginDrawing();
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada),
                                            edificiosCiudad, deambulantesCiudad, fuentesCiudad);
            ui::DibujarAcademia(anchoVentana, altoVentana, filasAcademia, mensajeFlotante);
            EndDrawing();
        } else if (estado == EstadoJuego::Pausa) {
            // Mismo timer que ya usa mensajeFlotante en Exploracion
            // (reutilizado tal cual, no uno aparte) — si el jugador pausa
            // con un cartel de botin/combate todavia visible, sigue
            // contando y desaparece igual (mensajeFlotante nunca se ata a
            // una pantalla en particular, solo al tiempo transcurrido).
            // "Guardar" ya no lo usa: ahora abre la seleccion de slot, que
            // tiene su propio mensajeSlot (sin timer, ver mas abajo).
            if (timerMensaje > 0.0f) {
                timerMensaje -= dt;
                if (timerMensaje <= 0.0f) {
                    timerMensaje = 0.0f;
                    mensajeFlotante.clear();
                }
            }

            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                opcionPausaSeleccionada = (opcionPausaSeleccionada + 1) % ui::kNumOpcionesPausa;
            } else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                opcionPausaSeleccionada = (opcionPausaSeleccionada + ui::kNumOpcionesPausa - 1) % ui::kNumOpcionesPausa;
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                // ESC en la pausa vuelve directo a jugar, como elegir
                // "Continuar" — es el uso mas comun (pausar por las dudas,
                // chequear que se puede guardar/salir, seguir jugando) y
                // evita que el jugador tenga que navegar hasta "Continuar"
                // a mano cada vez. "Jugar" ahora puede ser Exploracion O
                // Mapa, segun desde donde se abrio esta pausa (ver
                // estadoPrevioAPausa, seteado en ambos puntos de entrada).
                estado = estadoPrevioAPausa;
            } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                switch (static_cast<ui::OpcionPausa>(opcionPausaSeleccionada)) {
                    case ui::OpcionPausa::Continuar:
                        estado = estadoPrevioAPausa;
                        break;
                    case ui::OpcionPausa::Guardar: {
                        // Misma pantalla de seleccion de slot que dispara F5
                        // en la exploracion — no hay dos caminos distintos
                        // para guardar, solo dos formas de llegar a ella.
                        modoGuardarSlot = true;
                        estadoAlCancelarSlot = EstadoJuego::Pausa;
                        opcionSlotSeleccionada = 0;
                        mensajeSlot.clear();
                        estado = EstadoJuego::SeleccionSlot;
                        break;
                    }
                    case ui::OpcionPausa::VolverAlMapa:
                        // "Volver a la ciudad" (ver el comentario de este
                        // enum en menu_ui.h): abandona la mazmorra en curso
                        // sin marcarla como superada y vuelve a la Ciudad —
                        // el party sigue tal cual esta (HP, inventario,
                        // equipo, oro: "sigue con el desgaste" entre
                        // mazmorras). Ya no vuelve directo a MapaTema (solo
                        // se llega ahi caminando hasta la Entrada a las
                        // mazmorras dentro de la Ciudad). Si la pausa se
                        // abrio desde la propia Ciudad (estadoPrevioAPausa
                        // ya es Ciudad) esto es exactamente lo mismo que
                        // "Continuar".
                        mazmorraActivaIndice = -1;
                        opcionMapaTemaSeleccionada = 0;
                        EntrarALaCiudad();
                        break;
                    case ui::OpcionPausa::MenuPrincipal:
                        // No toca party/mapa/mazmorra/enemigos/cofres —
                        // siguen ahi tal cual, sirviendo de fondo "congelado"
                        // para el menu de inicio (mismo truco visual de
                        // siempre), por si el jugador vuelve a elegir
                        // "Continuar"... que en MenuInicio no existe: para
                        // retomar tiene que guardar antes y despues elegir
                        // "Cargar", o elegir "Nueva partida" y perder este
                        // progreso. Se limpia el cartel de guardado para no
                        // arrastrar un "Partida guardada." viejo a una
                        // pantalla donde ya no tiene sentido.
                        mensajeFlotante.clear();
                        timerMensaje = 0.0f;
                        opcionMenuSeleccionada = 0;
                        estado = EstadoJuego::MenuInicio;
                        break;
                    case ui::OpcionPausa::Salir:
                        salirDelJuego = true;
                        break;
                }
            }

            BeginDrawing();
            // 'edificiosCiudad'/'deambulantesCiudad'/'fuentesCiudad' vienen
            // vacios salvo que la pausa se haya abierto desde la Ciudad (ver
            // enCiudad) — en ese caso se ven de fondo, igual que se verian
            // los enemigos/cofres de una mazmorra si la pausa se abrio desde
            // ahi.
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada),
                                            edificiosCiudad, deambulantesCiudad, fuentesCiudad);
            ui::DibujarPausa(anchoVentana, altoVentana, opcionPausaSeleccionada, mensajeFlotante);
            EndDrawing();
        } else if (estado == EstadoJuego::SeleccionSlot) {
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                opcionSlotSeleccionada = (opcionSlotSeleccionada + 1) % ui::kNumOpcionesSlot;
            } else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                opcionSlotSeleccionada = (opcionSlotSeleccionada + ui::kNumOpcionesSlot - 1) % ui::kNumOpcionesSlot;
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                mensajeSlot.clear();
                estado = estadoAlCancelarSlot;
            } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                bool esVolver = (opcionSlotSeleccionada == game::kNumSlots);
                if (esVolver) {
                    mensajeSlot.clear();
                    estado = estadoAlCancelarSlot;
                } else {
                    int slot = opcionSlotSeleccionada;
                    if (modoGuardarSlot) {
                        // 'pantalla' se deduce de enCiudad/mazmorraActivaIndice
                        // (ver game::kPantallaCiudad/kPantallaMapa/
                        // kPantallaExploracion en save.h): la Ciudad tiene
                        // prioridad porque tambien deja mazmorraActivaIndice
                        // en -1 (igual que MapaTema/MapaDificultad) — sin
                        // chequear enCiudad primero las dos pantallas
                        // quedarian indistinguibles, igual que antes de la
                        // Ciudad (ver el comentario de kVersion sobre v6 en
                        // save.cpp).
                        int pantalla = enCiudad ? game::kPantallaCiudad
                            : (mazmorraActivaIndice < 0 ? game::kPantallaMapa : game::kPantallaExploracion);
                        bool guardado = game::GuardarPartida(slot, mazmorra, party, enemigos, cofres,
                                                              mazmorraSuperada, pantalla, mazmorraActivaIndice);
                        mensajeSlot = guardado
                            ? ("Guardado en Slot " + std::to_string(slot + 1) + ".")
                            : ("No se pudo guardar en Slot " + std::to_string(slot + 1) + ".");
                        hayGuardado[slot] = hayGuardado[slot] || guardado;
                    } else if (hayGuardado[slot]) {
                        game::ResultadoCarga carga = game::CargarPartida(slot);
                        if (carga.valido) {
                            // Party/inventario/oro/progreso del mapa: comun
                            // a las 3 pantallas posibles (ver game::
                            // kPantallaCiudad/kPantallaMapa/
                            // kPantallaExploracion en save.h).
                            party = game::Party(std::move(carga.datos.miembros));
                            party.GanarOro(carga.datos.oro);
                            for (auto& pila : carga.datos.pilasInventario) {
                                party.Inventario().Agregar(std::move(pila.item), pila.cantidad);
                            }
                            for (int i = 0; i < game::kNumCombinacionesMapa; ++i) {
                                mazmorraSuperada[i] = carga.datos.mazmorraSuperada[i];
                            }
                            mazmorraActivaIndice = carga.datos.mazmorraActivaIndice;
                            opcionMapaTemaSeleccionada = 0;

                            if (carga.datos.pantalla == game::kPantallaCiudad) {
                                // La Ciudad es siempre el mismo layout (ver
                                // ConstruirCiudad) — se reconstruye de cero
                                // en vez de usar las habitaciones/paredes/
                                // trampas guardadas (serian identicas de
                                // todos modos) para no tener que guardar
                                // tambien los 3 edificios, que no viajan en
                                // el archivo (ver el comentario de
                                // game::Edificio en edificio.h).
                                CiudadGenerada generada = ConstruirCiudad();
                                mazmorra = std::move(generada.mazmorra);
                                posicionInicial = generada.posicionInicial;
                                enemigos.clear();
                                cofres.clear();
                                edificiosCiudad = std::move(generada.edificios);
                                fuentesCiudad = std::move(generada.fuentes);
                                deambulantesCiudad = std::move(generada.deambulantes);
                                temaMazmorraCargada = static_cast<Tema>(render::kTemaCiudad);
                                enCiudad = true;
                                mazmorraActivaIndice = -1;
                                estado = EstadoJuego::Ciudad;
                            } else {
                                // MapaTema/MapaDificultad (fondo generico) o
                                // una mazmorra real a mitad de explorar —
                                // las dos comparten el mismo formato de
                                // datos, solo cambia el estado destino y si
                                // 'mazmorraActivaIndice' es valido.
                                mazmorra = game::Dungeon(std::move(carga.datos.habitaciones), std::move(carga.datos.paredes),
                                                          std::move(carga.datos.trampas));
                                posicionInicial = mazmorra.CentroDeSala(0);
                                enemigos = std::move(carga.datos.enemigos);
                                cofres = std::move(carga.datos.cofres);
                                edificiosCiudad.clear();
                                fuentesCiudad.clear();
                                deambulantesCiudad.clear();
                                enCiudad = false;
                                if (mazmorraActivaIndice >= 0) {
                                    temaMazmorraCargada = TemaDeIndiceCombinado(mazmorraActivaIndice);
                                    temaMapaElegido = temaMazmorraCargada;
                                }
                                estado = (carga.datos.pantalla == game::kPantallaExploracion)
                                    ? EstadoJuego::Exploracion : EstadoJuego::MapaTema;
                            }

                            party.ReiniciarFormacion(party.Lider().Posicion());
                            mensajeSlot.clear();
                        } else {
                            mensajeSlot = "Ese archivo esta dañado.";
                        }
                    }
                    // Si el slot esta vacio en modo Cargar, ENTER no hace
                    // nada (se dibuja deshabilitado, ver
                    // ui::DibujarSeleccionSlot) — mismo criterio que
                    // "Cargar" deshabilitada en el menu de inicio.
                }
            }

            BeginDrawing();
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada),
                                            edificiosCiudad, deambulantesCiudad, fuentesCiudad);
            ui::DibujarSeleccionSlot(anchoVentana, altoVentana, opcionSlotSeleccionada, modoGuardarSlot, hayGuardado, mensajeSlot);
            EndDrawing();
        } else {  // EstadoJuego::Combate
            encuentro->Actualizar(dt);

            if (encuentro->Fase() == game::FaseCombate::TurnoAliado) {
                if (menuItemCombateAbierto) {
                    // Con el sub-menu de items abierto se congelan el resto
                    // de las acciones del turno: ESC lo cierra sin gastar el
                    // turno, TAB cicla a que aliado se le aplica el proximo
                    // item (para Bomba de Veneno, que apunta a un enemigo,
                    // no hace falta -- usa el objetivo ya elegido en la
                    // pantalla principal) y 1-9 lo usa.
                    auto& miembros = party.Miembros();
                    if (IsKeyPressed(KEY_ESCAPE)) {
                        menuItemCombateAbierto = false;
                    } else {
                        if (IsKeyPressed(KEY_TAB) && !miembros.empty()) {
                            objetivoAliadoItemCombate = (objetivoAliadoItemCombate + 1) % miembros.size();
                        }
                        int indice = NumeroPresionado();
                        if (indice >= 0) {
                            game::Character* actorAntes = encuentro->AliadoEnTurno();
                            encuentro->AccionUsarItem(static_cast<size_t>(indice), objetivoAliadoItemCombate);
                            // Si el turno se consumio (cambio de fase, o le
                            // toca a otro aliado) el sub-menu se cierra solo;
                            // si el item no se pudo usar (indice invalido,
                            // sin ese item, aliado caido) sigue abierto para
                            // que el jugador pruebe otra cosa.
                            if (encuentro->Fase() != game::FaseCombate::TurnoAliado
                                || encuentro->AliadoEnTurno() != actorAntes) {
                                menuItemCombateAbierto = false;
                            }
                        }
                    }
                } else if (IsKeyPressed(KEY_ONE)) {
                    encuentro->AccionAtaqueBasico();
                } else if (IsKeyPressed(KEY_TWO)) {
                    encuentro->AccionHabilidadDeRol();
                } else if (IsKeyPressed(KEY_THREE)) {
                    menuItemCombateAbierto = true;
                } else if (IsKeyPressed(KEY_FOUR)) {
                    // Segunda habilidad de rol (ver game::NombreHabilidadNueva
                    // y la Academia en la Ciudad) -- AccionHabilidadNueva ya
                    // no hace nada sola si el personaje en turno todavia no
                    // la aprendio, asi que no hace falta chequear aca (mismo
                    // criterio defensivo que ya usan las otras Accion*).
                    encuentro->AccionHabilidadNueva();
                } else if (IsKeyPressed(KEY_TAB)) {
                    // Con mas de un enemigo vivo, cambia a quien le apuntan
                    // las acciones del aliado en turno (ver combat_ui.cpp).
                    encuentro->CiclarObjetivo();
                }
            } else if (encuentro->Fase() == game::FaseCombate::Ganado) {
                if (!lootRepartido) {
                    // Botin: se tira una vez por cada enemigo del encuentro
                    // (independiente de si el jugador puede haber visto un
                    // "Ganado" repetido en frames previos, por lootRepartido).
                    // Dificultad::Media de respaldo si por algun motivo no
                    // hay mazmorra activa (no deberia pasar nunca: a Combate
                    // solo se llega desde una mazmorra generada por el mapa).
                    Dificultad dificultadActual = (mazmorraActivaIndice >= 0)
                        ? DificultadDeIndiceCombinado(mazmorraActivaIndice) : Dificultad::Media;
                    std::string botin;
                    int oroGanado = 0;
                    for (game::Enemy* e : encuentro->Enemigos()) {
                        oroGanado += OroDeEnemigoPorDificultad(e->Tipo(), dificultadActual);
                        game::ResultadoLoot loot = LootDeEnemigoPorDificultad(e->Tipo(), dificultadActual);
                        if (loot.hay) {
                            party.Inventario().Agregar(loot.item);
                            if (!botin.empty()) botin += ", ";
                            botin += loot.item.nombre;
                        }
                    }
                    party.GanarOro(oroGanado);

                    // --- Experiencia (ver "Sistema de niveles" en
                    // docs/design.md): cada enemigo del encuentro da XP
                    // segun su rol (game::XpPorEnemigo, mismo criterio que
                    // el loot de arriba), y CADA miembro del party que siga
                    // con vida al ganar el combate la recibe COMPLETA — no
                    // se reparte entre los 4, nivel individual por
                    // personaje. Character::GanarExperiencia aplica sola el
                    // crecimiento de stats de su rol si sube de nivel.
                    //
                    // --- Recurso (ver "Regeneracion de recurso" en
                    // docs/design.md): en el mismo loop, cada miembro vivo
                    // recupera ademas una porcion de su Resistencia/
                    // Concentracion maxima (Character::RegenerarRecursoPor
                    // Victoria) — sin esto, el recurso quedaba seco para el
                    // resto de la run apenas se gastaba una vez, sin forma
                    // de recuperarlo salvo items o un Game Over. Se aplica
                    // siempre al ganar (no solo cuando hay XP), aunque en la
                    // practica todo encuentro con enemigos otorga XP > 0.
                    int xpGanada = 0;
                    for (game::Enemy* e : encuentro->Enemigos()) xpGanada += game::XpPorEnemigo(e->Tipo());
                    std::string subidasDeNivel;
                    for (auto& miembro : party.Miembros()) {
                        if (!miembro.EstaVivo()) continue;
                        miembro.RegenerarRecursoPorVictoria();
                        if (xpGanada <= 0) continue;
                        int nivelAntes = miembro.Nivel();
                        miembro.GanarExperiencia(xpGanada);
                        if (miembro.Nivel() > nivelAntes) {
                            if (!subidasDeNivel.empty()) subidasDeNivel += " ";
                            subidasDeNivel += miembro.Nombre() + " sube a nivel "
                                + std::to_string(miembro.Nivel()) + "!";
                        }
                    }

                    mensajeFlotante = botin.empty() ? "No encontraste botin esta vez." : ("Botin: " + botin);
                    if (oroGanado > 0) mensajeFlotante += "  +" + std::to_string(oroGanado) + " oro";
                    if (xpGanada > 0) mensajeFlotante += "  +" + std::to_string(xpGanada) + " XP";
                    if (!subidasDeNivel.empty()) mensajeFlotante += "  " + subidasDeNivel;
                    // Mas tiempo en pantalla si ademas hay que leer una o
                    // mas subidas de nivel (mensaje bastante mas largo que
                    // el de botin solo).
                    timerMensaje = subidasDeNivel.empty() ? kDuracionMensaje : kDuracionMensaje * 1.6f;
                    lootRepartido = true;
                    audio.ReproducirVictoria();
                }
                if (GetKeyPressed() != 0) {
                    // Ganarle al jefe (el de esta combinacion tema+
                    // dificultad, siempre solo en su sala — ver
                    // game::EsJefe) marca la mazmorra activa como Superada y
                    // vuelve a la Ciudad (antes volvia directo al mapa,
                    // MapaTema — decision nuestra, no pedida explicitamente:
                    // con la Ciudad como hub central tiene mas sentido que
                    // el jugador pase primero por ahi a gastar el oro/botin
                    // recien ganado antes de elegir la proxima mazmorra, ver
                    // "La Ciudad" en docs/design.md) — ya no queda nada mas
                    // que hacer en esta mazmorra (aunque sigue siendo
                    // rejugable desde el mapa, con un layout nuevo).
                    bool esVictoriaFinal = false;
                    for (game::Enemy* e : encuentro->Enemigos()) {
                        if (game::EsJefe(e->Tipo())) { esVictoriaFinal = true; break; }
                    }
                    if (esVictoriaFinal && mazmorraActivaIndice >= 0) {
                        mazmorraSuperada[mazmorraActivaIndice] = true;
                        mazmorraActivaIndice = -1;
                        opcionMapaTemaSeleccionada = 0;
                        EntrarALaCiudad();
                    } else {
                        estado = EstadoJuego::Exploracion;
                    }
                    encuentro.reset();
                }
            } else if (encuentro->Fase() == game::FaseCombate::Perdido) {
                if (!derrotaSonada) {
                    audio.ReproducirDerrota();
                    derrotaSonada = true;
                }
                if (GetKeyPressed() != 0) {
                    // Game over "de verdad", estilo roguelite (pedido
                    // explicito del usuario: "perder cuesta toda la
                    // partida"): un party wipe reinicia la RUN COMPLETA, no
                    // solo revive en el lugar como antes. El party vuelve a
                    // sus stats de partida (pierde todos los items/equipo/
                    // ORO ganados en la run, ver Party::GanarOro) y el mapa
                    // entero pierde su progreso (ninguna mazmorra queda
                    // Superada); se vuelve a la Ciudad para arrancar de
                    // cero (antes volvia directo al mapa, MapaTema — mismo
                    // criterio que la victoria final de arriba: la Ciudad
                    // es ahora el punto de partida de toda run, gane o
                    // pierda), no a la mazmorra donde se perdio. El NIVEL de
                    // cada personaje sobrevive a esto a proposito — es
                    // progreso permanente del jugador, no de la run (ver
                    // CrearPartyConProgreso y "Sistema de niveles" en
                    // docs/design.md).
                    party = CrearPartyConProgreso(game::Vec2{0.0f, 0.0f}, party);
                    for (int i = 0; i < game::kNumCombinacionesMapa; ++i) mazmorraSuperada[i] = false;
                    mazmorraActivaIndice = -1;
                    opcionMapaTemaSeleccionada = 0;
                    EntrarALaCiudad();
                    encuentro.reset();
                }
            }

            BeginDrawing();
            ClearBackground(BLACK);
            // Se dibuja la mazmorra "congelada" de fondo para dar contexto, y
            // encima la pantalla de combate (que ya trae su propio overlay oscuro).
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, static_cast<int>(temaMazmorraCargada));
            if (encuentro) {
                ui::DibujarCombate(*encuentro, anchoVentana, altoVentana, dt, renderer.Sprites());
                audio.ProcesarEventos(*encuentro);
                if (menuItemCombateAbierto && encuentro->Fase() == game::FaseCombate::TurnoAliado) {
                    ui::DibujarSubmenuUsarItem(encuentro->PartyRef(), objetivoAliadoItemCombate);
                }
            }
            EndDrawing();
        }
    }

    return 0;
}
