#include "deambulante.h"
#include "dice.h"
#include <cmath>

namespace game {

namespace {

// Distancia por debajo de la cual se considera "llegado" al objetivo --
// bastante mayor que 0 a proposito, para no dejar un residuo de temblor
// (ir/venir de un pixel) cuando la velocidad y el paso de frame no dan un
// numero exacto.
constexpr float kDistanciaLlegada = 4.0f;

// Cuanto puede durar la pausa (segundos, entre 0 y este maximo elegido al
// azar con game::Roll) al llegar a un objetivo, segun el tipo -- los
// aldeanos se quedan mas tiempo "charlando" parados que un perro o un
// pajaro, que enseguida vuelven a moverse.
float PausaMaximaDe(TipoDeambulante tipo) {
    switch (tipo) {
        case TipoDeambulante::Perro:  return 1.0f;
        case TipoDeambulante::Pajaro: return 0.6f;
        default:                      return 2.5f;  // AldeanoA/B/C
    }
}

// Angulo y radio al azar (game::Roll, ya usado en todo el resto del
// proyecto para esto en vez de sumar una dependencia de RNG nueva) dentro
// del circulo de deambule -- Roll da enteros de 1 a N, de ahi el -1 para
// cubrir el 0 tambien.
Vec2 PuntoAlAzarEnCirculo(Vec2 centro, float radio) {
    float angulo = (float)(Roll(360) - 1) * (3.14159265f / 180.0f);
    float distancia = (float)(Roll(100) - 1) / 100.0f * radio;
    return Vec2{ centro.x + std::cos(angulo) * distancia, centro.y + std::sin(angulo) * distancia };
}

} // namespace

Deambulante CrearDeambulante(TipoDeambulante tipo, Vec2 anclaje, float radio, float velocidad) {
    Deambulante d;
    d.tipo = tipo;
    d.anclaje = anclaje;
    d.radio = radio;
    d.velocidad = velocidad;
    d.posicion = anclaje;
    d.objetivo = PuntoAlAzarEnCirculo(anclaje, radio);
    return d;
}

void ActualizarDeambulante(Deambulante& d, float dt) {
    if (d.pausa > 0.0f) {
        d.pausa -= dt;
        return;
    }

    Vec2 haciaObjetivo = d.objetivo - d.posicion;
    float distancia = Length(haciaObjetivo);
    if (distancia <= kDistanciaLlegada) {
        d.objetivo = PuntoAlAzarEnCirculo(d.anclaje, d.radio);
        d.pausa = (float)(Roll(100) - 1) / 100.0f * PausaMaximaDe(d.tipo);
        return;
    }

    Vec2 direccion = Normalize(haciaObjetivo);
    d.posicion = d.posicion + direccion * (d.velocidad * dt);
}

} // namespace game
