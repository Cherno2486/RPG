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
    return Item{"Elixir de Energia", "Restaura 1d6+4 de recurso.",
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

Item MejoraAleatoria() {
    switch (Roll(4)) {
        case 1:  return PiedraDeFuerza();
        case 2:  return AmuletoDeProteccion();
        case 3:  return DagaVeloz();
        default: return TalismanDeVitalidad();
    }
}

Item ItemAleatorioDeCofre() {
    int tirada = Roll(10);
    if (tirada <= 5) return PocionCuracionMenor();
    if (tirada <= 8) return ElixirDeEnergia();
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
            std::snprintf(buffer, sizeof(buffer), "%s sobre %s: recupera %d de recurso.",
                          item.nombre.c_str(), objetivo.Nombre().c_str(), recuperado);
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
            // 60% de soltar una pocion de curacion.
            if (Roll(10) <= 6) { r.hay = true; r.item = PocionCuracionMenor(); }
            break;
        case TipoEnemigo::RataGigante:
            // Es el mas debil de los tres: menos chance y solo un elixir chico.
            if (Roll(10) <= 4) { r.hay = true; r.item = ElixirDeEnergia(); }
            break;
        case TipoEnemigo::BanditoAturdidor:
            // El mas duro de los comunes: mas chance de soltar algo, y a
            // veces una mejora permanente en vez de un consumible —
            // recompensa el riesgo.
            if (Roll(10) <= 7) {
                r.hay = true;
                r.item = (Roll(4) == 1) ? MejoraAleatoria() : PocionCuracionMenor();
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
