#pragma once
#include <vector>
#include "effects.h"

namespace game {

struct Stats;  // declarado en character.h; se usa aca solo por referencia/puntero

// Estado de combate que puede tener cualquier unidad (personaje del party o
// enemigo): la lista de efectos activos sobre ella, y las operaciones para
// manejarlos. Se guarda por separado de Character/Enemy para no repetir esta
// logica en cada clase.
class EstadoCombate {
public:
    void AgregarEfecto(EfectoActivo efecto);
    bool TieneEfecto(TipoEfecto tipo) const;
    int MagnitudEfecto(TipoEfecto tipo) const;  // 0 si no lo tiene
    void QuitarEfecto(TipoEfecto tipo);
    void LimpiarTodo();

    const std::vector<EfectoActivo>& Efectos() const { return efectos_; }

    struct ResultadoTick {
        bool aturdido = false;  // si esta aturdido, el portador pierde el turno
        int danoVeneno = 0;     // dano de veneno a aplicar este turno (0 si no tiene)
    };

    // Se llama al empezar el turno del portador: descuenta duracion a todos los
    // efectos, saca los que expiraron, y devuelve que hay que hacer este turno
    // (perder el turno por aturdimiento, aplicar dano de veneno).
    ResultadoTick TickInicioDeTurno();

private:
    std::vector<EfectoActivo> efectos_;
};

// Aplica 'dano' a 'stats', primero consumiendo el Escudo activo en 'estado' si
// lo tiene (y reduciendolo o quitandolo segun corresponda). Devuelve el dano
// que efectivamente le llego a la vida (para mostrar en el log de combate).
int AplicarDano(Stats& stats, EstadoCombate& estado, int dano);

// Cura 'cantidad' de vida a 'stats', sin pasarse del maximo. Devuelve la
// cantidad realmente curada.
int AplicarCuracion(Stats& stats, int cantidad);

} // namespace game
