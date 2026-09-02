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
#include "render/renderer.h"
#include "render/input.h"
#include "render/combat_ui.h"
#include "render/inventory_ui.h"

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

enum class EstadoJuego { Exploracion, Combate };

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

    render::Renderer renderer(anchoVentana, altoVentana, "RPG Mazmorras - Prototipo");

    EstadoJuego estado = EstadoJuego::Exploracion;
    std::unique_ptr<game::CombatEncounter> encuentro;
    bool panelExpandido = false;     // arranca compacto; TAB lo expande/oculta
    bool inventarioAbierto = false;  // [I] lo abre/cierra durante exploracion
    bool lootRepartido = false;      // evita repartir el botin mas de una vez por combate
    size_t objetivoInventario = 0;   // a quien se le aplica el proximo item usado
    std::string mensajeFlotante;     // botin de cofre/combate, visible unos segundos
    float timerMensaje = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        // Clamp defensivo: si el frame tarda mucho (ventana minimizada, breakpoint,
        // etc.), un dt gigante podria mover al personaje lo suficiente como para
        // atravesar una pared fina en un solo salto. Con esto el movimiento maximo
        // por frame queda acotado.
        if (dt > 1.0f / 30.0f) dt = 1.0f / 30.0f;

        if (estado == EstadoJuego::Exploracion) {
            if (timerMensaje > 0.0f) {
                timerMensaje -= dt;
                if (timerMensaje <= 0.0f) {
                    timerMensaje = 0.0f;
                    mensajeFlotante.clear();
                }
            }

            if (IsKeyPressed(KEY_I)) inventarioAbierto = !inventarioAbierto;

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
                        estado = EstadoJuego::Combate;
                        lootRepartido = false;
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
                ui::DibujarInventario(party, objetivoInventario);
                EndDrawing();
            } else {
                renderer.DibujarFrame(mazmorra, party, enemigos, cofres, panelExpandido, prompt, mensajeFlotante);
            }
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
                }
                if (GetKeyPressed() != 0) {
                    estado = EstadoJuego::Exploracion;
                    encuentro.reset();
                }
            } else if (encuentro->Fase() == game::FaseCombate::Perdido) {
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
                ui::DibujarCombate(*encuentro, anchoVentana, altoVentana);
            }
            EndDrawing();
        }
    }

    return 0;
}
