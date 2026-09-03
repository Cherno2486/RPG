#!/usr/bin/env python3
"""Genera los WAV de assets/audio/ de forma 100% procedural (sintesis con
numpy, sin samples ni musica con licencia de terceros) — son placeholders de
prototipo, pensados para reemplazarse mas adelante por assets definitivos
sin tocar el codigo (misma ruta/nombre de archivo, ver render/audio.h).

Correr desde la raiz del proyecto (o desde cualquier lado, las rutas de
salida son relativas a este script):

    python3 tools/generar_audio.py

Requiere numpy y scipy (solo para escribir el WAV) — no hace falta para
compilar ni correr el juego, solo para regenerar/ajustar estos sonidos.
"""
import os
import numpy as np
from scipy.io import wavfile

SR = 44100
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets", "audio")


def note(freq, dur, decay=8.0, harmonics=((1, 1.0), (2, 0.3), (3, 0.1)), sr=SR):
    """Un tono con caida exponencial (tipo 'pluck'/campana), sumando armonicos."""
    n = max(1, int(sr * dur))
    t = np.linspace(0.0, dur, n, endpoint=False)
    wave = np.zeros(n)
    for mult, amp in harmonics:
        wave += amp * np.sin(2.0 * np.pi * freq * mult * t)
    envelope = np.exp(-decay * t)
    return wave * envelope


def chirp(freq_inicial, freq_final, dur, decay=10.0, sr=SR):
    """Barrido de frecuencia lineal (usado para el 'whiff' de un fallo)."""
    n = max(1, int(sr * dur))
    t = np.linspace(0.0, dur, n, endpoint=False)
    freq_t = freq_inicial + (freq_final - freq_inicial) * (t / dur)
    fase = 2.0 * np.pi * np.cumsum(freq_t) / sr
    envelope = np.exp(-decay * t)
    return np.sin(fase) * envelope


def noise_burst(dur, decay=30.0, sr=SR, seed=None):
    """Ruido blanco con caida rapida — percusion/impacto sin tono definido."""
    n = max(1, int(sr * dur))
    rng = np.random.default_rng(seed)
    t = np.linspace(0.0, dur, n, endpoint=False)
    envelope = np.exp(-decay * t)
    return rng.uniform(-1.0, 1.0, n) * envelope


def swell(freqs_amps, dur, ataque_frac=0.25, release_frac=0.25, sr=SR):
    """Acorde sostenido con envolvente trapezoidal (fade in / sustain / fade
    out) — para pads de fondo. Termina cerca de 0, asi el loop no clickea."""
    n = max(1, int(sr * dur))
    t = np.linspace(0.0, dur, n, endpoint=False)
    wave = np.zeros(n)
    for freq, amp in freqs_amps:
        wave += amp * np.sin(2.0 * np.pi * freq * t)
    n_atk = max(1, int(n * ataque_frac))
    n_rel = max(1, int(n * release_frac))
    envelope = np.ones(n)
    envelope[:n_atk] = np.linspace(0.0, 1.0, n_atk)
    envelope[-n_rel:] = np.linspace(1.0, 0.0, n_rel)
    return wave * envelope


def mix(buffer, signal, start_sample=0):
    """Suma 'signal' dentro de 'buffer' en start_sample, recortando si se pasa."""
    fin = min(len(buffer), start_sample + len(signal))
    largo = fin - start_sample
    if largo > 0:
        buffer[start_sample:fin] += signal[:largo]


def fade_bordes(buffer, ms=15, sr=SR):
    """Fade in/out cortito en los extremos — red de seguridad contra clicks
    de loop por redondeo de punto flotante, no afecta el contenido musical."""
    n = int(sr * ms / 1000.0)
    n = min(n, len(buffer) // 4)
    if n <= 0:
        return buffer
    rampa = np.linspace(0.0, 1.0, n)
    buffer[:n] *= rampa
    buffer[-n:] *= rampa[::-1]
    return buffer


def normalizar(buffer, pico=0.9):
    maximo = np.max(np.abs(buffer))
    if maximo > 1e-9:
        buffer = buffer / maximo * pico
    return buffer


def guardar(nombre, buffer, sr=SR):
    buffer = np.clip(buffer, -1.0, 1.0)
    pcm16 = (buffer * 32767.0).astype(np.int16)
    ruta = os.path.join(OUT_DIR, nombre)
    wavfile.write(ruta, sr, pcm16)
    print(f"  {nombre}: {len(buffer) / sr:.2f}s")


# --- Efectos de combate ---

def generar_golpe():
    dur = 0.18
    buf = np.zeros(int(SR * dur))
    mix(buf, note(110, dur, decay=18, harmonics=((1, 1.0), (2, 0.2))))
    mix(buf, noise_burst(0.05, decay=60) * 0.5)
    guardar("golpe.wav", normalizar(buf, 0.9))


def generar_critico():
    dur = 0.26
    buf = np.zeros(int(SR * dur))
    mix(buf, note(90, dur, decay=14, harmonics=((1, 1.0), (2, 0.25))))
    mix(buf, noise_burst(0.08, decay=40) * 0.7)
    mix(buf, note(880, 0.15, decay=25, harmonics=((1, 0.6), (3, 0.3))), start_sample=int(SR * 0.02))
    guardar("critico.wav", normalizar(buf, 0.95))


def generar_curacion():
    dur = 0.55
    buf = np.zeros(int(SR * dur))
    notas = [523.25, 659.25, 783.99]  # C5, E5, G5
    for i, f in enumerate(notas):
        mix(buf, note(f, 0.3, decay=8, harmonics=((1, 1.0), (2, 0.25), (4, 0.1))),
            start_sample=int(SR * 0.11 * i))
    guardar("curacion.wav", normalizar(buf, 0.7))


def generar_fallo():
    dur = 0.2
    buf = chirp(300, 120, dur, decay=10)
    guardar("fallo.wav", normalizar(buf, 0.45))


def generar_victoria():
    dur = 1.5
    buf = np.zeros(int(SR * dur))
    arpegio = [523.25, 659.25, 783.99, 1046.50]  # C5 E5 G5 C6
    for i, f in enumerate(arpegio):
        mix(buf, note(f, 0.45, decay=6, harmonics=((1, 1.0), (2, 0.3), (3, 0.15))),
            start_sample=int(SR * 0.16 * i))
    acorde_final = [(1046.50, 0.6), (1318.51, 0.5), (1567.98, 0.5)]  # C6 E6 G6
    inicio_acorde = int(SR * 0.75)
    for f, amp in acorde_final:
        mix(buf, note(f, 0.75, decay=4, harmonics=((1, amp), (2, amp * 0.3))), start_sample=inicio_acorde)
    guardar("victoria.wav", normalizar(buf, 0.85))


def generar_derrota():
    dur = 1.4
    buf = np.zeros(int(SR * dur))
    notas = [440.0, 349.23, 293.66]  # A4 F4 D4
    for i, f in enumerate(notas):
        mix(buf, note(f, 0.4, decay=5, harmonics=((1, 1.0), (2, 0.2))), start_sample=int(SR * 0.22 * i))
        # segunda voz levemente desafinada -> "batido" lento, sensacion de derrota
        mix(buf, note(f * 0.995, 0.4, decay=5, harmonics=((1, 0.6),)), start_sample=int(SR * 0.22 * i))
    mix(buf, note(146.83, 0.8, decay=3, harmonics=((1, 1.0), (2, 0.15))), start_sample=int(SR * 0.66))  # D3
    guardar("derrota.wav", normalizar(buf, 0.8))


# --- Musica de fondo (loops) ---

def generar_musica_exploracion():
    T = 8.0
    n = int(SR * T)
    buf = np.zeros(n)

    # Pad de fondo: quinta abierta + octava, en Re menor, con un "swell"
    # lento que arranca y termina en 0 -> loop sin click, y de paso se siente
    # como una respiracion ambiental.
    pad = swell([(146.83, 0.35), (220.00, 0.25), (293.66, 0.18)], T, ataque_frac=0.22, release_frac=0.28)
    mix(buf, pad)

    # Melodia pulsada, una nota por segundo (Re menor natural), sube y baja.
    melodia = [293.66, 349.23, 440.00, 523.25, 440.00, 349.23, 293.66, None]
    for i, f in enumerate(melodia):
        if f is None:
            continue
        mix(buf, note(f, 0.65, decay=5, harmonics=((1, 1.0), (2, 0.25), (4, 0.08))) * 0.4,
            start_sample=int(SR * 1.0 * i))

    buf = normalizar(buf, 0.5)
    buf = fade_bordes(buf, ms=20)
    guardar("musica_exploracion.wav", buf)


def generar_musica_combate():
    T = 4.0
    n = int(SR * T)
    buf = np.zeros(n)

    # Bajo pulsado (ostinato), 8 golpes de 0.5s.
    for i in range(8):
        mix(buf, note(82.41, 0.3, decay=12, harmonics=((1, 1.0), (2, 0.3))) * 0.55,
            start_sample=int(SR * 0.5 * i))

    # Motivo de tension arriba, alternando dos notas (intervalo tenso).
    motivo = [440.0, 622.25]  # A4, D#5
    for i in range(8):
        f = motivo[i % 2]
        mix(buf, note(f, 0.28, decay=9, harmonics=((1, 1.0), (2, 0.2))) * 0.32,
            start_sample=int(SR * 0.5 * i) + int(SR * 0.05))

    # Percusion en cada pulso (kick+ruido).
    for i in range(8):
        golpe = noise_burst(0.09, decay=35) * 0.3
        mix(buf, golpe, start_sample=int(SR * 0.5 * i))

    buf = normalizar(buf, 0.55)
    buf = fade_bordes(buf, ms=15)
    guardar("musica_combate.wav", buf)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    print("Generando efectos de combate...")
    generar_golpe()
    generar_critico()
    generar_curacion()
    generar_fallo()
    generar_victoria()
    generar_derrota()
    print("Generando musica de fondo (loops)...")
    generar_musica_exploracion()
    generar_musica_combate()
    print("Listo.")


if __name__ == "__main__":
    main()
