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
#include "game/save.h"
#include "render/renderer.h"
#include "render/input.h"
#include "render/combat_ui.h"
#include "render/inventory_ui.h"
#include "render/audio.h"
#include "render/menu_ui.h"

namespace {

// Dificultad de una mazmorra del mapa (ver EstadoJuego::Mapa mas abajo). El
// orden de este enum importa: coincide por CONVENCION (no por dependencia de
// header — render/menu_ui.h no incluye main.cpp) con el orden fijo
// Facil/Media/Dificil que dibuja ui::DibujarMapa, y con el indice 0..2 que
// se guarda en mazmorraSuperada/mazmorraActivaIndice (ver game/save.h).
enum class Dificultad { Facil, Media, Dificil };

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

game::TipoEnemigo TipoAleatorio() {
    switch (game::Roll(3)) {
        case 1:  return game::TipoEnemigo::EsqueletoErrante;
        case 2:  return game::TipoEnemigo::RataGigante;
        default: return game::TipoEnemigo::BanditoAturdidor;
    }
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
// de que existiera el mapa de dificultades).
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
        case game::TipoEnemigo::EsqueletoErrante:
            return game::Enemy(std::string("Esqueleto Errante") + sufijo, tipo,
                Escalado(/*hp*/22, /*ataque*/7, /*defensa*/3, /*velocidad*/80.0f),
                posicion, salaIndice);
        case game::TipoEnemigo::RataGigante:
            return game::Enemy(std::string("Rata Gigante") + sufijo, tipo,
                Escalado(/*hp*/12, /*ataque*/5, /*defensa*/1, /*velocidad*/130.0f),
                posicion, salaIndice);
        case game::TipoEnemigo::BanditoAturdidor:
            return game::Enemy(std::string("Bandido Aturdidor") + sufijo, tipo,
                Escalado(/*hp*/24, /*ataque*/7, /*defensa*/4, /*velocidad*/85.0f),
                posicion, salaIndice);
        default:
            return game::Enemy("Capitan Bandido", tipo,
                Escalado(/*hp*/52, /*ataque*/8, /*defensa*/5, /*velocidad*/85.0f),
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
std::vector<game::Enemy> CrearGrupoDeSala(game::Vec2 centro, int salaIndice, Dificultad dificultad) {
    int cantidad;
    switch (dificultad) {
        case Dificultad::Facil:   cantidad = 1 + game::Roll(2); break;  // 2 o 3
        case Dificultad::Dificil: cantidad = 4 + game::Roll(3); break;  // 5, 6 o 7
        default:                  cantidad = 2 + game::Roll(3); break; // 3, 4 o 5 (Media)
    }
    std::vector<game::Enemy> grupo;
    int vistos[3] = {0, 0, 0};  // contador por TipoEnemigo, para el sufijo

    constexpr int kMaxPorFila = 3;
    constexpr float kSeparacionX = 65.0f;
    constexpr float kSeparacionY = 55.0f;

    for (int j = 0; j < cantidad; ++j) {
        game::TipoEnemigo tipo = TipoAleatorio();
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

// Todo lo que hace falta para generar una mazmorra jugable de la dificultad
// pedida: la mazmorra procedural en si, el punto donde aparece el party al
// entrar, y los enemigos/cofres repartidos por sus salas. A diferencia de la
// vieja PartidaNueva/GenerarPartidaNueva (antes de Round G), esto YA NO
// incluye al party — ahora el party es independiente del mapa (persiste con
// su HP/inventario/equipo actual al pasar de una mazmorra a otra, "sigue con
// el desgaste" — ver EstadoJuego::Mapa) y se crea/resetea aparte, solo en
// "Nueva partida" o tras un Game Over (reinicio total de la run).
struct MazmorraGenerada {
    game::Dungeon mazmorra;
    game::Vec2 posicionInicial;
    std::vector<game::Enemy> enemigos;
    std::vector<game::Cofre> cofres;
};

MazmorraGenerada GenerarMazmorra(Dificultad dificultad) {
    // Mazmorra procedural: una cadena de salas conectadas por pasillos (ver
    // game/dungeon.cpp). La sala 0 es donde arranca el party, sin enemigos;
    // las salas intermedias tienen un grupo de enemigos de tipo aleatorio
    // (cuantos, segun 'dificultad' — ver CrearGrupoDeSala), que se enganchan
    // todos juntos en un mismo combate; la ultima sala, en cambio, tiene un
    // unico Capitan Bandido — el jefe de la mazmorra, que al caer la marca
    // como "Superada" en el mapa (ver "Balance" en docs/design.md y
    // CombatEncounter::Actualizar para su IA especial).
    game::Dungeon mazmorra;
    game::Vec2 posicionInicial = mazmorra.CentroDeSala(0);

    std::vector<game::Enemy> enemigos;
    const auto& salas = mazmorra.Habitaciones();
    size_t indiceSalaJefe = salas.size() - 1;
    for (size_t i = 1; i < salas.size(); ++i) {
        if (i == indiceSalaJefe) {
            enemigos.push_back(CrearEnemigoDeTipo(game::TipoEnemigo::CapitanBandido,
                mazmorra.CentroDeSala(i), static_cast<int>(i), 1, dificultad));
            continue;
        }
        std::vector<game::Enemy> grupo = CrearGrupoDeSala(mazmorra.CentroDeSala(i), static_cast<int>(i), dificultad);
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

// MenuInicio es el estado inicial: pantalla de titulo con las 4 opciones de
// ui::OpcionMenuInicio (ver render/menu_ui.h) antes de largar a explorar.
// SobreMi es la pantalla placeholder de esa opcion (ver ui::DibujarSobreMi) —
// un estado propio, no un sub-estado de MenuInicio, para que se dibuje y se
// lea el input igual que cualquier otra pantalla de la maquina de estados.
// Mapa es la pantalla de seleccion de mazmorra (ver ui::DibujarMapa) — a ella
// se llega al elegir "Nueva partida", al cargar una partida guardada parada
// ahi, al ganarle al jefe de una mazmorra, al elegir "Volver al mapa" en la
// pausa, o tras un Game Over (reinicio total de la run); desde ahi ENTER
// genera una mazmorra nueva de la dificultad elegida y entra a Exploracion.
// Pausa es la pantalla que abre ESC durante la exploracion (ver
// ui::DibujarPausa) — desde ahi se puede volver a jugar, guardar, volver al
// mapa, volver a MenuInicio sin cerrar el juego, o salir. No existe durante
// Combate (ESC no hace nada ahi, igual que F5 tampoco guarda en combate).
// SeleccionSlot es la pantalla de elegir en que slot guardar o de cual
// cargar (ver ui::DibujarSeleccionSlot) — se llega desde F5, desde
// "Guardar" en la pausa, o desde "Cargar" en el menu de inicio;
// 'estadoAlCancelarSlot' (mas abajo) guarda a cual de esos tres volver con
// ESC/"Volver".
enum class EstadoJuego { MenuInicio, SobreMi, Mapa, Exploracion, Pausa, Combate, SeleccionSlot };

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
    // llama a GenerarMazmorra(dificultad) para reemplazar esto.
    game::Party party = CrearPartyDeEjemplo(game::Vec2{0.0f, 0.0f});
    MazmorraGenerada fondoInicial = GenerarMazmorra(Dificultad::Media);
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
    bool panelExpandido = false;     // arranca compacto; TAB lo expande/oculta
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

    // --- Mapa de mazmorras (ver EstadoJuego::Mapa) ---
    bool mazmorraSuperada[game::kNumMazmorrasMapa] = { false, false, false };  // que mazmorras se ganaron esta run
    int mazmorraActivaIndice = -1;   // indice (0..2) de la mazmorra en curso, o -1 si no hay ninguna (parado en el mapa)
    int opcionMapaSeleccionada = 0;  // indice sobre ui::kNumMazmorrasMapa
    // Adonde vuelve "Continuar"/ESC en la pausa — se pisa cada vez que se
    // entra a Pausa (desde Exploracion o desde Mapa), ver mas abajo.
    EstadoJuego estadoPrevioAPausa = EstadoJuego::Exploracion;

    // --- Seleccion de slot (ver EstadoJuego::SeleccionSlot) ---
    int opcionSlotSeleccionada = 0;      // indice sobre ui::kNumOpcionesSlot
    bool modoGuardarSlot = true;         // true = eligiendo donde guardar; false = eligiendo que cargar
    EstadoJuego estadoAlCancelarSlot = EstadoJuego::MenuInicio;  // adonde vuelve ESC/"Volver"
    std::string mensajeSlot;             // resultado de la ultima accion en esta pantalla

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
                        // Reinicio total: party de cero (stats de partida,
                        // sin items ni equipo) y el mapa entero sin ninguna
                        // mazmorra superada — asi el codigo no tiene que
                        // distinguir entre el party/mapa "de fondo" del menu
                        // y una vuelta aca despues de haber jugado (via
                        // Pausa -> Menu principal, ver EstadoJuego::Pausa):
                        // "Nueva partida" siempre arranca una run nueva de
                        // cero. Ya no entra directo a una mazmorra — ahora
                        // primero hay que elegir dificultad en el mapa.
                        party = CrearPartyDeEjemplo(game::Vec2{0.0f, 0.0f});
                        for (int i = 0; i < game::kNumMazmorrasMapa; ++i) mazmorraSuperada[i] = false;
                        mazmorraActivaIndice = -1;
                        opcionMapaSeleccionada = 0;
                        party.ReiniciarFormacion(posicionInicial);
                        estado = EstadoJuego::Mapa;
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
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres);
            ui::DibujarMenuInicio(anchoVentana, altoVentana, opcionMenuSeleccionada, AlgunaPartidaGuardada(hayGuardado));
            EndDrawing();
        } else if (estado == EstadoJuego::SobreMi) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                estado = EstadoJuego::MenuInicio;
            }

            BeginDrawing();
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres);
            ui::DibujarSobreMi(anchoVentana, altoVentana);
            EndDrawing();
        } else if (estado == EstadoJuego::Mapa) {
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                opcionMapaSeleccionada = (opcionMapaSeleccionada + 1) % ui::kNumMazmorrasMapa;
            } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                opcionMapaSeleccionada = (opcionMapaSeleccionada + ui::kNumMazmorrasMapa - 1) % ui::kNumMazmorrasMapa;
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                opcionPausaSeleccionada = 0;
                estadoPrevioAPausa = EstadoJuego::Mapa;
                estado = EstadoJuego::Pausa;
            } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                // Entrar genera SIEMPRE una mazmorra nueva (layout fresco),
                // incluso si esta ya se habia superado antes — las 3
                // mazmorras son rejugables sin limite (pedido explicito:
                // "dificultad creciente, cada una rejugable indefinidamente
                // con un layout regenerado"). El party NO se toca — sigue
                // con el HP/inventario/equipo que traia del mapa ("sigue con
                // el desgaste" entre mazmorras).
                Dificultad dificultad = static_cast<Dificultad>(opcionMapaSeleccionada);
                MazmorraGenerada generada = GenerarMazmorra(dificultad);
                mazmorra = std::move(generada.mazmorra);
                posicionInicial = generada.posicionInicial;
                enemigos = std::move(generada.enemigos);
                cofres = std::move(generada.cofres);
                mazmorraActivaIndice = opcionMapaSeleccionada;
                party.ReiniciarFormacion(posicionInicial);
                estado = EstadoJuego::Exploracion;
            }

            BeginDrawing();
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres);
            ui::DibujarMapa(anchoVentana, altoVentana, opcionMapaSeleccionada, mazmorraSuperada);
            EndDrawing();
        } else if (estado == EstadoJuego::Exploracion) {
            if (timerMensaje > 0.0f) {
                timerMensaje -= dt;
                if (timerMensaje <= 0.0f) {
                    timerMensaje = 0.0f;
                    mensajeFlotante.clear();
                }
            }

            // ESC con el inventario abierto lo cierra primero (mismo criterio
            // "cerrar lo de encima antes" que un juego tipico) — recien con
            // el inventario ya cerrado, ESC abre la pausa. Arranca siempre
            // en "Continuar" (indice 0) para que un ESC sin querer, seguido
            // de un ENTER sin querer, no dispare "Guardar" ni "Salir".
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (inventarioAbierto) {
                    inventarioAbierto = false;
                } else {
                    opcionPausaSeleccionada = 0;
                    estadoPrevioAPausa = EstadoJuego::Exploracion;
                    estado = EstadoJuego::Pausa;
                }
            }

            if (IsKeyPressed(KEY_I)) inventarioAbierto = !inventarioAbierto;

            // F5 abre la seleccion de slot en cualquier momento de la
            // exploracion (con el inventario abierto o no) — nunca durante
            // combate, ni siquiera apretando la tecla por error, porque
            // este bloque es EstadoJuego::Exploracion nomas. Antes guardaba
            // directo a un unico archivo; ahora hay que elegir en cual de
            // los kNumSlots guardar (ver EstadoJuego::SeleccionSlot).
            if (IsKeyPressed(KEY_F5)) {
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

                if (IsKeyPressed(KEY_TAB)) panelExpandido = !panelExpandido;

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
                        mensajeFlotante = "Encontraste: " + cofreCercano->contenido.nombre;
                        timerMensaje = kDuracionMensaje;
                    }
                }
            }

            if (inventarioAbierto) {
                BeginDrawing();
                renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres);
                ui::DibujarInventario(party, objetivoInventario, renderer.Sprites());
                EndDrawing();
            } else {
                renderer.DibujarFrame(mazmorra, party, enemigos, cofres, panelExpandido, prompt, mensajeFlotante);
            }
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
                        // Abandona la mazmorra en curso sin marcarla como
                        // superada — el party sigue tal cual esta (HP,
                        // inventario, equipo: "sigue con el desgaste" entre
                        // mazmorras). Si la pausa se abrio desde el propio
                        // Mapa (estadoPrevioAPausa ya es Mapa) esto es
                        // exactamente lo mismo que "Continuar".
                        mazmorraActivaIndice = -1;
                        opcionMapaSeleccionada = 0;
                        estado = EstadoJuego::Mapa;
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
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres);
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
                        // enMapa se deduce de mazmorraActivaIndice: solo es
                        // -1 cuando no hay ninguna mazmorra en curso (parado
                        // en el mapa, o en su pausa) — ver como se mantiene
                        // en EstadoJuego::Mapa/Pausa/VolverAlMapa mas arriba.
                        bool enMapa = (mazmorraActivaIndice < 0);
                        bool guardado = game::GuardarPartida(slot, mazmorra, party, enemigos, cofres,
                                                              mazmorraSuperada, enMapa, mazmorraActivaIndice);
                        mensajeSlot = guardado
                            ? ("Guardado en Slot " + std::to_string(slot + 1) + ".")
                            : ("No se pudo guardar en Slot " + std::to_string(slot + 1) + ".");
                        hayGuardado[slot] = hayGuardado[slot] || guardado;
                    } else if (hayGuardado[slot]) {
                        game::ResultadoCarga carga = game::CargarPartida(slot);
                        if (carga.valido) {
                            // Mismo reemplazo de estado que antes hacia
                            // "Cargar" directo desde el menu de inicio (ver
                            // ui::OpcionMenuInicio::Cargar mas arriba) — pero
                            // ahora el estado destino depende de si se habia
                            // guardado parado en el mapa o a mitad de una
                            // mazmorra (ver DatosPartida::enMapa en save.h).
                            mazmorra = game::Dungeon(std::move(carga.datos.habitaciones), std::move(carga.datos.paredes),
                                                      std::move(carga.datos.trampas));
                            posicionInicial = mazmorra.CentroDeSala(0);
                            party = game::Party(std::move(carga.datos.miembros));
                            for (auto& pila : carga.datos.pilasInventario) {
                                party.Inventario().Agregar(std::move(pila.item), pila.cantidad);
                            }
                            enemigos = std::move(carga.datos.enemigos);
                            cofres = std::move(carga.datos.cofres);
                            for (int i = 0; i < game::kNumMazmorrasMapa; ++i) {
                                mazmorraSuperada[i] = carga.datos.mazmorraSuperada[i];
                            }
                            mazmorraActivaIndice = carga.datos.mazmorraActivaIndice;
                            opcionMapaSeleccionada = 0;
                            party.ReiniciarFormacion(party.Lider().Posicion());
                            mensajeSlot.clear();
                            estado = carga.datos.enMapa ? EstadoJuego::Mapa : EstadoJuego::Exploracion;
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
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres);
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
                        ? static_cast<Dificultad>(mazmorraActivaIndice) : Dificultad::Media;
                    std::string botin;
                    for (game::Enemy* e : encuentro->Enemigos()) {
                        game::ResultadoLoot loot = LootDeEnemigoPorDificultad(e->Tipo(), dificultadActual);
                        if (loot.hay) {
                            party.Inventario().Agregar(loot.item);
                            if (!botin.empty()) botin += ", ";
                            botin += loot.item.nombre;
                        }
                    }
                    mensajeFlotante = botin.empty() ? "No encontraste botin esta vez." : ("Botin: " + botin);
                    timerMensaje = kDuracionMensaje;
                    lootRepartido = true;
                    audio.ReproducirVictoria();
                }
                if (GetKeyPressed() != 0) {
                    // Ganarle al Capitan Bandido (el jefe, siempre solo en su
                    // sala) marca la mazmorra activa como Superada y vuelve
                    // al mapa en vez de a la exploracion — ya no queda nada
                    // mas que hacer en esta mazmorra (aunque sigue siendo
                    // rejugable desde el mapa, con un layout nuevo).
                    bool esVictoriaFinal = false;
                    for (game::Enemy* e : encuentro->Enemigos()) {
                        if (e->Tipo() == game::TipoEnemigo::CapitanBandido) { esVictoriaFinal = true; break; }
                    }
                    if (esVictoriaFinal && mazmorraActivaIndice >= 0) {
                        mazmorraSuperada[mazmorraActivaIndice] = true;
                        mazmorraActivaIndice = -1;
                        opcionMapaSeleccionada = 0;
                        estado = EstadoJuego::Mapa;
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
                    // sus stats de partida (pierde todos los items/equipo
                    // ganados en la run) y el mapa entero pierde su progreso
                    // (ninguna mazmorra queda Superada); se vuelve al mapa
                    // para arrancar de cero, no a la mazmorra donde se perdio.
                    party = CrearPartyDeEjemplo(game::Vec2{0.0f, 0.0f});
                    for (int i = 0; i < game::kNumMazmorrasMapa; ++i) mazmorraSuperada[i] = false;
                    mazmorraActivaIndice = -1;
                    opcionMapaSeleccionada = 0;
                    party.ReiniciarFormacion(posicionInicial);
                    estado = EstadoJuego::Mapa;
                    encuentro.reset();
                }
            }

            BeginDrawing();
            ClearBackground(BLACK);
            // Se dibuja la mazmorra "congelada" de fondo para dar contexto, y
            // encima la pantalla de combate (que ya trae su propio overlay oscuro).
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres);
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
