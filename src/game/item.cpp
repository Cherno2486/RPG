#include "item.h"
#include "combat_state.h"
#include "dice.h"
#include <algorithm>
#include <cstdio>

namespace game {

Item PocionCuracionMenor() {
    return Item{"Pocion de Curacion Menor", "Cura 2d6+2 de vida.",
                TipoItem::Consumible, EfectoItem::CurarVida, RanuraEquipo::Ninguna, 2, 6, 2};
}

Item ElixirDeEnergia() {
    // Descripcion generica a proposito: el mismo elixir restaura Resistencia
    // en un personaje fisico (Tanque/Danio) o Concentracion en un mago
    // (Soporte/Control) segun a quien se le use — ver
    // game::NombreRecurso/UsarItem, que arma el texto final con el nombre
    // especifico de cada uno.
    return Item{"Elixir de Energia", "Restaura 1d6+4 de Resistencia o Concentracion, segun quien lo use.",
                TipoItem::Consumible, EfectoItem::CurarRecurso, RanuraEquipo::Ninguna, 1, 6, 4};
}

Item PiedraDeFuerza() {
    return Item{"Piedra de Fuerza", "Arma: sube el ataque en 1, para siempre.",
                TipoItem::Mejora, EfectoItem::MejorarAtaque, RanuraEquipo::Arma, 0, 0, 1};
}

Item AmuletoDeProteccion() {
    return Item{"Amuleto de Proteccion", "Accesorio: sube la defensa en 1, para siempre.",
                TipoItem::Mejora, EfectoItem::MejorarDefensa, RanuraEquipo::Accesorio, 0, 0, 1};
}

Item DagaVeloz() {
    return Item{"Daga Veloz", "Arma: sube la velocidad en 10, para siempre.",
                TipoItem::Mejora, EfectoItem::MejorarVelocidad, RanuraEquipo::Arma, 0, 0, 10};
}

Item TalismanDeVitalidad() {
    return Item{"Talisman de Vitalidad", "Accesorio: sube la vida maxima en 5, para siempre.",
                TipoItem::Mejora, EfectoItem::MejorarVidaMaxima, RanuraEquipo::Accesorio, 0, 0, 5};
}

Item BombaDeVeneno() {
    return Item{"Bomba de Veneno",
                "Combate: aplica Veneno a un enemigo (3 de daño por turno durante 3 turnos), sin necesidad de acertar un golpe.",
                TipoItem::Consumible, EfectoItem::AplicarEstado, RanuraEquipo::Ninguna,
                /*dados=duracionTurnos*/ 3, /*caras=no usado*/ 0, /*bono=magnitud*/ 3,
                TipoEfecto::Veneno, /*apuntaAEnemigo=*/true};
}

Item FrascoDeEscudo() {
    return Item{"Frasco de Escudo",
                "Combate: da Escudo a un aliado (absorbe 6 de daño antes de tocarle la vida).",
                TipoItem::Consumible, EfectoItem::AplicarEstado, RanuraEquipo::Ninguna,
                // El Escudo no "dura" turnos, se consume por daño -- mismo
                // centinela (99) que ya usa el Golpe Provocador del Tanque
                // (ver combat.cpp) para una duracion que en la practica nunca
                // expira sola.
                /*dados=duracionTurnos*/ 99, /*caras=no usado*/ 0, /*bono=magnitud*/ 6,
                TipoEfecto::Escudo, /*apuntaAEnemigo=*/false};
}

Item Antidoto() {
    return Item{"Antidoto", "Combate: cura Aturdido y Veneno de un aliado.",
                TipoItem::Consumible, EfectoItem::CurarEstados, RanuraEquipo::Ninguna,
                0, 0, 0, TipoEfecto::Aturdido /*no usado con CurarEstados*/, /*apuntaAEnemigo=*/false};
}

Item MejoraAleatoria() {
    switch (Roll(4)) {
        case 1:  return PiedraDeFuerza();
        case 2:  return AmuletoDeProteccion();
        case 3:  return DagaVeloz();
        default: return TalismanDeVitalidad();
    }
}

// Uno de los 3 consumibles de combate al azar, mismo criterio de sorteo
// parejo que MejoraAleatoria (evita repetir el switch en ItemAleatorioDeCofre
// y TirarLootDeEnemigo).
Item ConsumibleDeCombateAleatorio() {
    switch (Roll(3)) {
        case 1:  return BombaDeVeneno();
        case 2:  return FrascoDeEscudo();
        default: return Antidoto();
    }
}

Item ItemAleatorioDeCofre() {
    int tirada = Roll(10);
    if (tirada <= 4) return PocionCuracionMenor();
    if (tirada <= 6) return ElixirDeEnergia();
    if (tirada <= 8) return ConsumibleDeCombateAleatorio();
    return MejoraAleatoria();
}

ResultadoUsoItem UsarItem(const Item& item, Character& objetivo) {
    ResultadoUsoItem r;
    // Las Mejoras se equipan (Character::Equipar / Inventory::Equipar), no
    // se "usan" instantaneo — si algo intenta usar una aca, es un error del
    // llamador, no una accion valida.
    if (item.tipo != TipoItem::Consumible) return r;

    Stats& stats = objetivo.GetStatsMut();
    char buffer[192];
    r.exitoso = true;

    switch (item.efecto) {
        case EfectoItem::CurarVida: {
            int tirada = item.dados > 0 ? RollDados(item.dados, item.caras, item.bono) : item.bono;
            int curado = AplicarCuracion(stats, tirada);
            r.valor = curado;
            std::snprintf(buffer, sizeof(buffer), "%s sobre %s: recupera %d de vida.",
                          item.nombre.c_str(), objetivo.Nombre().c_str(), curado);
            break;
        }
        case EfectoItem::CurarRecurso: {
            int tirada = item.dados > 0 ? RollDados(item.dados, item.caras, item.bono) : item.bono;
            int recuperado = std::min(tirada, stats.recursoMax - stats.recurso);
            if (recuperado < 0) recuperado = 0;
            stats.recurso += recuperado;
            r.valor = recuperado;
            std::snprintf(buffer, sizeof(buffer), "%s sobre %s: recupera %d de %s.",
                          item.nombre.c_str(), objetivo.Nombre().c_str(), recuperado,
                          NombreRecurso(objetivo.Rol()));
            break;
        }
        default:
            // No deberia pasar: un Consumible del catalogo siempre es
            // CurarVida o CurarRecurso. Si algun dia hay otro efecto de
            // Consumible, hace falta un caso nuevo aca.
            r.exitoso = false;
            return r;
    }
    r.texto = buffer;
    return r;
}

ResultadoLoot TirarLootDeEnemigo(TipoEnemigo tipo) {
    ResultadoLoot r;
    switch (tipo) {
        case TipoEnemigo::EsqueletoErrante:
            // 60% de soltar algo: casi siempre una pocion de curacion, a
            // veces un consumible de combate.
            if (Roll(10) <= 6) {
                r.hay = true;
                r.item = (Roll(5) == 1) ? ConsumibleDeCombateAleatorio() : PocionCuracionMenor();
            }
            break;
        case TipoEnemigo::RataGigante:
            // Es el mas debil de los tres: menos chance y solo un elixir chico.
            if (Roll(10) <= 4) { r.hay = true; r.item = ElixirDeEnergia(); }
            break;
        case TipoEnemigo::BanditoAturdidor:
            // El mas duro de los comunes: mas chance de soltar algo, y a
            // veces una mejora permanente o un consumible de combate en vez
            // de una pocion — recompensa el riesgo.
            if (Roll(10) <= 7) {
                r.hay = true;
                int tirada = Roll(4);
                if (tirada == 1) r.item = MejoraAleatoria();
                else if (tirada == 2) r.item = ConsumibleDeCombateAleatorio();
                else r.item = PocionCuracionMenor();
            }
            break;
        case TipoEnemigo::CapitanBandido:
            // El jefe: siempre suelta algo, y siempre una mejora permanente
            // (nunca un consumible) — es el premio grande de la run.
            r.hay = true;
            r.item = MejoraAleatoria();
            break;
    }
    return r;
}

} // namespace game
