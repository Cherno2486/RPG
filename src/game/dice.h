#pragma once

// Utilidades de dados para el combate (estilo D&D/BG3: tiradas de d20 para
// impactar, dados de daño tipo 1d6/1d8, etc).

namespace game {

// Tira un dado de 'caras' caras (1..caras).
int Roll(int caras);

// Tira 'cantidad' dados de 'caras' caras, los suma, y le agrega 'modificador'.
// Ej: RollDados(1, 6, 2) = 1d6+2.
int RollDados(int cantidad, int caras, int modificador = 0);

// Tira un d20 (1-20). Se usa para tiradas de ataque.
int RollD20();

} // namespace game
