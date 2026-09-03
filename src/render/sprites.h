#pragma once
#include "raylib.h"
#include "../game/character.h"
#include "../game/enemy.h"

namespace render {

// Tamano del lienzo (en pixeles) sobre el que se dibujan los sprites antes
// de escalarlos con filtro "point" (nearest neighbor) — ver SpriteSet en
// sprites.cpp para el detalle de como se generan. Se exponen aca porque
// renderer.cpp los necesita para calcular el tamano final en pantalla
// (canvas * escala) y la relacion de tileado piso/pared con game::kTileSize.
constexpr int kCanvasPersonaje = 20;      // ancho de personajes/enemigos humanoides (y la Rata)
constexpr int kCanvasPersonajeAlto = 26;  // alto
constexpr int kCanvasTile = 16;           // tiles de piso/pared/cofre, cuadrados

// Todas las texturas pixel-art del juego: personajes del party, enemigos,
// cofres y tiles de piso/pared. Generadas por codigo (sin archivos de
// assets externos, mismo criterio que ya usa el audio sintetizado — ver
// render/audio.cpp) componiendo formas con las funciones ImageDraw* de
// raylib sobre un lienzo chico, y despues escalando con filtro POINT para
// que se vea nitido y "en bloques" — la estetica tipica de pixel art en vez
// de un blur al agrandar.
//
// Se crea una sola vez, DESPUES de InitWindow() (las texturas necesitan un
// contexto GL valido) — Renderer es quien la posee y la construye en su
// constructor. Las texturas de piso y pared llevan wrap REPEAT para poder
// tilearlas sobre areas grandes con DibujarTileado() (ver mas abajo).
class SpriteSet {
public:
    SpriteSet();
    ~SpriteSet();

    // No tiene sentido copiar un SpriteSet (serian dos juegos de texturas
    // GL independientes apuntando a la misma data) — cada Renderer tiene el
    // suyo propio.
    SpriteSet(const SpriteSet&) = delete;
    SpriteSet& operator=(const SpriteSet&) = delete;

    const Texture2D& Personaje(game::Role rol) const { return personajes_[static_cast<int>(rol)]; }
    const Texture2D& Enemigo(game::TipoEnemigo tipo) const { return enemigos_[static_cast<int>(tipo)]; }
    const Texture2D& CofreCerrado() const { return cofreCerrado_; }
    const Texture2D& CofreAbierto() const { return cofreAbierto_; }
    const Texture2D& TilePiso() const { return tilePiso_; }
    const Texture2D& TilePared() const { return tilePared_; }

    // Decoraciones sueltas de piso (grieta, musgo, escombros/huesos, charco)
    // — se reparten disperso por las salas (ver renderer.cpp) para que el
    // piso no se vea repetitivo ni "vacio". 'indice' se toma modulo
    // kNumDecoracionesPiso, asi que cualquier hash entero sirve sin
    // chequear rango antes.
    static constexpr int kNumDecoracionesPiso = 4;
    const Texture2D& DecoracionPiso(int indice) const {
        return decoracionesPiso_[((indice % kNumDecoracionesPiso) + kNumDecoracionesPiso) % kNumDecoracionesPiso];
    }
    const Texture2D& Antorcha() const { return antorcha_; }

private:
    Texture2D personajes_[4];  // indexado por game::Role
    Texture2D enemigos_[4];    // indexado por game::TipoEnemigo
    Texture2D cofreCerrado_;
    Texture2D cofreAbierto_;
    Texture2D tilePiso_;
    Texture2D tilePared_;
    Texture2D decoracionesPiso_[kNumDecoracionesPiso];
    Texture2D antorcha_;
};

// Dibuja un sprite de personaje/enemigo "parado en el piso": centrado en X
// sobre posicionPies.x, con los pies (borde inferior del lienzo) apoyados en
// posicionPies.y. Es el mismo anclaje que ya se veia en el prototipo
// aprobado por el usuario (la fila "escala real de juego" del prototipo) —
// evita que el personaje "flote" respecto de su posicion de colision, que
// sigue siendo la misma que antes (ver game::Character::Colisionador()).
void DibujarSpritePlantado(const Texture2D& textura, Vector2 posicionPies, float escala, Color tinte = WHITE);

// Dibuja un sprite centrado (X e Y) en 'centro' — para objetos simetricos
// sin nocion de "pies", como el cofre (que ya se posicionaba por su centro,
// ver game::Cofre::posicion).
void DibujarSpriteCentrado(const Texture2D& textura, Vector2 centro, float escala, Color tinte = WHITE);

// Dibuja 'textura' (debe tener wrap REPEAT — ver TilePiso()/TilePared())
// tileada para cubrir 'destinoMundo' (coordenadas de mundo, dentro de la
// camara 2D). El rectangulo fuente se calcula dividiendo las coordenadas de
// 'destinoMundo' por 'escalaTile' (game::kTileSize / kCanvasTile) en vez de
// arrancar siempre desde (0,0) — asi el patron queda alineado globalmente
// sin importar en que sala o pared se dibuje, y no se nota una costura en
// los bordes entre salas vecinas.
void DibujarTileado(const Texture2D& textura, Rectangle destinoMundo, float escalaTile);

} // namespace render
