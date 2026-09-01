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

- **WASD / flechas**: mover al líder del party (movimiento libre, no por
  grilla, con colisión contra las paredes).
- Los otros dos miembros del party siguen al líder en formación de fila.

## Estado actual

1. ✅ Proyecto base: ventana raylib (1280x720), loop principal, grilla de
   tiles de referencia, panel de party de ejemplo (Bruna/tanque,
   Kael/daño, Sara/soporte).
2. ✅ Movimiento libre/continuo con colisión contra paredes (rectángulos).
3. 🔧 En curso: sistema de party básico — líder controlado, dos
   seguidores seguidores en formación, panel con HP/rol.
4. Pendiente: combate por turnos mínimo (orden por velocidad).
5. Pendiente: generación de mazmorra por salas conectadas.
6. Pendiente: balance, UI de combate, evaluar build de Android.

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
