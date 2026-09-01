#pragma once

// Efectos de estado aplicables durante el combate (buffs/debuffs), en la
// linea de lo que pidió el diseño: combate tipo BG3 con dados + efectos.

namespace game {

enum class TipoEfecto {
    Aturdido,    // pierde el turno mientras dura
    Veneno,      // dano por turno (damage over time) al empezar el turno del portador
    Escudo,      // absorbe 'magnitud' puntos de daño antes de tocar la vida
    Debilitado,  // resta 'magnitud' al bonus de ataque mientras dura
    Marcado,     // fuerza a los enemigos a priorizar atacar a este objetivo (aggro del tanque)
};

const char* NombreEfecto(TipoEfecto tipo);

struct EfectoActivo {
    TipoEfecto tipo;
    int duracionTurnos;  // turnos restantes; se descuenta al empezar el turno del portador
    int magnitud;        // uso segun el tipo: dano del veneno, puntos de escudo, resta de ataque...
};

} // namespace game
