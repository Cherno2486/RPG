#pragma once
#include "../game/dungeon.h"
#include "../game/party.h"
#include "../game/enemy.h"

namespace render {

class Renderer {
public:
    Renderer(int anchoVentana, int altoVentana, const char* titulo);
    ~Renderer();

    // 'enemigo' puede ser nullptr (o estar vencido) si todavia/ya no hay
    // nada que mostrar en la mazmorra. 'panelExpandido' controla si el panel
    // de party se ve completo o en su version compacta (se alterna con TAB
    // desde main.cpp) para no taparle el mapa al jugador todo el tiempo.
    void DibujarFrame(const game::Dungeon& mazmorra, const game::Party& party, const game::Enemy* enemigo, bool panelExpandido);

    // Dibuja solo la mazmorra + party + enemigo (sin panel de UI ni el cartel
    // de "[E] Atacar"), y sin BeginDrawing/EndDrawing propios. La usa main.cpp
    // para pintar la mazmorra "de fondo" cuando se esta en la pantalla de
    // combate (que ya trae su propio overlay encima).
    void DibujarEscenarioSinUI(const game::Dungeon& mazmorra, const game::Party& party, const game::Enemy* enemigo);

private:
    int anchoVentana_;
    int altoVentana_;
};

} // namespace render
