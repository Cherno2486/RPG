#include "party.h"

namespace game {

Party::Party(std::vector<Character> miembros) : miembros_(std::move(miembros)) {}

void Party::ActualizarFormacion(float /*deltaSeconds*/) {
    if (miembros_.empty()) return;

    // Se graba la posicion del lider cada frame para armar un rastro; cada
    // seguidor toma el punto del rastro que esta a su distancia objetivo
    // (i * kEspaciado), recorriendo el historial y acumulando distancia.
    historialLider_.push_front(Lider().Posicion());
    if ((int)historialLider_.size() > kMaxHistorial) historialLider_.pop_back();

    for (size_t i = 1; i < miembros_.size(); ++i) {
        float distanciaObjetivo = kEspaciado * static_cast<float>(i);

        float acumulado = 0.0f;
        Vec2 anterior = Lider().Posicion();
        Vec2 objetivo = anterior;
        for (const Vec2& punto : historialLider_) {
            acumulado += Length(punto - anterior);
            anterior = punto;
            objetivo = punto;
            if (acumulado >= distanciaObjetivo) break;
        }

        miembros_[i].SetPosicion(objetivo);
    }
}

} // namespace game
