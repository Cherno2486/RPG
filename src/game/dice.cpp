#include "dice.h"
#include <random>

namespace game {

namespace {
std::mt19937& Rng() {
    // thread_local para que sea segura si alguna vez se llama desde mas de un
    // hilo; se siembra una sola vez con una fuente de aleatoriedad real.
    thread_local std::mt19937 rng(std::random_device{}());
    return rng;
}
} // namespace

int Roll(int caras) {
    if (caras <= 0) return 0;
    std::uniform_int_distribution<int> dist(1, caras);
    return dist(Rng());
}

int RollDados(int cantidad, int caras, int modificador) {
    int total = modificador;
    for (int i = 0; i < cantidad; ++i) total += Roll(caras);
    return total;
}

int RollD20() {
    return Roll(20);
}

} // namespace game
