# RPG Mazmorras — prototipo

Prototipo 2D en C++ + raylib: exploración de mazmorras con un party de
personajes con roles (tanque / daño / soporte / control), combate táctico
por turnos. Primera etapa antes de portar el proyecto a Unreal Engine.

Ver `docs/design.md` para el documento de diseño completo (también vive en
el proyecto de Claude "RPG").

## Requisitos

- Compilador GCC (MinGW-w64), CMake y Git — instalados con
  `setup-herramientas.ps1` (usa Scoop, sin necesitar admin).
- VS Code con las extensiones **C/C++** y **CMake Tools** de Microsoft.

## Compilar y correr (línea de comandos, PowerShell)

```powershell
cmake -B build -G "MinGW Makefiles"
cmake --build build
.\build\rpg_mazmorras.exe
```

La primera vez que corras `cmake -B build ...` va a bajar y compilar raylib
automáticamente dentro de `external/` (puede tardar unos minutos). Las
siguientes veces es mucho más rápido.

## Compilar y correr (VS Code)

1. Abrir la carpeta del proyecto en VS Code.
2. Con la extensión CMake Tools, elegir el kit **GCC** cuando lo pida
   (abajo a la izquierda, o `Ctrl+Shift+P` → "CMake: Select a Kit").
3. `F7` para compilar, `Shift+F5` (o el botón de Run del CMake Tools) para
   correr.

## Controles del prototipo actual

**Exploración:**
- **WASD / flechas**: mover al líder del party (movimiento libre, no por
  grilla, con colisión contra las paredes). Los otros dos miembros siguen
  al líder en formación de fila.
- **E** (cerca del enemigo): engancha el combate.
- **TAB**: alterna el panel de party entre compacto (chiquito, no tapa el
  mapa) y expandido (con nombre/rol/HP detallado de cada uno).

**Combate:**
- **1**: ataque básico (1d6 + bono de ataque, tirada de d20 vs la defensa
  del objetivo).
- **2**: habilidad de rol del personaje en turno (distinta por rol — ver
  "Sistema de combate" abajo).
- Cualquier tecla, al terminar el combate (victoria o derrota): vuelve a
  la exploración.

## Sistema de combate (estilo BG3: dados + efectos)

Por turnos, con el orden decidido por la velocidad de cada unidad (se
calcula una sola vez al empezar el encuentro). Cada acción que ataca tira
un d20 + bono de ataque contra la "clase de defensa" del objetivo
(10 + su stat de defensa): 1 natural es pifia automática, 20 natural es
crítico automático (dobla los dados de daño). El daño baja la vida
directo, salvo que el objetivo tenga el efecto **Escudo** activo, que lo
absorbe primero.

Habilidad de rol (cuesta recurso, salvo la del Tanque):
- **Tanque — Golpe Provocador**: como un ataque básico, y si impacta
  aplica **Marcado** al enemigo (pensado para futuros encuentros con
  varios enemigos, donde va a hacer que prioricen atacarlo a él). Además,
  se cubre a sí mismo con **Escudo** al usarla (impacte o no).
- **Daño — Golpe Certero**: tira con ventaja (2d20, se queda con el mejor)
  y hace más daño (1d8 en vez de 1d6). Si sale crítico (20 natural),
  además aplica **Veneno** al enemigo.
- **Soporte — Curar**: sana 1d8+2 de vida al aliado con menos HP, sin
  tirada de ataque (no puede fallar).
- **Control — Grito Debilitante**: ataque menor (1d4) que, si impacta,
  aplica **Debilitado** al enemigo (resta a su bono de ataque mientras
  dura). El party de ejemplo ya incluye un personaje de este rol (Milo).

Efectos de estado ya soportados por el sistema (`game/effects.h`):
Aturdido (pierde el turno), Veneno (daño por turno), Escudo (absorbe
daño), Debilitado (resta al ataque) y Marcado (aggro). Los cuatro últimos
ya están conectados a habilidades reales del party; Aturdido está
implementado y testeado pero todavía sin ninguna fuente en el juego real
(pensado para un futuro enemigo o habilidad que lo aplique).

Hay un enemigo de prueba fijo en la mazmorra ("Esqueleto Errante") para
poder probar el combate — la generación de encuentros reales es un paso
futuro.

## Estado actual

1. ✅ Proyecto base: ventana raylib (1280x720), loop principal, grilla de
   tiles de referencia, panel de party de ejemplo (Bruna/tanque,
   Kael/daño, Sara/soporte, Milo/control).
2. ✅ Movimiento libre/continuo con colisión contra paredes (rectángulos).
3. ✅ Sistema de party básico: líder controlado, seguidores en formación,
   panel con HP/rol (compacto o expandido con TAB).
4. ✅ Combate por turnos contra un enemigo: orden por velocidad, ataque +
   habilidad por cada uno de los 4 roles, dados (d20 para impactar, dados
   de daño, ventaja, críticos) y los cinco efectos de estado, ya
   conectados a habilidades reales (Marcado y Escudo desde el Tanque,
   Veneno desde un crítico del Daño, Debilitado desde el Control).
5. Pendiente: más enemigos / encuentros — ahí Marcado empieza a importar
   de verdad, y tendría sentido que algún enemigo aplique Aturdido.
6. Pendiente: generación de mazmorra por salas conectadas.
7. Pendiente: pantalla de derrota/game over como corresponde (hoy, si
   pierde el party, se vuelve a la exploración igual que si se gana).
8. Pendiente: balance, y recién ahí evaluar build de Android.

Ver `docs/design.md` para el detalle completo de arquitectura y roadmap.

## Por qué está armado así

`src/game/` es la capa de lógica (stats, party, colisión, mazmorra) y **no
depende de raylib** — usa sus propios tipos (`game::Vec2`, `game::Rect`) a
propósito, para que el día que se porte a Unreal sea "portar lógica" y no
reescribir el juego entero. `src/render/` es la única capa que toca raylib
(ventana, dibujo, input, UI) y consume la capa de lógica, nunca al revés.

## Importante: hacé backup esta vez

La versión anterior de este proyecto se perdió al cambiar de compu porque
no estaba en un repositorio. Con `git` ya instalado por el script de setup,
conviene iniciar un repo ahora mismo:

```powershell
git init
git add .
git commit -m "Proyecto base: ventana, grilla, movimiento y party de ejemplo"
```

Y subirlo a GitHub (o similar) para tener el código a salvo de otro cambio
de compu.
