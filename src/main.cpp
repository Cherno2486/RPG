#include "raylib.h"
#include <vector>
#include <utility>
#include <memory>
#include <string>

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

game::Party CrearPartyDeEjemplo(game::Vec2 posicionInicial) {
    using game::Character;
    using game::Role;
    using game::Stats;

    std::vector<Character> miembros;
    miembros.emplace_back("Bruna", Role::Tanque,
        Stats{ /*hpMax*/30, /*hp*/30, /*recursoMax*/0,  /*recurso*/0,  /*ataque*/4, /*defensa*/6, /*velocidad*/90.0f },
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
// se puedan distinguir en el log y en las fichas de combate.
game::Enemy CrearEnemigoDeTipo(game::TipoEnemigo tipo, game::Vec2 posicion, int salaIndice, int ocurrencia) {
    const char* sufijo = (ocurrencia == 2) ? " II" : (ocurrencia == 3) ? " III" : "";

    switch (tipo) {
        case game::TipoEnemigo::EsqueletoErrante:
            return game::Enemy(std::string("Esqueleto Errante") + sufijo, tipo,
                game::Stats{ /*hpMax*/22, /*hp*/22, /*recursoMax*/0, /*recurso*/0, /*ataque*/7, /*defensa*/3, /*velocidad*/80.0f },
                posicion, salaIndice);
        case game::TipoEnemigo::RataGigante:
            return game::Enemy(std::string("Rata Gigante") + sufijo, tipo,
                game::Stats{ /*hpMax*/12, /*hp*/12, /*recursoMax*/0, /*recurso*/0, /*ataque*/5, /*defensa*/1, /*velocidad*/130.0f },
                posicion, salaIndice);
        case game::TipoEnemigo::BanditoAturdidor:
            return game::Enemy(std::string("Bandido Aturdidor") + sufijo, tipo,
                game::Stats{ /*hpMax*/24, /*hp*/24, /*recursoMax*/0, /*recurso*/0, /*ataque*/7, /*defensa*/4, /*velocidad*/85.0f },
                posicion, salaIndice);
        default:
            return game::Enemy("Capitan Bandido", tipo,
                game::Stats{ /*hpMax*/52, /*hp*/52, /*recursoMax*/0, /*recurso*/0, /*ataque*/8, /*defensa*/5, /*velocidad*/85.0f },
                posicion, salaIndice);
    }
}

// Arma el grupo de enemigos de una sala con contenido: entre 1 y 3, de
// tipos elegidos al azar (pueden repetirse), separados un poco entre si
// para que no queden todos superpuestos en el mismo punto. El margen mas
// chico entre salas (la alargada, 6 tiles = 288px de ancho) deja de sobra
// para hasta 3 enemigos separados 70px del centro sin acercarse a la pared.
std::vector<game::Enemy> CrearGrupoDeSala(game::Vec2 centro, int salaIndice) {
    int cantidad = game::Roll(3);  // 1, 2 o 3 enemigos en esta sala
    std::vector<game::Enemy> grupo;
    int vistos[3] = {0, 0, 0};  // contador por TipoEnemigo, para el sufijo

    for (int j = 0; j < cantidad; ++j) {
        game::TipoEnemigo tipo = TipoAleatorio();
        int& ocurrencias = vistos[static_cast<int>(tipo)];
        ocurrencias += 1;

        float dx = (cantidad > 1) ? (j - (cantidad - 1) / 2.0f) * 70.0f : 0.0f;
        float dy = (cantidad > 1 && j % 2 == 1) ? 18.0f : (cantidad > 2 && j == 2 ? -18.0f : 0.0f);
        game::Vec2 posicion = centro + game::Vec2{dx, dy};

        grupo.push_back(CrearEnemigoDeTipo(tipo, posicion, salaIndice, ocurrencias));
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

// Todo lo que hace falta para arrancar una partida nueva desde cero: una
// mazmorra procedural distinta cada vez, el party de ejemplo parado en la
// sala 0, y los enemigos/cofres repartidos por el resto de las salas. Antes
// esto vivia inline al principio de main() y se corria una sola vez (el
// menu de inicio solo se veia al arrancar el ejecutable); ahora que se
// puede volver a MenuInicio desde la pausa (ver EstadoJuego::Pausa) y elegir
// "Nueva partida" de nuevo, hace falta poder repetirlo en cualquier momento
// sin reiniciar el programa — de ahi que estar en una funcion aparte en vez
// de en el cuerpo de main().
struct PartidaNueva {
    game::Dungeon mazmorra;
    game::Vec2 posicionInicial;
    game::Party party;
    std::vector<game::Enemy> enemigos;
    std::vector<game::Cofre> cofres;
};

PartidaNueva GenerarPartidaNueva() {
    // Mazmorra procedural: una cadena de salas conectadas por pasillos (ver
    // game/dungeon.cpp). La sala 0 es donde arranca el party, sin enemigos;
    // las salas intermedias tienen un grupo de 1 a 3 enemigos de tipo
    // aleatorio, que se enganchan todos juntos en un mismo combate; la
    // ultima sala, en cambio, tiene un unico Capitan Bandido — el jefe de
    // la mazmorra, que le da un cierre a la run (ver "Balance" en
    // docs/design.md y CombatEncounter::Actualizar para su IA especial).
    game::Dungeon mazmorra;
    game::Vec2 posicionInicial = mazmorra.CentroDeSala(0);
    game::Party party = CrearPartyDeEjemplo(posicionInicial);

    std::vector<game::Enemy> enemigos;
    const auto& salas = mazmorra.Habitaciones();
    size_t indiceSalaJefe = salas.size() - 1;
    for (size_t i = 1; i < salas.size(); ++i) {
        if (i == indiceSalaJefe) {
            enemigos.push_back(CrearEnemigoDeTipo(game::TipoEnemigo::CapitanBandido,
                mazmorra.CentroDeSala(i), static_cast<int>(i), 1));
            continue;
        }
        std::vector<game::Enemy> grupo = CrearGrupoDeSala(mazmorra.CentroDeSala(i), static_cast<int>(i));
        for (auto& e : grupo) enemigos.push_back(std::move(e));
    }

    // Cofres: uno garantizado en la sala inicial (para que el sistema se vea
    // sin depender del azar) y, ademas, una chance por cada sala con
    // contenido de tener uno extra aparte de su grupo de enemigos.
    std::vector<game::Cofre> cofres;
    cofres.push_back(CrearCofreEnEsquina(salas[0], game::ItemAleatorioDeCofre()));
    for (size_t i = 1; i < salas.size(); ++i) {
        if (game::Roll(10) <= kChanceCofrePorSalaDe10) {
            cofres.push_back(CrearCofreEnEsquina(salas[i], game::ItemAleatorioDeCofre()));
        }
    }

    return PartidaNueva{ std::move(mazmorra), posicionInicial, std::move(party), std::move(enemigos), std::move(cofres) };
}

// MenuInicio es el estado inicial: pantalla de titulo con las 4 opciones de
// ui::OpcionMenuInicio (ver render/menu_ui.h) antes de largar a explorar.
// SobreMi es la pantalla placeholder de esa opcion (ver ui::DibujarSobreMi) —
// un estado propio, no un sub-estado de MenuInicio, para que se dibuje y se
// lea el input igual que cualquier otra pantalla de la maquina de estados.
// Pausa es la pantalla que abre ESC durante la exploracion (ver
// ui::DibujarPausa) — desde ahi se puede volver a jugar, guardar, volver a
// MenuInicio sin cerrar el juego, o salir. No existe durante Combate (ESC
// no hace nada ahi, igual que F5 tampoco guarda en combate).
enum class EstadoJuego { MenuInicio, SobreMi, Exploracion, Pausa, Combate };

// Distancia (en pixeles) a la que hay que estar del interactuable mas
// cercano (enemigo o cofre) para poder engancharlo/abrirlo con [E].
constexpr float kDistanciaInteraccion = 90.0f;

// Cuanto tiempo (segundos) queda en pantalla un mensaje flotante (botin de
// un cofre o de un combate ganado) antes de desaparecer solo.
constexpr float kDuracionMensaje = 3.0f;

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

    // Arranca con una partida generada de cero — sirve como fondo "congelado"
    // del menu de inicio hasta que el jugador elija que hacer. Si mas tarde
    // vuelve a elegir "Nueva partida" (desde el menu de inicio la primera
    // vez, o volviendo por la pausa despues), se llama de nuevo a
    // GenerarPartidaNueva() para reemplazar esto por una mazmorra distinta —
    // ver el case NuevaPartida mas abajo.
    PartidaNueva partidaInicial = GenerarPartidaNueva();
    game::Dungeon mazmorra = std::move(partidaInicial.mazmorra);
    game::Vec2 posicionInicial = partidaInicial.posicionInicial;
    game::Party party = std::move(partidaInicial.party);
    std::vector<game::Enemy> enemigos = std::move(partidaInicial.enemigos);
    std::vector<game::Cofre> cofres = std::move(partidaInicial.cofres);

    render::Renderer renderer(anchoVentana, altoVentana, "RPG Mazmorras - Prototipo");
    render::Audio audio;

    // Se chequea una vez al arrancar el programa; despues se mantiene al
    // dia a mano (nunca se vuelve a llamar a HayPartidaGuardada) cada vez
    // que F5 o "Guardar" en la pausa guardan con exito, sumando el
    // resultado con OR — ver los dos lugares que hacen
    // "hayGuardado = hayGuardado || guardado;" mas abajo. Controla si
    // "Cargar" se dibuja habilitada en el menu de inicio (ver
    // render/menu_ui.h), que ahora se puede volver a visitar en cualquier
    // momento a traves de la pausa (Pausa -> Menu principal), asi que no
    // alcanza con chequearlo una sola vez al principio como antes.
    bool hayGuardado = game::HayPartidaGuardada();

    EstadoJuego estado = EstadoJuego::MenuInicio;
    std::unique_ptr<game::CombatEncounter> encuentro;
    bool panelExpandido = false;     // arranca compacto; TAB lo expande/oculta
    bool inventarioAbierto = false;  // [I] lo abre/cierra durante exploracion
    bool lootRepartido = false;      // evita repartir el botin mas de una vez por combate
    bool derrotaSonada = false;      // evita repetir el sonido de derrota mientras se ve el Game Over
    size_t objetivoInventario = 0;   // a quien se le aplica el proximo item usado
    std::string mensajeFlotante;     // botin de cofre/combate, visible unos segundos
    float timerMensaje = 0.0f;
    int opcionMenuSeleccionada = 0;  // indice sobre ui::OpcionMenuInicio (ver render/menu_ui.h)
    int opcionPausaSeleccionada = 0; // indice sobre ui::OpcionPausa (ver render/menu_ui.h)
    bool salirDelJuego = false;      // "Salir" (del menu de inicio o de la pausa) lo pone en true

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
                        // Se regenera siempre, incluso la primerísima vez
                        // (que ya tenia una mazmorra "de fondo" generada
                        // antes del loop) — asi el codigo no tiene que
                        // distinguir entre esa mazmorra inicial y una
                        // vuelta al menu despues de haber jugado (via
                        // Pausa -> Menu principal, ver EstadoJuego::Pausa):
                        // "Nueva partida" siempre da una mazmorra nueva,
                        // nunca retoma la que estaba de fondo en el menu.
                        PartidaNueva partida = GenerarPartidaNueva();
                        mazmorra = std::move(partida.mazmorra);
                        posicionInicial = partida.posicionInicial;
                        party = std::move(partida.party);
                        enemigos = std::move(partida.enemigos);
                        cofres = std::move(partida.cofres);
                        estado = EstadoJuego::Exploracion;
                        break;
                    }
                    case ui::OpcionMenuInicio::Cargar: {
                        // Deshabilitada (ver ui::DibujarMenuInicio) mientras
                        // no haya guardado — confirmarla en ese caso no hace
                        // nada, el jugador se queda en el menu.
                        if (!hayGuardado) break;
                        game::ResultadoCarga carga = game::CargarPartida();
                        if (carga.valido) {
                            // Reemplaza TODO el estado pre-generado de arriba
                            // (mazmorra/party/enemigos/cofres) por el
                            // guardado — lo generado antes del loop solo era
                            // para tener algo de fondo en este mismo menu.
                            mazmorra = game::Dungeon(std::move(carga.datos.habitaciones), std::move(carga.datos.paredes));
                            posicionInicial = mazmorra.CentroDeSala(0);
                            party = game::Party(std::move(carga.datos.miembros));
                            for (auto& pila : carga.datos.pilasInventario) {
                                party.Inventario().Agregar(std::move(pila.item), pila.cantidad);
                            }
                            enemigos = std::move(carga.datos.enemigos);
                            cofres = std::move(carga.datos.cofres);
                            // Reubica a todo el party en la posicion guardada
                            // del lider y borra el rastro de formacion — sin
                            // esto, los seguidores "correrian" desde un
                            // rastro vacio en vez de aparecer ya en fila
                            // detras del lider (mismo recurso que ya usa el
                            // revivir tras una derrota).
                            party.ReiniciarFormacion(party.Lider().Posicion());
                            estado = EstadoJuego::Exploracion;
                        }
                        // Si el archivo estaba corrupto/incompleto (a pesar
                        // de que HayPartidaGuardada() dio true), se ignora en
                        // silencio y el jugador se queda en el menu — puede
                        // elegir "Nueva partida" en vez de trabarse.
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
            ui::DibujarMenuInicio(anchoVentana, altoVentana, opcionMenuSeleccionada, hayGuardado);
            EndDrawing();
        } else if (estado == EstadoJuego::SobreMi) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                estado = EstadoJuego::MenuInicio;
            }

            BeginDrawing();
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres);
            ui::DibujarSobreMi(anchoVentana, altoVentana);
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
                    estado = EstadoJuego::Pausa;
                }
            }

            if (IsKeyPressed(KEY_I)) inventarioAbierto = !inventarioAbierto;

            // F5 guarda en cualquier momento de la exploracion (con el
            // inventario abierto o no) — nunca durante combate, ni siquiera
            // apretando la tecla por error, porque este bloque es
            // EstadoJuego::Exploracion nomas. El archivo de guardado
            // persiste todo lo necesario para reconstruir la partida
            // (mazmorra ya resuelta, party, inventario, enemigos, cofres) —
            // ver game/save.h.
            if (IsKeyPressed(KEY_F5)) {
                bool guardado = game::GuardarPartida(mazmorra, party, enemigos, cofres);
                mensajeFlotante = guardado ? "Partida guardada." : "No se pudo guardar la partida.";
                timerMensaje = kDuracionMensaje;
                hayGuardado = hayGuardado || guardado;
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
                        if (pilas[indice].item.tipo == game::TipoItem::Consumible) {
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
                game::Vec2 direccion = input::LeerDireccionMovimiento();
                game::Character& lider = party.Lider();

                float velocidadPxPorSeg = lider.GetStats().velocidad;
                game::Vec2 posicionActual = lider.Posicion();
                game::Vec2 posicionDeseada = posicionActual + direccion * (velocidadPxPorSeg * dt);

                game::Vec2 posicionResuelta = mazmorra.ResolverColision(
                    lider.Colisionador(), posicionActual, posicionDeseada);
                lider.SetPosicion(posicionResuelta);

                party.ActualizarFormacion(dt);

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
                        // Se suma al encuentro TODO el resto de la sala
                        // (todos los enemigos vivos con la misma Sala()), no
                        // solo a el — un combate por sala, no por enemigo
                        // individual.
                        std::vector<game::Enemy*> grupo;
                        for (auto& e : enemigos) {
                            if (!e.Vencido() && e.Sala() == enemigoCercano->Sala()) grupo.push_back(&e);
                        }
                        encuentro = std::make_unique<game::CombatEncounter>(party, std::move(grupo));
                        ui::ReiniciarFeedbackVisual();
                        audio.ReiniciarCombate();
                        estado = EstadoJuego::Combate;
                        lootRepartido = false;
                        derrotaSonada = false;
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
            // Mismo timer que ya usaba F5 en Exploracion (reutilizado tal
            // cual, no uno aparte) — asi "Guardar" desde la pausa muestra el
            // mismo cartel "Partida guardada." con la misma duracion, y si
            // el jugador elige Continuar con el cartel todavia visible, va a
            // seguir viendolo un rato en la exploracion (comportamiento ya
            // aceptado: mensajeFlotante nunca se ata a una pantalla en
            // particular, solo al tiempo transcurrido).
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
                // a mano cada vez.
                estado = EstadoJuego::Exploracion;
            } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                switch (static_cast<ui::OpcionPausa>(opcionPausaSeleccionada)) {
                    case ui::OpcionPausa::Continuar:
                        estado = EstadoJuego::Exploracion;
                        break;
                    case ui::OpcionPausa::Guardar: {
                        // Misma funcion que ya usaba F5 — no hay dos caminos
                        // distintos para guardar, solo dos formas de
                        // dispararla.
                        bool guardado = game::GuardarPartida(mazmorra, party, enemigos, cofres);
                        mensajeFlotante = guardado ? "Partida guardada." : "No se pudo guardar la partida.";
                        timerMensaje = kDuracionMensaje;
                        hayGuardado = hayGuardado || guardado;
                        break;
                    }
                    case ui::OpcionPausa::MenuPrincipal:
                        // No toca mazmorra/party/enemigos/cofres — siguen
                        // ahi tal cual, sirviendo de fondo "congelado" para
                        // el menu de inicio (mismo truco visual de siempre),
                        // por si el jugador vuelve a elegir "Continuar"...
                        // que en MenuInicio no existe: para retomar tiene
                        // que guardar antes y despues elegir "Cargar", o
                        // elegir "Nueva partida" y perder este progreso.
                        // Se limpia el cartel de guardado para no arrastrar
                        // un "Partida guardada." viejo a una pantalla donde
                        // ya no tiene sentido.
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
        } else {  // EstadoJuego::Combate
            encuentro->Actualizar(dt);

            if (encuentro->Fase() == game::FaseCombate::TurnoAliado) {
                if (IsKeyPressed(KEY_ONE)) {
                    encuentro->AccionAtaqueBasico();
                } else if (IsKeyPressed(KEY_TWO)) {
                    encuentro->AccionHabilidadDeRol();
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
                    std::string botin;
                    for (game::Enemy* e : encuentro->Enemigos()) {
                        game::ResultadoLoot loot = game::TirarLootDeEnemigo(e->Tipo());
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
                    estado = EstadoJuego::Exploracion;
                    encuentro.reset();
                }
            } else if (encuentro->Fase() == game::FaseCombate::Perdido) {
                if (!derrotaSonada) {
                    audio.ReproducirDerrota();
                    derrotaSonada = true;
                }
                if (GetKeyPressed() != 0) {
                    // Game over "de verdad": si solo volvieramos a explorar,
                    // el party quedaria con HP 0 para siempre (el proximo
                    // combate terminaria en derrota instantanea). En vez de
                    // eso, revive a todos a full HP/recurso, les limpia los
                    // efectos, y los manda de vuelta al punto de partida.
                    for (auto& miembro : party.Miembros()) {
                        miembro.Revivir();
                    }
                    party.ReiniciarFormacion(posicionInicial);
                    estado = EstadoJuego::Exploracion;
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
            }
            EndDrawing();
        }
    }

    return 0;
}
