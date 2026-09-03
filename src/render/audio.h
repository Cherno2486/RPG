#pragma once
#include "raylib.h"
#include "../game/combat.h"

namespace render {

// Audio del prototipo: musica de fondo (una pista para exploracion, otra
// mas tensa para combate, ambas en loop) y efectos de combate (golpe,
// critico, curacion, fallo, victoria, derrota). RAII, mismo patron que
// render::Renderer con la ventana: el constructor inicializa el dispositivo
// de audio y carga los assets de 'assets/audio/'; el destructor los
// descarga y cierra el dispositivo.
//
// Los assets son clips generados 100% proceduralmente (sintesis simple,
// ver tools/generar_audio.py) como placeholder de prototipo — no hay
// samples ni musica con licencia de terceros. Pensados para reemplazarse
// mas adelante por assets definitivos sin tocar el codigo (misma ruta y
// nombre de archivo).
//
// Si el dispositivo de audio no esta disponible (por ejemplo, corriendo
// sin hardware de sonido) el juego sigue andando sin sonido: todos los
// metodos son no-ops en ese caso, nunca crashean.
class Audio {
public:
    Audio();
    ~Audio();

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    // Llamar una vez por frame, siempre (este en exploracion o en combate):
    // hace avanzar el streaming de musica y cambia de pista si 'enCombate'
    // cambio desde el frame anterior.
    void Actualizar(bool enCombate);

    // Llamar una vez, justo despues de crear un game::CombatEncounter nuevo
    // (mismo momento que ui::ReiniciarFeedbackVisual, y por la misma razon:
    // sin esto, un combate nuevo que retoma un numero de secuencia ya visto
    // en el combate anterior podria perderse su primer efecto de sonido).
    void ReiniciarCombate();

    // Llamar una vez por frame mientras haya un combate en curso (mismo
    // patron de deteccion por SecuenciaEventos() que
    // ui::ActualizarFeedbackVisual en combat_ui.cpp): reproduce el efecto
    // de golpe/critico/curacion/fallo del paso mas reciente resuelto.
    void ProcesarEventos(const game::CombatEncounter& encuentro);

    // Efectos de cierre de combate — llamar una sola vez al entrar a
    // Ganado/Perdido (usar un flag "ya sonado", igual que lootRepartido en
    // main.cpp, para no repetirlo mientras la pantalla de fin sigue en
    // pantalla varios frames).
    void ReproducirVictoria();
    void ReproducirDerrota();

private:
    bool audioListo_ = false;

    Music musicaExploracion_{};
    Music musicaCombate_{};
    bool musicaDeCombate_ = false;

    Sound sonidoGolpe_{};
    Sound sonidoCritico_{};
    Sound sonidoCuracion_{};
    Sound sonidoFallo_{};
    Sound sonidoVictoria_{};
    Sound sonidoDerrota_{};

    int ultimaSecuenciaVista_ = -1;
};

} // namespace render
