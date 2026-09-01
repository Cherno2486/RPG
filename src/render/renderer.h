#pragma once
#include "../game/dungeon.h"
#include "../game/party.h"

namespace render {

class Renderer {
public:
    Renderer(int anchoVentana, int altoVentana, const char* titulo);
    ~Renderer();

    void DibujarFrame(const game::Dungeon& mazmorra, const game::Party& party);

private:
    int anchoVentana_;
    int altoVentana_;
};

} // namespace render
