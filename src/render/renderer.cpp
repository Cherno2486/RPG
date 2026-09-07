#include "renderer.h"
#include "ui.h"
#include "raylib.h"
#include <cmath>
#include <cstdint>
#include <unordered_set>

namespace render {

namespace {
Color ColorDePiso()  { return Color{ 40, 38, 45, 255 }; }

// Hash entero determinístico por tile (no criptográfico, solo necesita
// verse disperso) — se usa para decidir, sin guardar ningún estado nuevo en
// la capa de juego, qué tiles de piso llevan una decoración suelta (y
// cuál) y qué tiles de pared "de frente" llevan una antorcha. Determinístico
// en las mismas coordenadas de tile: la mazmorra se ve igual cuadro a
// cuadro, no parpadea ni cambia de decoración sola.
uint32_t HashTile(int x, int y) {
    uint32_t h = (uint32_t)(x * 374761393 + y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Cuantos tiles de piso de cada 100 (dentro de una sala) llevan alguna
// decoración suelta — disperso a propósito, para que se note como "detalle"
// y no como otro patrón repetitivo.
constexpr int kChanceDecoracionPisoDe100 = 12;

// Empaqueta una coordenada de tile (con offset para tolerar negativos, que
// no deberían darse pero cuestan nada de cubrir) en una clave de 64 bits
// para el set de tiles de pared.
int64_t ClaveTile(int tx, int ty) {
    return (int64_t)(tx + 100000) * 1000000LL + (int64_t)(ty + 100000);
}

// Relacion entre un tile de juego (game::kTileSize, 48px) y el lienzo nativo
// de las texturas de piso/pared (kCanvasTile, 16px) — cuanto hay que
// escalar esas texturas para que un tile de textura cubra exactamente un
// tile de juego. La usan tanto el tileado de piso/pared como el escalado de
// personajes/enemigos/cofres (comparten el mismo lienzo base).
constexpr float kEscalaTile = game::kTileSize / (float)kCanvasTile;  // 3.0

// Escala de los sprites de personaje/enemigo comunes y del cofre en el
// mapa — elegidas para que ocupen un tamaño similar al que tenian los
// circulos que reemplazan (radio 14 los personajes, ~16-18 los enemigos
// comunes, mitad=14 el cofre). El jefe usa una escala mayor aparte (ver
// abajo) para seguir notandose mas grande e imponente que el resto, como ya
// pasaba con su radio de 26.
constexpr float kEscalaPersonaje = 1.8f;
constexpr float kEscalaJefe = 2.4f;
constexpr float kEscalaCofre = 1.6f;
constexpr float kEscalaAntorcha = 1.6f;
constexpr float kEscalaDeambulante = 1.6f;        // perro y aldeanos
constexpr float kEscalaDeambulantePajaro = 1.1f;  // mas chico que el resto

Color ColorDeGrillaJefe() { return Color{ 230, 190, 80, 255 }; }  // anillo dorado detras del Capitan

// Cofres: sprite de tapa cerrada (con banda dorada) o abierto (tapa
// levantada, interior oscuro a la vista) — mismo criterio que antes (se
// notaba a simple vista si ya no tenia nada adentro), ahora con arte
// pixel-art en vez de un rectangulo liso.
void DibujarCofre(const game::Cofre& cofre, const SpriteSet& sprites) {
    const Texture2D& tex = cofre.abierto ? sprites.CofreAbierto() : sprites.CofreCerrado();
    DibujarSpriteCentrado(tex, Vector2{ cofre.posicion.x, cofre.posicion.y }, kEscalaCofre);
}

// Edificios de la Ciudad (ver EstadoJuego::Ciudad en main.cpp): no tienen
// arte pixel-art propio (a proposito, para no sumar otro juego de texturas
// solo para 4 construcciones estaticas) -- se dibujan con primitivas de
// raylib, mas el nombre arriba (mismo recurso que el nombre de un enemigo)
// para que se lea que son interactuables.
//
// Silueta distinta por tipo (no solo el color del techo, como en la primera
// version) -- pedido directo del usuario tras ver los 3 originales ("todos
// los edificios son iguales"): Herreria suma una chimenea (forja), Tienda
// cambia el techo a punta por un toldo de mercado a rayas, Academia suma dos
// columnas clasicas, y la Entrada a las mazmorras deja de ser una casa del
// todo -- pasa a ser un arco de piedra con un hueco oscuro en el medio (ver
// DibujarPortalEntrada mas abajo), que ya venia siendo su lectura ("portal")
// en el color de techo de la version vieja.
Color ColorDeTechoEdificio(game::TipoEdificio tipo) {
    switch (tipo) {
        case game::TipoEdificio::Herreria: return Color{ 168, 92, 48, 255 };   // tejas oxidadas
        case game::TipoEdificio::Tienda:   return Color{ 70, 128, 168, 255 };  // toldo azulado
        case game::TipoEdificio::Academia: return Color{ 92, 158, 96, 255 };   // techo verde biblioteca
        default:                           return Color{ 132, 64, 158, 255 }; // EntradaMazmorras: portal violeta
    }
}

// Arco de piedra para la Entrada a las mazmorras -- reemplaza el viejo
// "casa con techo violeta" por algo que se lea como un pasaje, no como un
// cuarto edificio mas: dos pilares + un dintel curvo, con un hueco oscuro
// en el medio (mismo color que el interior de una puerta, pero mas alto,
// para que se note que ahi "se entra a otro lado", no a un local).
void DibujarPortalEntrada(const game::Edificio& edificio) {
    constexpr float kAnchoHueco = game::kTileSize * 0.9f;
    constexpr float kAltoHueco = game::kTileSize * 1.5f;
    constexpr float kAnchoPilar = 10.0f;
    constexpr float kAltoDintel = 16.0f;

    float yBase = edificio.posicion.y;
    float yTope = yBase - kAltoHueco;
    float xHuecoIzq = edificio.posicion.x - kAnchoHueco * 0.5f;
    float xHuecoDer = edificio.posicion.x + kAnchoHueco * 0.5f;

    Color piedra = Color{ 110, 100, 96, 255 };
    Color piedraOsc = Color{ 70, 64, 60, 255 };
    Color violeta = ColorDeTechoEdificio(game::TipoEdificio::EntradaMazmorras);

    // Pilares a los costados del hueco.
    DrawRectangle((int)(xHuecoIzq - kAnchoPilar), (int)yTope, (int)kAnchoPilar, (int)kAltoHueco, piedra);
    DrawRectangle((int)xHuecoDer, (int)yTope, (int)kAnchoPilar, (int)kAltoHueco, piedra);
    DrawRectangleLines((int)(xHuecoIzq - kAnchoPilar), (int)yTope, (int)kAnchoPilar, (int)kAltoHueco, piedraOsc);
    DrawRectangleLines((int)xHuecoDer, (int)yTope, (int)kAnchoPilar, (int)kAltoHueco, piedraOsc);

    // Dintel curvo (semicirculo) uniendo los dos pilares por arriba, con un
    // filo violeta -- lo unico que conserva del color de techo viejo.
    float xCentro = edificio.posicion.x;
    float radioDintel = kAnchoHueco * 0.5f + kAnchoPilar;
    DrawCircleSector(Vector2{ xCentro, yTope }, radioDintel, 180.0f, 360.0f, 24, piedra);
    DrawRing(Vector2{ xCentro, yTope }, radioDintel - 4.0f, radioDintel, 180.0f, 360.0f, 24, violeta);
    DrawLine((int)(xCentro - radioDintel), (int)yTope, (int)(xCentro + radioDintel), (int)yTope, piedraOsc);

    // Hueco oscuro (el "pasaje" en si) -- mas alto que una puerta comun a
    // proposito, para que no se lea como la puerta de una casa.
    DrawRectangle((int)xHuecoIzq, (int)(yTope + kAltoDintel * 0.3f), (int)kAnchoHueco,
                  (int)(kAltoHueco - kAltoDintel * 0.3f), Color{ 18, 14, 22, 255 });

    const char* nombre = game::NombreDeEdificio(edificio.tipo);
    int anchoTexto = MeasureText(nombre, 14);
    DrawText(nombre, (int)(edificio.posicion.x - anchoTexto / 2.0f), (int)(yTope - 24), 14, RAYWHITE);
}

void DibujarEdificio(const game::Edificio& edificio) {
    if (edificio.tipo == game::TipoEdificio::EntradaMazmorras) {
        DibujarPortalEntrada(edificio);
        return;
    }

    constexpr float kAncho = game::kTileSize * 1.7f;
    constexpr float kAltoCuerpo = game::kTileSize * 1.1f;
    constexpr float kAltoTecho = game::kTileSize * 0.7f;
    constexpr float kAleroTecho = 8.0f;

    // 'posicion' es la base ("pies") del edificio, igual convencion que
    // DibujarSpritePlantado -- asi el punto de interaccion (main.cpp mide
    // distancia contra esta misma posicion) queda a la altura de la puerta,
    // no del techo.
    float x0 = edificio.posicion.x - kAncho * 0.5f;
    float x1 = edificio.posicion.x + kAncho * 0.5f;
    float yBase = edificio.posicion.y;
    float yTechoBase = yBase - kAltoCuerpo;
    float yCumbre = yTechoBase - kAltoTecho;

    // Herreria tiene el cuerpo de piedra mas oscura (forja), Tienda/Academia
    // el tono calido de siempre.
    Color colorCuerpo = (edificio.tipo == game::TipoEdificio::Herreria)
        ? Color{ 72, 66, 60, 255 } : Color{ 96, 88, 76, 255 };
    DrawRectangle((int)x0, (int)yTechoBase, (int)kAncho, (int)kAltoCuerpo, colorCuerpo);
    DrawRectangleLines((int)x0, (int)yTechoBase, (int)kAncho, (int)kAltoCuerpo, Color{ 40, 36, 30, 255 });

    // Puerta simple, centrada, para que se lea como entrada.
    float anchoPuerta = kAncho * 0.28f;
    float altoPuerta = kAltoCuerpo * 0.6f;
    DrawRectangle((int)(edificio.posicion.x - anchoPuerta * 0.5f), (int)(yBase - altoPuerta),
                  (int)anchoPuerta, (int)altoPuerta, Color{ 30, 26, 22, 255 });

    if (edificio.tipo == game::TipoEdificio::Tienda) {
        // Toldo de mercado: en vez del techo a dos aguas de siempre, una
        // sola faldilla inclinada que sobresale hacia el frente, a rayas
        // azul/blanco -- silueta bien distinta de una casa con techo.
        constexpr int kFranjas = 5;
        float anchoFranja = (kAncho + kAleroTecho * 2.0f) / kFranjas;
        float yToldoFrente = yTechoBase + kAltoTecho * 0.9f;
        for (int i = 0; i < kFranjas; ++i) {
            float xi = x0 - kAleroTecho + anchoFranja * i;
            Color franja = (i % 2 == 0) ? ColorDeTechoEdificio(game::TipoEdificio::Tienda)
                                         : Color{ 235, 235, 230, 255 };
            Vector2 p1{ xi, yTechoBase };
            Vector2 p2{ xi + anchoFranja, yTechoBase };
            Vector2 p3{ xi + anchoFranja * 0.6f, yToldoFrente };
            Vector2 p4{ xi + anchoFranja * 0.4f, yToldoFrente };
            DrawTriangle(p1, p4, p2, franja);
            DrawTriangle(p2, p4, p3, franja);
        }
        DrawLine((int)(x0 - kAleroTecho), (int)yTechoBase, (int)(x1 + kAleroTecho), (int)yTechoBase,
                 Color{ 40, 36, 30, 255 });
    } else {
        Vector2 puntaTecho{ edificio.posicion.x, yCumbre };
        Vector2 baseIzq{ x0 - kAleroTecho, yTechoBase };
        Vector2 baseDer{ x1 + kAleroTecho, yTechoBase };
        DrawTriangle(baseIzq, puntaTecho, baseDer, ColorDeTechoEdificio(edificio.tipo));

        if (edificio.tipo == game::TipoEdificio::Herreria) {
            // Chimenea: un rectangulo angosto asomando del lado derecho del
            // techo -- lectura rapida de "aca se forja algo".
            float anchoChimenea = 10.0f;
            float xChimenea = edificio.posicion.x + kAncho * 0.22f;
            float yChimeneaTope = yCumbre + (yTechoBase - yCumbre) * 0.35f - 14.0f;
            DrawRectangle((int)(xChimenea - anchoChimenea * 0.5f), (int)yChimeneaTope,
                          (int)anchoChimenea, (int)(yTechoBase - yChimeneaTope + 4.0f),
                          Color{ 80, 74, 68, 255 });
        } else if (edificio.tipo == game::TipoEdificio::Academia) {
            // Dos columnas clasicas flanqueando la puerta -- lectura de
            // "academia/biblioteca", no una casa mas.
            float anchoColumna = 7.0f;
            float xColIzq = edificio.posicion.x - anchoPuerta * 0.5f - anchoColumna - 4.0f;
            float xColDer = edificio.posicion.x + anchoPuerta * 0.5f + 4.0f;
            Color colorColumna = Color{ 210, 205, 190, 255 };
            DrawRectangle((int)xColIzq, (int)(yBase - altoPuerta), (int)anchoColumna, (int)altoPuerta, colorColumna);
            DrawRectangle((int)xColDer, (int)(yBase - altoPuerta), (int)anchoColumna, (int)altoPuerta, colorColumna);
        }
    }

    const char* nombre = game::NombreDeEdificio(edificio.tipo);
    int anchoTexto = MeasureText(nombre, 14);
    DrawText(nombre, (int)(edificio.posicion.x - anchoTexto / 2.0f), (int)(yCumbre - 20), 14, RAYWHITE);
}

// Fuente decorativa de la plaza (ver ConstruirCiudad en main.cpp): igual que
// los edificios, sin arte pixel-art propio -- primitivas de raylib nomas,
// con un pulso chico de tamano en el agua (mismo recurso que ya usan la
// antorcha y la trampa de fuego, GetTime() variando algo cuadro a cuadro
// para que no se sienta estatica del todo). Sin colision, igual que los
// edificios -- el party puede caminar "a traves" del circulo.
void DibujarFuente(Vector2 centro, float tiempo) {
    constexpr float kRadio = game::kTileSize * 1.1f;
    Color piedra = { 150, 142, 128, 255 };
    Color piedraOsc = { 110, 104, 92, 255 };
    Color agua = { 90, 150, 200, 210 };
    Color aguaClara = { 150, 200, 230, 200 };

    DrawCircle((int)centro.x, (int)centro.y, kRadio, piedra);
    DrawCircleLines((int)centro.x, (int)centro.y, (int)kRadio, piedraOsc);
    float pulso = 0.85f + 0.06f * sinf(tiempo * 2.0f);
    DrawCircle((int)centro.x, (int)centro.y, kRadio * 0.72f * pulso, agua);
    DrawCircle((int)centro.x, (int)centro.y, kRadio * 0.3f, piedraOsc);
    DrawCircle((int)centro.x, (int)centro.y, kRadio * 0.18f, aguaClara);
}

// Tinte por instancia de un pajaro (ver game::TipoDeambulante::Pajaro): la
// textura base es neutra (ver CrearPajaro en sprites.cpp) para poder reusar
// la misma para las 3 instancias que arma ConstruirCiudad, tinendola
// distinto segun su posicion en el vector -- mismo mecanismo que ya usa
// TinteDecoracionPorTema, aplicado por indice en vez de por tema.
Color TintePajaroPorIndice(int indice) {
    switch (((indice % 3) + 3) % 3) {
        case 0:  return Color{ 150, 110, 80, 255 };   // gorrion, marron
        case 1:  return Color{ 100, 100, 108, 255 };  // paloma, gris
        default: return Color{ 205, 80, 65, 255 };    // petirrojo, pecho rojizo
    }
}
} // namespace

Renderer::Renderer(int anchoVentana, int altoVentana, const char* titulo)
    : anchoVentana_(anchoVentana), altoVentana_(altoVentana) {
    InitWindow(anchoVentana_, altoVentana_, titulo);
    SetTargetFPS(60);
    // raylib por defecto usa ESC como "tecla de salida" (hace que
    // WindowShouldClose() de true apenas se aprieta, sin pasar por el loop
    // principal). Lo desactivamos porque ESC ahora tiene un uso propio
    // dentro del juego (volver de la pantalla "Sobre mi" al menu, ver
    // EstadoJuego::SobreMi en main.cpp) — "Salir" en el menu de inicio
    // sigue siendo la unica forma de cerrar el juego.
    SetExitKey(KEY_NULL);

    camara_.offset = Vector2{ anchoVentana_ / 2.0f, altoVentana_ / 2.0f };
    camara_.target = Vector2{ 0.0f, 0.0f };
    camara_.rotation = 0.0f;
    camara_.zoom = 1.0f;

    // Recien aca hay un contexto GL valido (InitWindow ya corrio arriba) —
    // por eso SpriteSet no puede ser un miembro por valor construido antes
    // del cuerpo de este constructor.
    sprites_ = std::make_unique<SpriteSet>();
}

Renderer::~Renderer() {
    // sprites_ tiene que liberar sus texturas GL (UnloadTexture) ANTES de
    // que CloseWindow() cierre el contexto — si no, se libera con el
    // contexto ya invalido y crashea. El orden normal de destruccion de
    // miembros (reverso a la declaracion) no alcanza aca porque los
    // miembros se destruyen DESPUES de que termina el cuerpo de este
    // destructor, no antes; por eso el reset() explicito.
    sprites_.reset();
    CloseWindow();
}

void Renderer::DibujarEscenarioSinUI(const game::Dungeon& mazmorra, const game::Party& party,
                                      const std::vector<game::Enemy>& enemigos, const std::vector<game::Cofre>& cofres,
                                      int tema, const std::vector<game::Edificio>& edificios,
                                      const std::vector<game::Deambulante>& deambulantes,
                                      const std::vector<game::Vec2>& fuentes) {
    ClearBackground(ColorDePiso());

    // La camara sigue al lider: la mazmorra generada por salas es mas
    // grande que la ventana, asi que sin esto no se veria nada apenas se
    // sale de la sala inicial.
    camara_.target = Vector2{ party.Lider().Posicion().x, party.Lider().Posicion().y };

    BeginMode2D(camara_);

    // La Ciudad (ver EstadoJuego::Ciudad en main.cpp) usa su propio juego de
    // texturas (piso/pared/decoracion), no uno de los 3 temas de mazmorra —
    // ver render::kTemaCiudad en sprites.h para el porque (evitar que la
    // Ciudad "parezca una mazmorra mas" con la paleta de Castillo, que era
    // lo que hacia la primera version). Sin tinte: esas texturas ya tienen
    // el color final, a diferencia de las decoraciones de mazmorra que se
    // tinen por tema.
    bool esCiudad = (tema == kTemaCiudad);
    const Texture2D& texPiso = esCiudad ? sprites_->TilePisoCiudad() : sprites_->TilePiso(tema);
    const Texture2D& texPared = esCiudad ? sprites_->TileParedCiudad() : sprites_->TilePared(tema);
    Color tinteDecoracion = esCiudad ? WHITE : TinteDecoracionPorTema(tema);

    // Piso tileado, solo dentro de cada sala (los pasillos siguen mostrando
    // el color de fondo liso de ClearBackground, que coincide con el color
    // base del tile — ver ColorDePiso()/CrearTilePiso()). El juego de
    // texturas depende de 'tema' (ver SpriteSet::TilePiso/TilePared).
    for (const auto& sala : mazmorra.Habitaciones()) {
        float x0 = sala.x * game::kTileSize;
        float y0 = sala.y * game::kTileSize;
        float x1 = (sala.x + sala.ancho) * game::kTileSize;
        float y1 = (sala.y + sala.alto) * game::kTileSize;
        DibujarTileado(texPiso, Rectangle{ x0, y0, x1 - x0, y1 - y0 }, kEscalaTile);

        // Decoracion suelta de piso (grieta/musgo/escombros/charco en una
        // mazmorra; pasto/maceta en la Ciudad), disperso por tile via
        // HashTile — se dibuja ANTES que las paredes (mas abajo), asi que si
        // algun tile "cae" sobre una muesca de sala en L o un pilar (que
        // tambien son parte de este bounding box, ver "Variedad de formas de
        // sala"), la pared que se dibuja despues lo tapa sin dejar rastro;
        // no hace falta que este loop sepa distinguir piso real de hueco.
        for (int ty = sala.y; ty < sala.y + sala.alto; ++ty) {
            for (int tx = sala.x; tx < sala.x + sala.ancho; ++tx) {
                uint32_t h = HashTile(tx, ty);
                if ((int)(h % 100) >= kChanceDecoracionPisoDe100) continue;
                Vector2 centro{ (tx + 0.5f) * game::kTileSize, (ty + 0.5f) * game::kTileSize };
                if (esCiudad) {
                    int variante = (int)((h / 100) % SpriteSet::kNumDecoracionesCiudad);
                    DibujarSpriteCentrado(sprites_->DecoracionCiudad(variante), centro, kEscalaTile, tinteDecoracion);
                } else {
                    int variante = (int)((h / 100) % SpriteSet::kNumDecoracionesPiso);
                    DibujarSpriteCentrado(sprites_->DecoracionPiso(variante), centro, kEscalaTile, tinteDecoracion);
                }
            }
        }
    }

    // Paredes, con textura de ladrillos tileada en vez de un rectangulo
    // liso.
    for (const auto& pared : mazmorra.Paredes()) {
        DibujarTileado(texPared, Rectangle{ pared.x, pared.y, pared.width, pared.height }, kEscalaTile);
    }

    // Fuentes decorativas (solo la Ciudad las pasa) — ver DibujarFuente
    // arriba.
    {
        float tiempo = (float)GetTime();
        for (const auto& fuente : fuentes) {
            DibujarFuente(Vector2{ fuente.x, fuente.y }, tiempo);
        }
    }

    // Antorchas: solo en tiles de pared "de frente" (el tile de abajo, hacia
    // el jugador, NO es pared — por construccion de Paredes(), ver "Sistema
    // de mazmorras", eso significa que es piso) — tipicamente la fila
    // superior de cada sala, la unica cara de pared que este angulo de
    // camara realmente "mira". Se reparten cada 3 tiles, con un offset por
    // fila derivado del hash para que no quede perfectamente alineado entre
    // salas distintas. La Ciudad no lleva antorchas (una tapia con seto no
    // es un buen lugar para montar una — ver DibujarFuente arriba para su
    // propio detalle de ambientacion).
    if (!esCiudad) {
        std::unordered_set<int64_t> tilesDePared;
        tilesDePared.reserve(mazmorra.Paredes().size() * 2);
        for (const auto& pared : mazmorra.Paredes()) {
            int tx = (int)std::lround(pared.x / game::kTileSize);
            int ty = (int)std::lround(pared.y / game::kTileSize);
            tilesDePared.insert(ClaveTile(tx, ty));
        }

        float tiempo = (float)GetTime();
        for (const auto& pared : mazmorra.Paredes()) {
            int tx = (int)std::lround(pared.x / game::kTileSize);
            int ty = (int)std::lround(pared.y / game::kTileSize);
            if (tilesDePared.count(ClaveTile(tx, ty + 1)) != 0) continue;  // no es pared "de frente"

            uint32_t h = HashTile(tx, ty);
            int offsetFila = (int)(HashTile(0, ty) % 3);
            if (((tx + offsetFila) % 3 + 3) % 3 != 0) continue;

            float fase = (float)(h % 628) / 100.0f;  // 0..~2π, distinto por antorcha
            float escala = kEscalaAntorcha * (1.0f + 0.06f * sinf(tiempo * 6.0f + fase));
            Vector2 posicionPies{ (tx + 0.5f) * game::kTileSize, (ty + 1) * game::kTileSize };
            DibujarSpritePlantado(sprites_->Antorcha(), posicionPies, escala);
        }
    }

    // Trampas de piso: fuego con un parpadeo de escala (mismo recurso visual
    // que ya usa la antorcha, GetTime() variando el tamano cuadro a cuadro)
    // y acido con un pulso de opacidad, para que se lean como "algo activo"
    // en vez de una decoracion mas del piso -- coherente con que SI hacen
    // dano (ver el chequeo de trampas en main.cpp).
    {
        float tiempo = (float)GetTime();
        for (const auto& trampa : mazmorra.Trampas()) {
            Vector2 centro{ trampa.area.x + trampa.area.width * 0.5f, trampa.area.y + trampa.area.height * 0.5f };
            if (trampa.tipo == game::TipoTrampa::Fuego) {
                float escala = kEscalaTile * (1.0f + 0.08f * sinf(tiempo * 7.0f + centro.x));
                DibujarSpriteCentrado(sprites_->Trampa(trampa.tipo), centro, escala);
            } else {
                unsigned char alfa = (unsigned char)(200 + 55 * sinf(tiempo * 3.0f + centro.y));
                DibujarSpriteCentrado(sprites_->Trampa(trampa.tipo), centro, kEscalaTile, Color{ 255, 255, 255, alfa });
            }
        }
    }

    // Cofres
    for (const auto& cofre : cofres) {
        DibujarCofre(cofre, *sprites_);
    }

    // Edificios (solo la Ciudad los pasa; el resto de las pantallas usa el
    // default vacio) -- ver DibujarEdificio arriba.
    for (const auto& edificio : edificios) {
        DibujarEdificio(edificio);
    }

    // Deambulantes (perro/pajaros/aldeanos, solo la Ciudad los pasa): mismo
    // anclaje "plantado" que un personaje/enemigo (ver DibujarSpritePlantado),
    // sin nombre ni ningun otro adorno encima porque son pura ambientacion,
    // no interactuables (ver game::Deambulante). Los pajaros se tinen
    // distinto por indice (ver TintePajaroPorIndice arriba) y se dibujan mas
    // chicos que el resto.
    for (size_t i = 0; i < deambulantes.size(); ++i) {
        const auto& d = deambulantes[i];
        Vector2 pos{ d.posicion.x, d.posicion.y };
        bool esPajaro = (d.tipo == game::TipoDeambulante::Pajaro);
        float escala = esPajaro ? kEscalaDeambulantePajaro : kEscalaDeambulante;
        Color tinte = esPajaro ? TintePajaroPorIndice((int)i) : WHITE;
        DibujarSpritePlantado(sprites_->Deambulante(d.tipo), pos, escala, tinte);
    }

    // Enemigos vivos en la mazmorra: su sprite pixel-art segun tipo, con su
    // nombre arriba para saber que es interactuable.
    for (const auto& enemigo : enemigos) {
        if (enemigo.Vencido()) continue;
        Vector2 posEnemigo = { enemigo.Posicion().x, enemigo.Posicion().y };
        bool esJefe = game::EsJefe(enemigo.Tipo());
        float escala = esJefe ? kEscalaJefe : kEscalaPersonaje;
        float alturaSprite = kCanvasPersonajeAlto * escala;

        // El jefe lleva un anillo dorado detras del sprite (mismo recurso
        // visual que antes tenia como contorno de su circulo), para que se
        // note a simple vista que es distinto apenas se lo ve.
        if (esJefe) {
            Vector2 centroAnillo{ posEnemigo.x, posEnemigo.y - alturaSprite * 0.5f };
            DrawCircleLines((int)centroAnillo.x, (int)centroAnillo.y, kCanvasPersonaje * escala * 0.6f, ColorDeGrillaJefe());
        }
        DibujarSpritePlantado(sprites_->Enemigo(enemigo.Tipo()), posEnemigo, escala);

        // Los agresivos (ver game::EsAgresivo) persiguen al jugador durante
        // la exploracion en vez de esperar [E] — el nombre en rojo mas un
        // "!" arriba avisan de eso antes de que se acerquen demasiado.
        bool esAgresivo = game::EsAgresivo(enemigo.Tipo());
        Color colorNombre = esAgresivo ? Color{ 255, 120, 120, 255 } : RAYWHITE;
        int anchoTexto = MeasureText(enemigo.Nombre().c_str(), 12);
        DrawText(enemigo.Nombre().c_str(), (int)posEnemigo.x - anchoTexto / 2, (int)(posEnemigo.y - alturaSprite - 14), 12, colorNombre);
        if (esAgresivo) {
            const char* marca = "!";
            int anchoMarca = MeasureText(marca, 16);
            DrawText(marca, (int)posEnemigo.x - anchoMarca / 2, (int)(posEnemigo.y - alturaSprite - 30), 16, Color{ 255, 60, 60, 255 });
        }
    }

    // Party: se dibuja del ultimo al primero para que el lider quede arriba de los demas
    const auto& miembros = party.Miembros();
    for (size_t i = miembros.size(); i-- > 0; ) {
        const auto& personaje = miembros[i];
        Vector2 pos = { personaje.Posicion().x, personaje.Posicion().y };
        DibujarSpritePlantado(sprites_->Personaje(personaje.Rol()), pos, kEscalaPersonaje);
    }

    EndMode2D();
}

void Renderer::DibujarFrame(const game::Dungeon& mazmorra, const game::Party& party,
                             const std::vector<game::Enemy>& enemigos, const std::vector<game::Cofre>& cofres, int tema,
                             bool panelExpandido, const std::string& promptInteraccion, const std::string& mensajeFlotante,
                             const std::vector<game::Edificio>& edificios,
                             const std::vector<game::Deambulante>& deambulantes,
                             const std::vector<game::Vec2>& fuentes) {
    BeginDrawing();

    DibujarEscenarioSinUI(mazmorra, party, enemigos, cofres, tema, edificios, deambulantes, fuentes);

    ui::DibujarPanelParty(party, panelExpandido, *sprites_);

    if (!mensajeFlotante.empty()) {
        int anchoTexto = MeasureText(mensajeFlotante.c_str(), 18);
        int x = (anchoVentana_ - anchoTexto) / 2;
        int y = altoVentana_ - 68;
        DrawRectangle(x - 12, y - 6, anchoTexto + 24, 30, Color{ 30, 28, 20, 210 });
        DrawText(mensajeFlotante.c_str(), x, y, 18, Color{ 230, 210, 140, 255 });
    }

    if (!promptInteraccion.empty()) {
        int anchoTexto = MeasureText(promptInteraccion.c_str(), 16);
        DrawText(promptInteraccion.c_str(), (anchoVentana_ - anchoTexto) / 2, altoVentana_ - 40, 16, Color{ 255, 235, 180, 255 });
    }

    DrawFPS(anchoVentana_ - 90, 10);

    // Hint fijo de guardado — F5 funciona en toda la exploracion (ver
    // main.cpp), asi que el recordatorio tambien es fijo, no depende de
    // estar cerca de nada como el prompt de interaccion.
    DrawText("[F5] Guardar", 16, altoVentana_ - 24, 14, Color{ 150, 150, 160, 255 });

    EndDrawing();
}

} // namespace render
