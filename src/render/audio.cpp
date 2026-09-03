#include "audio.h"

namespace render {

namespace {
constexpr float kVolumenMusica = 0.5f;
constexpr float kVolumenSfx = 0.8f;
constexpr float kVolumenSfxCritico = 0.9f;
constexpr float kVolumenSfxFallo = 0.55f;
} // namespace

Audio::Audio() {
    InitAudioDevice();
    audioListo_ = IsAudioDeviceReady();
    if (!audioListo_) {
        // Sin dispositivo de audio disponible (por ejemplo, sin hardware de
        // sonido) el juego sigue andando en silencio en vez de crashear —
        // todos los metodos de esta clase chequean audioListo_ primero.
        TraceLog(LOG_WARNING, "AUDIO: no se pudo inicializar el dispositivo de audio; el juego sigue sin sonido.");
        return;
    }

    musicaExploracion_ = LoadMusicStream("assets/audio/musica_exploracion.wav");
    musicaCombate_ = LoadMusicStream("assets/audio/musica_combate.wav");
    musicaExploracion_.looping = true;
    musicaCombate_.looping = true;
    SetMusicVolume(musicaExploracion_, kVolumenMusica);
    SetMusicVolume(musicaCombate_, kVolumenMusica);

    sonidoGolpe_ = LoadSound("assets/audio/golpe.wav");
    sonidoCritico_ = LoadSound("assets/audio/critico.wav");
    sonidoCuracion_ = LoadSound("assets/audio/curacion.wav");
    sonidoFallo_ = LoadSound("assets/audio/fallo.wav");
    sonidoVictoria_ = LoadSound("assets/audio/victoria.wav");
    sonidoDerrota_ = LoadSound("assets/audio/derrota.wav");
    SetSoundVolume(sonidoGolpe_, kVolumenSfx);
    SetSoundVolume(sonidoCritico_, kVolumenSfxCritico);
    SetSoundVolume(sonidoCuracion_, kVolumenSfx);
    SetSoundVolume(sonidoFallo_, kVolumenSfxFallo);
    SetSoundVolume(sonidoVictoria_, kVolumenSfx);
    SetSoundVolume(sonidoDerrota_, kVolumenSfx);

    PlayMusicStream(musicaExploracion_);
    musicaDeCombate_ = false;
}

Audio::~Audio() {
    if (audioListo_) {
        UnloadMusicStream(musicaExploracion_);
        UnloadMusicStream(musicaCombate_);
        UnloadSound(sonidoGolpe_);
        UnloadSound(sonidoCritico_);
        UnloadSound(sonidoCuracion_);
        UnloadSound(sonidoFallo_);
        UnloadSound(sonidoVictoria_);
        UnloadSound(sonidoDerrota_);
        CloseAudioDevice();
    }
    // Si InitAudioDevice() habia fallado, no hay nada que descargar ni
    // dispositivo que cerrar (evita un warning de raylib sobre cerrar un
    // dispositivo que nunca se abrio).
}

void Audio::Actualizar(bool enCombate) {
    if (!audioListo_) return;
    if (enCombate != musicaDeCombate_) {
        StopMusicStream(musicaDeCombate_ ? musicaCombate_ : musicaExploracion_);
        PlayMusicStream(enCombate ? musicaCombate_ : musicaExploracion_);
        musicaDeCombate_ = enCombate;
    }
    UpdateMusicStream(musicaDeCombate_ ? musicaCombate_ : musicaExploracion_);
}

void Audio::ReiniciarCombate() {
    ultimaSecuenciaVista_ = -1;
}

void Audio::ProcesarEventos(const game::CombatEncounter& encuentro) {
    if (!audioListo_) return;
    int secuencia = encuentro.SecuenciaEventos();
    if (secuencia == ultimaSecuenciaVista_) return;
    ultimaSecuenciaVista_ = secuencia;

    for (const auto& ev : encuentro.UltimosEventos()) {
        switch (ev.tipo) {
            case game::TipoEventoVisual::Dano:
                PlaySound(ev.critico ? sonidoCritico_ : sonidoGolpe_);
                break;
            case game::TipoEventoVisual::Curacion:
                PlaySound(sonidoCuracion_);
                break;
            case game::TipoEventoVisual::Fallo:
                PlaySound(sonidoFallo_);
                break;
        }
    }
}

void Audio::ReproducirVictoria() {
    if (!audioListo_) return;
    PlaySound(sonidoVictoria_);
}

void Audio::ReproducirDerrota() {
    if (!audioListo_) return;
    PlaySound(sonidoDerrota_);
}

} // namespace render
