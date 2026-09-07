#pragma once
#include "mathtypes.h"

// Entidades puramente decorativas que le dan "vida" a la Ciudad (ver
// EstadoJuego::Ciudad en main.cpp): un perro, pajaros y aldeanos caminando
// sola por la plaza y la calle de comercios. Nacen del pedido directo del
// usuario tras ver la primera version de la Ciudad ("parece una mazmorra
// mas... quiero que tenga vida, que se mueva un perro, algunos pajaros").
//
// A diferencia de un game::Enemy o un game::Character, no tienen stats,
// bando, ni ninguna interaccion posible (ver el scoping de esta vuelta en
// docs/design.md, seccion "La Ciudad") -- main.cpp las actualiza cada frame
// con ActualizarDeambulante() mientras el jugador esta parado en la Ciudad,
// y el renderer las dibuja con su sprite segun 'tipo' (ver
// render::SpriteSet::Deambulante). Deliberadamente NO chocan contra las
// paredes de la Ciudad (evitaria reusar la logica de colision de Dungeon
// para algo que no importa si por un instante el sprite pisa el borde) --
// en cambio quedan acotadas a un circulo de radio 'radio' alrededor de su
// punto de anclaje, elegido a mano en ConstruirCiudad() para cubrir zonas de
// piso real, asi que en la practica no se los ve alejarse hacia una pared.
//
// No se guardan en el sistema de guardado (game/save.h) -- mismo criterio
// que game::Edificio: la Ciudad es siempre el mismo layout, asi que se
// reconstruyen identicos (en una posicion inicial fija, no donde haya
// quedado el anclaje la ultima vez) cada vez que se entra o se carga una
// partida parada ahi.
namespace game {

enum class TipoDeambulante { Perro, Pajaro, AldeanoA, AldeanoB, AldeanoC };

struct Deambulante {
    TipoDeambulante tipo = TipoDeambulante::Perro;

    // Centro del area donde deambula, y que tan lejos de ese centro puede
    // llegar a elegir su proximo objetivo (ver ActualizarDeambulante) --
    // ambos fijos, elegidos en ConstruirCiudad() para caer dentro de piso
    // caminable de la sala que le toca.
    Vec2 anclaje;
    float radio = 70.0f;

    Vec2 posicion;  // arranca en 'anclaje', ver CrearDeambulante
    Vec2 objetivo;  // proximo punto al que camina (siempre dentro de 'radio')
    float velocidad = 30.0f;  // pixeles por segundo

    // Cuenta regresiva (segundos) parado en 'objetivo' antes de elegir uno
    // nuevo -- sin esto, todos caminarian sin parar todo el tiempo, mucho
    // menos creible que hacer alguna pausa entre destino y destino (mismo
    // motivo por el que un enemigo agresivo no interesa aca: esto es pura
    // ambientacion, no gameplay).
    float pausa = 0.0f;
};

// Arma un Deambulante ya listo para actualizar cuadro a cuadro: arranca
// parado en 'anclaje' con un primer objetivo ya elegido al azar dentro de
// 'radio' (ver ElegirNuevoObjetivo en deambulante.cpp) para que no todos
// arranquen quietos el primer instante despues de entrar a la Ciudad.
Deambulante CrearDeambulante(TipoDeambulante tipo, Vec2 anclaje, float radio, float velocidad);

// Se llama una vez por frame por cada deambulante activo (ver
// EstadoJuego::Ciudad en main.cpp): si todavia esta en pausa, solo descuenta
// 'dt'; si no, avanza 'd.posicion' hacia 'd.objetivo' a razon de
// 'd.velocidad' pixeles por segundo y, al llegar (distancia menor a un
// margen chico), elige un nuevo objetivo al azar dentro de 'd.radio' de
// 'd.anclaje' y arranca una pausa nueva (tambien al azar, mas larga cuanto
// mas "humano" el tipo -- ver deambulante.cpp) antes de volver a moverse.
void ActualizarDeambulante(Deambulante& d, float dt);

} // namespace game
