# Documento de diseño y arquitectura técnica — RPG de mazmorras (nombre provisional)

## Concepto

Un RPG con exploración de mazmorras y un sistema de party: el jugador controla a varios personajes a la vez, cada uno con un rol distinto (por ejemplo tanque, daño, soporte/curación, control), en la línea de referencias como Baldur's Gate 3 o Persona 5 Royal — combate táctico por turnos apoyado en la composición del grupo, más que en la habilidad de un solo personaje. El objetivo de esta primera etapa es un prototipo jugable en 2D, liviano, que sirva como base de diseño y de lógica de juego antes de portar el proyecto a Unreal Engine.

## Plataformas y stack técnico

El desarrollo arranca en VS Code, en C++, usando raylib como capa de gráficos/input/audio. La elección de C++ es deliberada: es el mismo lenguaje que usa Unreal Engine, así que gran parte de la lógica de juego (sistema de stats, IA de combate, generación de mazmorras, manejo del party) se puede migrar con cambios moderados en vez de reescribirse desde cero. raylib es liviana, no impone una arquitectura de motor completa, y tiene soporte multiplataforma para Windows, Linux y macOS, con soporte para Android (el build de iOS es más experimental y conviene evaluarlo más adelante, cuando se llegue a esa etapa).

El proyecto se arma con CMake, compilado con el toolchain GCC/MinGW-w64 (instalado vía Scoop, sin necesitar permisos de administrador ni Visual Studio completo). En VS Code se trabaja con las extensiones C/C++ y CMake Tools de Microsoft, más la extensión de Claude Code, todo desde la carpeta abierta del proyecto.

## Sistema de personajes y roles

Cada personaje del party tiene un rol que define su función táctica en combate y, en parte, su forma de interactuar con la mazmorra (por ejemplo, un rol de soporte podría detectar trampas, uno de tanque podría forzar aggro). Para la primera etapa conviene definir 3–4 roles base bien diferenciados en vez de un sistema de clases abierto, y ampliar después:

- **Tanque**: alta resistencia, habilidades para atraer/retener enemigos.
- **Daño (DPS)**: alto output ofensivo, recursos limitados (energía/maná/cooldowns).
- **Soporte/curación**: sanación, buffs, remoción de debuffs.
- **Control**: debuffs, aturdimiento, manejo de posicionamiento.

Cada personaje tiene stats base (vida, recursos, ataque, defensa, velocidad) más una o dos habilidades activas propias del rol y una pasiva. El jugador arma el party antes de entrar a una mazmorra y puede rotar quién está activo, similar a como funcionan los grupos de reserva en BG3 o Persona.

## Sistema de mazmorras

Mazmorras en 2D, vista cenital, basadas en grilla/tiles. Para el prototipo conviene generación semi-procedural: layouts armados a partir de "salas" prediseñadas (room templates) que se conectan proceduralmente, en vez de generación 100% procedural desde el vacío — es más controlable para el diseño de encuentros y más fácil de balancear al principio. Cada sala puede tener combate, un puzzle simple, un cofre/recurso, o ser puramente de conexión. Esto también deja la puerta abierta a mezclar salas armadas a mano con conexiones proceduralas cuando el sistema esté más maduro.

El movimiento del personaje dentro de la mazmorra es **libre/continuo** (no por grilla): el personaje se desplaza en cualquier ángulo, con colisión contra las paredes resuelta por rectángulos en vez de solo la celda destino. Se eligió así pensando en que se sienta más natural, sobre todo en mobile con un joystick virtual más adelante.

## Sistema de combate

Por turnos, con orden determinado por la velocidad de cada unidad (jugador y enemigos intercalados según stat de velocidad, no "todo el equipo primero"). Esto da profundidad táctica sin la complejidad de implementar combate en tiempo real con pausa.

Decisión de diseño (tomada al implementar el primer combate jugable): el combate es **basado en dados y efectos de estado, estilo Baldur's Gate 3 / D&D**, no en daño determinístico. Cada ataque tira un d20 + bono de ataque (derivado del stat de ataque) contra una "clase de defensa" del objetivo (10 + su stat de defensa): 1 natural es pifia automática, 20 natural es crítico (dobla los dados de daño). El daño en sí se tira con dados (1d6 para ataque básico, 1d8 para la habilidad del rol de Daño, con ventaja — 2d20, mejor de los dos). Encima de eso hay efectos de estado con duración en turnos: Aturdido (pierde el turno), Veneno (daño por turno), Escudo (absorbe daño antes que la vida), Debilitado (resta al bono de ataque) y Marcado (pensado para que los enemigos prioricen atacar al Tanque una vez que haya encuentros con más de un enemigo).

Cada rol tiene una habilidad propia (además del ataque básico, disponible siempre):
- **Tanque — Golpe Provocador**: ataque que además aplica Marcado.
- **Daño — Golpe Certero**: ataque con ventaja y más daño, cuesta recurso.
- **Soporte — Curar**: cura al aliado con menos vida, sin tirada (no falla), cuesta recurso.
- **Control**: pendiente — todavía no hay un personaje de este rol en el party de ejemplo.

El primer encuentro implementado es contra un único enemigo fijo en la mazmorra de prueba, para validar el ciclo completo (enganchar combate, elegir acciones, terminar en victoria o derrota) antes de construir generación de encuentros real.

## Arquitectura de código (pensando en la portabilidad a Unreal)

Para que la migración futura a Unreal sea lo más parecida a "portar lógica" y no "reescribir el juego", conviene separar desde el principio:

- **Capa de lógica de juego** (game layer): stats, sistema de turnos, IA de combate, inventario, generación de mazmorras, guardado, posición/colisión de entidades. Esta capa no debería depender directamente de raylib — trabaja con sus propias estructuras de datos y expone funciones/eventos.
- **Capa de presentación** (render layer): todo lo que sí depende de raylib — dibujar sprites, manejar input, reproducir audio, UI. Esta capa consume la capa de lógica, nunca al revés.

Esta separación es la diferencia entre "reemplazar raylib por Unreal en la capa de presentación" y "reescribir todo el juego". No hace falta una arquitectura perfecta desde el día uno, pero sí mantener esa frontera clara desde los primeros archivos.

## Estructura de carpetas

```
rpg-mazmorras/
├── CMakeLists.txt
├── external/            # raylib vía CMake FetchContent
├── src/
│   ├── main.cpp
│   ├── game/            # capa de lógica: independiente de raylib
│   │   ├── character.h/.cpp
│   │   ├── party.h/.cpp
│   │   ├── enemy.h/.cpp
│   │   ├── combat.h/.cpp        # resolución de ataques/habilidades, turnos
│   │   ├── combat_state.h/.cpp  # efectos activos, aplicar daño/curación
│   │   ├── effects.h/.cpp       # tipos de efecto de estado
│   │   ├── dice.h/.cpp          # tiradas de dados (d20, dN+mod)
│   │   ├── dungeon.h/.cpp
│   │   └── inventory.h/.cpp
│   └── render/           # capa de presentación: usa raylib
│       ├── renderer.h/.cpp
│       ├── input.h/.cpp
│       ├── ui.h/.cpp
│       └── combat_ui.h/.cpp
├── assets/
│   ├── sprites/
│   ├── audio/
│   └── tilesets/
└── docs/
    └── design.md
```

## Roadmap y estado actual

1. ✅ **Proyecto base**: ventana con raylib (1280x720), loop principal, grilla de tiles de referencia y panel con la party de ejemplo (Bruna/tanque, Kael/daño, Sara/soporte). Compilando y corriendo en Windows vía VS Code + CMake + GCC/MinGW.
2. ✅ **Movimiento**: personaje controlable con movimiento libre/continuo (no por grilla) dentro de una mazmorra de prueba (una sola sala), con colisión contra las paredes. Probado: direcciones, diagonales normalizadas, colisión de frente y en ángulo, todo OK.
3. ✅ **Sistema de party básico**: 3 personajes con stats y un rol cada uno. Un personaje "líder" controlado directamente, los otros dos siguiéndolo en formación (estilo tren/conga); UI mostrando HP/rol de cada uno.
4. ✅ **Combate por turnos contra un enemigo**: orden por velocidad, ataque básico + habilidad de rol, sistema de dados (d20 para impactar, dados de daño, ventaja, críticos) y efectos de estado (Aturdido, Veneno, Escudo, Debilitado, Marcado). Un enemigo de prueba fijo en la mazmorra para poder engancharlo y probar el ciclo completo.
5. Conectar los efectos que todavía no se usan (Veneno, Escudo, Debilitado) a más habilidades, y sumar la habilidad de Control (no hay personaje de ese rol todavía en el party de ejemplo).
6. Generación de mazmorra por salas conectadas (aunque sea con 3–4 room templates), y encuentros reales en vez del enemigo fijo de prueba — ahí es donde Marcado empieza a importar (varios enemigos a la vez).
7. Pantalla de derrota/game over como corresponde (hoy, perder el combate vuelve a la exploración igual que ganar).
8. Iterar sobre balance, UI de combate, y recién ahí evaluar el salto a mobile (build de Android vía NDK).

## Notas sobre la futura migración a Unreal

Cuando llegue el momento, lo que se traslada más directo es la capa de lógica de juego (si se mantuvo separada de raylib como se describe arriba): stats, reglas de combate, generación de mazmorras. Lo que se descarta o rehace por completo es la capa de presentación (sprites, tilemap 2D casero) — en Unreal eso pasa a resolverse con sus propios sistemas (Blueprints/C++, Niagara, el editor de niveles, etc.), y ahí también se decide si el salto es a 2D dentro de Unreal (Paper2D) o directamente a 3D.
