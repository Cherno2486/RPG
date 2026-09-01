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
- Cualquier tecla, al ganar: vuelve a la exploración.
- Cualquier tecla, al perder (pantalla de **Game Over**): revive a todo el
  party a HP/recurso completo, sin efectos, y los manda de vuelta al punto
  de partida de la mazmorra.

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
daño), Debilitado (resta al ataque) y Marcado (aggro). Los cinco ya están
conectados a contenido real: los cuatro primeros desde habilidades del
party, y Aturdido desde el Golpe Aturdidor del Bandido Aturdidor.

Hay tres enemigos de prueba fijos en la mazmorra, de tipos distintos, para
poder probar variedad de combates — la generación de encuentros reales
(varios enemigos a la vez) es un paso futuro:
- **Esqueleto Errante**: el original, parejo, solo ataque básico.
- **Rata Gigante**: rápida y frágil (poca vida y defensa), ataque básico
  nomás — un combate corto y fácil.
- **Bandido Aturdidor**: más resistente y con más ataque; a veces, en vez
  de un golpe normal, usa **Golpe Aturdidor**, que si impacta aplica
  **Aturdido** (pierde el turno) — la primera fuente real de ese efecto en
  el juego.

Cada uno se engancha por separado (se acerca el líder y se aprieta E sobre
el más cercano); los otros dos quedan de fondo, sin participar, hasta que
se los engancha a ellos.

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
   Veneno desde un crítico del Daño, Debilitado desde el Control, Aturdido
   desde el Bandido Aturdidor).
5. ✅ Variedad de enemigos: tres tipos con stats e IA distintos (Esqueleto
   Errante, Rata Gigante, Bandido Aturdidor), repartidos por la mazmorra
   de prueba, cada uno enganchable por separado.
6. ✅ Pantalla de Game Over: al perder un combate, se ve una pantalla
   distinta a la de victoria y, al apretar una tecla, el party revive a
   full HP/recurso (sin efectos) y vuelve al punto de partida — antes,
   perder dejaba al party en HP 0 para siempre (softlock).
7. Pendiente: generación de mazmorra por salas conectadas y encuentros
   reales con varios enemigos a la vez — ahí Marcado empieza a importar
   de verdad.
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
