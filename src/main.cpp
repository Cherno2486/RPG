#include "raylib.h"
#include <vector>
#include <utility>
#include <memory>

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

// Elige uno de los tres tipos de enemigo al azar, con sus stats de siempre,
// para poblar una sala de la mazmorra generada. Cada partida termina con
// una mezcla distinta de encuentros.
game::Enemy CrearEnemigoAleatorio(game::Vec2 posicion) {
    switch (game::Roll(3)) {
        case 1:
            return game::Enemy("Esqueleto Errante", game::TipoEnemigo::EsqueletoErrante,
                game::Stats{ /*hpMax*/22, /*hp*/22, /*recursoMax*/0, /*recurso*/0, /*ataque*/7, /*defensa*/3, /*velocidad*/80.0f },
                posicion);
        case 2:
            return game::Enemy("Rata Gigante", game::TipoEnemigo::RataGigante,
                game::Stats{ /*hpMax*/12, /*hp*/12, /*recursoMax*/0, /*recurso*/0, /*ataque*/5, /*defensa*/1, /*velocidad*/130.0f },
                posicion);
        default:
            return game::Enemy("Bandido Aturdidor", game::TipoEnemigo::BanditoAturdidor,
                game::Stats{ /*hpMax*/26, /*hp*/26, /*recursoMax*/0, /*recurso*/0, /*ataque*/8, /*defensa*/4, /*velocidad*/85.0f },
                posicion);
    }
}

enum class EstadoJuego { Exploracion, Combate };

// Distancia (en pixeles) a la que hay que estar del enemigo para poder
// engancharlo en combate con [E].
constexpr float kDistanciaInteraccion = 90.0f;

} // namespace

int main() {
    const int anchoVentana = 1280;
    const int altoVentana = 720;

    // Mazmorra procedural: una cadena de salas conectadas por pasillos (ver
    // game/dungeon.cpp). La sala 0 es donde arranca el party, sin enemigo;
    // cada sala siguiente tiene un enemigo de tipo aleatorio.
    game::Dungeon mazmorra;
    game::Vec2 posicionInicial = mazmorra.CentroDeSala(0);
    game::Party party = CrearPartyDeEjemplo(posicionInicial);

    std::vector<game::Enemy> enemigos;
    const auto& salas = mazmorra.Habitaciones();
    for (size_t i = 1; i < salas.size(); ++i) {
        enemigos.push_back(CrearEnemigoAleatorio(mazmorra.CentroDeSala(i)));
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
            // distancia de interaccion y se aprieta E.
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
                encuentro = std::make_unique<game::CombatEncounter>(party, *enemigoCercano);
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
