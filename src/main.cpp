#include "raylib.h"
#include <vector>
#include <utility>

#include "game/character.h"
#include "game/party.h"
#include "game/dungeon.h"
#include "render/renderer.h"
#include "render/input.h"

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

    return game::Party(std::move(miembros));
}

} // namespace

int main() {
    const int anchoVentana = 1280;
    const int altoVentana = 720;

    game::Dungeon mazmorra(/*anchoTiles*/20, /*altoTiles*/12);
    game::Vec2 posicionInicial{ anchoVentana / 2.0f, altoVentana / 2.0f };
    game::Party party = CrearPartyDeEjemplo(posicionInicial);

    render::Renderer renderer(anchoVentana, altoVentana, "RPG Mazmorras - Prototipo");

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        // Clamp defensivo: si el frame tarda mucho (ventana minimizada, breakpoint,
        // etc.), un dt gigante podria mover al personaje lo suficiente como para
        // atravesar una pared fina en un solo salto. Con esto el movimiento maximo
        // por frame queda acotado.
        if (dt > 1.0f / 30.0f) dt = 1.0f / 30.0f;

        game::Vec2 direccion = input::LeerDireccionMovimiento();
        game::Character& lider = party.Lider();

        float velocidadPxPorSeg = lider.GetStats().velocidad;
        game::Vec2 posicionActual = lider.Posicion();
        game::Vec2 posicionDeseada = posicionActual + direccion * (velocidadPxPorSeg * dt);

        game::Vec2 posicionResuelta = mazmorra.ResolverColision(
            lider.Colisionador(), posicionActual, posicionDeseada);
        lider.SetPosicion(posicionResuelta);

        party.ActualizarFormacion(dt);

        renderer.DibujarFrame(mazmorra, party);
    }

    return 0;
}
