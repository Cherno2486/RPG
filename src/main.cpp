#include "raylib.h"
#include <vector>
#include <utility>
#include <memory>

#include "game/character.h"
#include "game/party.h"
#include "game/dungeon.h"
#include "game/enemy.h"
#include "game/combat.h"
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

enum class EstadoJuego { Exploracion, Combate };

// Distancia (en pixeles) a la que hay que estar del enemigo para poder
// engancharlo en combate con [E].
constexpr float kDistanciaInteraccion = 90.0f;

} // namespace

int main() {
    const int anchoVentana = 1280;
    const int altoVentana = 720;

    game::Dungeon mazmorra(/*anchoTiles*/20, /*altoTiles*/12);
    game::Vec2 posicionInicial{ anchoVentana / 2.0f, altoVentana / 2.0f };
    game::Party party = CrearPartyDeEjemplo(posicionInicial);

    // Tres enemigos de tipos distintos repartidos por la sala de prueba,
    // lejos del panel de party (que vive arriba a la izquierda) y de las
    // paredes, para que se vean siempre. Cada uno se engancha por separado
    // (todavia no hay combates de varios enemigos a la vez, eso queda para
    // cuando haya generacion real de encuentros).
    std::vector<game::Enemy> enemigos;
    enemigos.emplace_back("Esqueleto Errante", game::TipoEnemigo::EsqueletoErrante,
        game::Stats{ /*hpMax*/22, /*hp*/22, /*recursoMax*/0, /*recurso*/0, /*ataque*/7, /*defensa*/3, /*velocidad*/80.0f },
        game::Vec2{ 760.0f, 420.0f });
    enemigos.emplace_back("Rata Gigante", game::TipoEnemigo::RataGigante,
        game::Stats{ /*hpMax*/12, /*hp*/12, /*recursoMax*/0, /*recurso*/0, /*ataque*/5, /*defensa*/1, /*velocidad*/130.0f },
        game::Vec2{ 300.0f, 450.0f });
    enemigos.emplace_back("Bandido Aturdidor", game::TipoEnemigo::BanditoAturdidor,
        game::Stats{ /*hpMax*/26, /*hp*/26, /*recursoMax*/0, /*recurso*/0, /*ataque*/8, /*defensa*/4, /*velocidad*/85.0f },
        game::Vec2{ 860.0f, 180.0f });

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
            } else if (encuentro->Fase() == game::FaseCombate::Ganado ||
                       encuentro->Fase() == game::FaseCombate::Perdido) {
                if (GetKeyPressed() != 0) {
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
