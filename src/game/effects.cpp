#include "effects.h"

namespace game {

const char* NombreEfecto(TipoEfecto tipo) {
    switch (tipo) {
        case TipoEfecto::Aturdido:   return "Aturdido";
        case TipoEfecto::Veneno:     return "Envenenado";
        case TipoEfecto::Escudo:     return "Escudo";
        case TipoEfecto::Debilitado: return "Debilitado";
        case TipoEfecto::Marcado:    return "Marcado";
    }
    return "?";
}

} // namespace game
