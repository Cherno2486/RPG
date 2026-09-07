#pragma once
#include <memory>
#include <string>
#include <vector>
#include "raylib.h"
#include "../game/dungeon.h"
#include "../game/party.h"
#include "../game/enemy.h"
#include "../game/item.h"
#include "../game/edificio.h"
#include "../game/deambulante.h"
#include "sprites.h"

namespace render {

class Renderer {
public:
    Renderer(int anchoVentana, int altoVentana, const char* titulo);
    ~Renderer();

    // Las texturas pixel-art del juego (personajes, enemigos, cofres, tiles
    // de piso/pared) — combat_ui.cpp e inventory_ui.cpp la necesitan para
    // dibujar sus propios retratos chicos con el mismo sprite que ya se ve
    // en el mapa, en vez de duplicar la generacion de texturas en cada
    // archivo (que ademas gastaria memoria GL por las pilas).
    const SpriteSet& Sprites() const { return *sprites_; }

    // 'enemigos' puede tener cualquier cantidad (incluso ninguno vivo) — se
    // dibujan todos los que no esten Vencido(). 'cofres' se dibujan cerrados
    // o abiertos segun Cofre::abierto. 'tema' elige el juego de texturas de
    // piso/pared y el tinte de la decoracion suelta (ver SpriteSet::TilePiso/
    // TilePared/DecoracionPiso y render::TinteDecoracionPorTema en
    // sprites.h) — 0=Bosque, 1=Carcel, 2=Castillo, mismo orden fijo que
    // game::Tema en main.cpp (este archivo no incluye ese header, asi que
    // solo importa el orden). 'panelExpandido' controla si el panel de
    // party se ve completo o compacto (TAB). 'promptInteraccion', si no
    // esta vacio, se muestra como cartel abajo (p.ej. "[E] Atacar" o "[E]
    // Abrir cofre" — lo decide main.cpp, que ya sabe cual es el interactuable
    // mas cercano). 'mensajeFlotante', si no esta vacio, se muestra arriba
    // del prompt (p.ej. el resultado de abrir un cofre) por un tiempo corto.
    // 'edificios', 'deambulantes' y 'fuentes' son opcionales (vacios por
    // defecto): solo los usa la Ciudad (ver EstadoJuego::Ciudad en main.cpp)
    // -- construcciones interactuables, el perro/pajaros/aldeanos que le dan
    // vida (ver game::Deambulante) y el centro de cada fuente decorativa de
    // la plaza, respectivamente. Las mazmorras normales no pasan nada aca.
    // 'tema' puede ser tambien render::kTemaCiudad (ver sprites.h), en cuyo
    // caso se usa la paleta dedicada de la Ciudad en vez de un tema de
    // mazmorra.
    void DibujarFrame(const game::Dungeon& mazmorra, const game::Party& party,
                       const std::vector<game::Enemy>& enemigos, const std::vector<game::Cofre>& cofres, int tema,
                       bool panelExpandido, const std::string& promptInteraccion, const std::string& mensajeFlotante,
                       const std::vector<game::Edificio>& edificios = {},
                       const std::vector<game::Deambulante>& deambulantes = {},
                       const std::vector<game::Vec2>& fuentes = {});

    // Dibuja solo la mazmorra + party + enemigos + cofres (sin panel de UI ni
    // carteles), y sin BeginDrawing/EndDrawing propios. La usa main.cpp para
    // pintar la mazmorra "de fondo" cuando se esta en la pantalla de combate
    // (que ya trae su propio overlay encima), o en las pantallas de menu/mapa.
    //
    // Centra la camara en el lider del party y dibuja el mundo (grilla,
    // paredes, enemigos, cofres, party) dentro de ese espacio de camara — la
    // mazmorra generada por salas es mas grande que la ventana, asi que sin
    // esto no se veria nada al alejarse de la sala inicial. 'tema': ver el
    // comentario de DibujarFrame arriba. 'edificios'/'deambulantes'/'fuentes':
    // ver el comentario de DibujarFrame arriba (mismos defaults vacios).
    void DibujarEscenarioSinUI(const game::Dungeon& mazmorra, const game::Party& party,
                                const std::vector<game::Enemy>& enemigos, const std::vector<game::Cofre>& cofres,
                                int tema, const std::vector<game::Edificio>& edificios = {},
                                const std::vector<game::Deambulante>& deambulantes = {},
                                const std::vector<game::Vec2>& fuentes = {});

private:
    int anchoVentana_;
    int altoVentana_;
    Camera2D camara_;
    // unique_ptr porque SpriteSet necesita un contexto GL valido (InitWindow
    // ya llamado) para construirse — no puede ser un miembro por valor
    // inicializado antes que el cuerpo del constructor de Renderer corra.
    std::unique_ptr<SpriteSet> sprites_;
};

} // namespace render
