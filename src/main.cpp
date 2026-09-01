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
#include "render/renderer.h"
#include "render/input.h"
#include "render/combat_ui.h"

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
        default:
            return game::Enemy(std::string("Bandido Aturdidor") + sufijo, tipo,
                game::Stats{ /*hpMax*/26, /*hp*/26, /*recursoMax*/0, /*recurso*/0, /*ataque*/8, /*defensa*/4, /*velocidad*/85.0f },
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

enum class EstadoJuego { Exploracion, Combate };

// Distancia (en pixeles) a la que hay que estar del enemigo mas cercano
// para poder engancharlo (a el y a todo el resto de su sala) en combate
// con [E].
constexpr float kDistanciaInteraccion = 90.0f;

} // namespace

int main() {
    const int anchoVentana = 1280;
    const int altoVentana = 720;

    // Mazmorra procedural: una cadena de salas conectadas por pasillos (ver
    // game/dungeon.cpp). La sala 0 es donde arranca el party, sin enemigos;
    // cada sala siguiente tiene un grupo de 1 a 3 enemigos de tipo
    // aleatorio, que se enganchan todos juntos en un mismo combate.
    game::Dungeon mazmorra;
    game::Vec2 posicionInicial = mazmorra.CentroDeSala(0);
    game::Party party = CrearPartyDeEjemplo(posicionInicial);

    std::vector<game::Enemy> enemigos;
    const auto& salas = mazmorra.Habitaciones();
    for (size_t i = 1; i < salas.size(); ++i) {
        std::vector<game::Enemy> grupo = CrearGrupoDeSala(mazmorra.CentroDeSala(i), static_cast<int>(i));
        for (auto& e : grupo) enemigos.push_back(std::move(e));
    }

    render::Renderer renderer(anchoVentana, altoVentana, "RPG Mazmorras - Prototipo");

    EstadoJuego estado = EstadoJuego::Exploracion;
    std::unique_ptr<game::CombatEncounter> encuentro;
    bool panelExpandido = false;  // arranca compacto; TAB lo expande/oculta

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        // Clamp defensivo: si el frame tarda mucho (ventana minimizada, breakpoint,
        // etc.), un dt gigante podria mover al personaje lo suficiente como para
        // atravesar una pared fina en un solo salto. Con esto el movimiento maximo
        // por frame queda acotado.
        if (dt > 1.0f / 30.0f) dt = 1.0f / 30.0f;

        if (estado == EstadoJuego::Exploracion) {
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

            // Enganchar combate: el enemigo vivo mas cercano, si esta a
            // distancia de interaccion y se aprieta E. Al engancharlo se
            // suma al combate TODO el resto de su sala (todos los enemigos
            // vivos con la misma Sala()), no solo a el — un combate por
            // sala, no por enemigo individual.
            game::Enemy* enemigoCercano = nullptr;
            float distanciaCercana = kDistanciaInteraccion;
            for (auto& e : enemigos) {
                if (e.Vencido()) continue;
                float distancia = game::Length(lider.Posicion() - e.Posicion());
                if (distancia < distanciaCercana) {
                    distanciaCercana = distancia;
                    enemigoCercano = &e;
                }
            }
            if (enemigoCercano != nullptr && IsKeyPressed(KEY_E)) {
                std::vector<game::Enemy*> grupo;
                for (auto& e : enemigos) {
                    if (!e.Vencido() && e.Sala() == enemigoCercano->Sala()) grupo.push_back(&e);
                }
                encuentro = std::make_unique<game::CombatEncounter>(party, std::move(grupo));
                estado = EstadoJuego::Combate;
            }

            renderer.DibujarFrame(mazmorra, party, enemigos, panelExpandido);
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
            renderer.DibujarEscenarioSinUI(mazmorra, party, enemigos);
            if (encuentro) {
                ui::DibujarCombate(*encuentro, anchoVentana, altoVentana);
            }
            EndDrawing();
        }
    }

    return 0;
}
