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

Mazmorras en 2D, vista cenital, basadas en grilla/tiles. La generación es procedural por cadena de salas (`game::Dungeon`, en `src/game/dungeon.cpp`): en vez de una sola sala fija, cada partida arma una secuencia de 5 salas, cada una elegida al azar entre 4 "room templates" de tamaño (chica 8x8, grande 14x10, alargada 6x14, mediana 10x10), donde cada sala nueva se ubica pegada a la anterior extendiéndose al Este o al Sur (elegido al azar), conectada por un pasillo de 3 tiles de ancho centrado en el solape entre ambas. La sala 0 es siempre el punto de partida del party, sin enemigos; las otras 4 tienen contenido (un grupo de 1 a 3 enemigos de tipo aleatorio cada una — ver "Encuentros multi-enemigo" más abajo).

Las paredes se calculan de forma indirecta y robusta: primero se arma el conjunto completo de tiles de piso (unión de todas las salas más los pasillos que las conectan), y recién al final cualquier tile dentro del área total ocupada que no sea piso se convierte en una pared (un `Rect` de 1 tile por celda — más pared de la estrictamente necesaria comparado con fusionar rects, pero mucho más simple y difícil de romper a esta escala). Esto evita calcular "aberturas" a mano donde un pasillo conecta con una sala: al ser piso de los dos lados, ya no queda pared en el medio.

Como el layout generado es más grande que la ventana (1280x720), la capa de presentación agrega una cámara (`Camera2D` en `render/renderer.cpp`) que sigue al líder del party; la grilla de referencia se dibuja sólo dentro de los límites de cada sala, no en los pasillos ni fuera del layout ocupado.

El movimiento del personaje dentro de la mazmorra sigue siendo **libre/continuo** (no por grilla): el personaje se desplaza en cualquier ángulo, con colisión contra las paredes resuelta por rectángulos (eje por eje) en vez de solo la celda destino. Se eligió así pensando en que se sienta más natural, sobre todo en mobile con un joystick virtual más adelante.

Este generador procedural fue validado con fuzz-testing: un test standalone (`test_game_layer.cpp`, fuera del build de raylib) construye muchas mazmorras con semillas al azar y verifica que el centro de cada sala generada quede en piso transitable (no solapado con una pared) y que la colisión contra la pared izquierda de la sala inicial se comporte de forma consistente. Un hallazgo de ese proceso: con el movimiento en pasos discretos (velocidad × delta-tiempo por frame), la posición final contra una pared puede quedar hasta un paso entero más allá del borde exacto — no es un bug de colisión ni de generación, así que el test tolera ese margen en vez de exigir precisión sub-píxel.

## Sistema de combate

Por turnos, con orden determinado por la velocidad de cada unidad (jugador y enemigos intercalados según stat de velocidad, no "todo el equipo primero"). Esto da profundidad táctica sin la complejidad de implementar combate en tiempo real con pausa.

Decisión de diseño (tomada al implementar el primer combate jugable): el combate es **basado en dados y efectos de estado, estilo Baldur's Gate 3 / D&D**, no en daño determinístico. Cada ataque tira un d20 + bono de ataque (derivado del stat de ataque) contra una "clase de defensa" del objetivo (10 + su stat de defensa): 1 natural es pifia automática, 20 natural es crítico (dobla los dados de daño). El daño en sí se tira con dados (1d6 para ataque básico, 1d8 para la habilidad del rol de Daño, con ventaja — 2d20, mejor de los dos). Encima de eso hay efectos de estado con duración en turnos: Aturdido (pierde el turno), Veneno (daño por turno), Escudo (absorbe daño antes que la vida), Debilitado (resta al bono de ataque) y Marcado.

Marcado ahora tiene un efecto observable de verdad: en `CombatEncounter::Actualizar` (la resolución del turno de un enemigo), si el enemigo en turno tiene el efecto Marcado activo, su IA prioriza atacar al miembro del party con `Role::Tanque` (si sigue con vida) en vez de al aliado con menos HP, que es el criterio default. Antes de los encuentros multi-enemigo esto era código sin efecto observable: con un único enemigo posible en pantalla, "a quién prioriza atacar" nunca se notaba porque solo había un objetivo posible.

Cada rol tiene una habilidad propia (además del ataque básico, disponible siempre):
- **Tanque — Golpe Provocador**: ataque que además aplica Marcado al enemigo, y le da Escudo a si mismo (se cubre mientras provoca) — conecta Marcado y Escudo al mismo tiempo.
- **Daño — Golpe Certero**: ataque con ventaja y más daño (1d8), cuesta recurso; si el golpe es crítico (20 natural), además aplica Veneno al enemigo (la herida sangra).
- **Soporte — Curar**: cura al aliado con menos vida, sin tirada (no falla), cuesta recurso.
- **Control — Grito Debilitante**: ataque menor (1d4) que si impacta aplica Debilitado al enemigo (resta a su bono de ataque mientras dura), cuesta recurso. Ya hay un personaje de este rol en el party de ejemplo (Milo).

Con esto los cinco efectos de estado (Aturdido, Veneno, Escudo, Debilitado, Marcado) están conectados a contenido real del juego. Aturdido en particular no viene de ninguna habilidad del party, sino de un enemigo: el Bandido Aturdidor (ver más abajo) a veces usa "Golpe Aturdidor" en vez de un ataque normal.

Hay tres tipos de enemigo comunes, que pueblan las salas intermedias de la mazmorra generada, más un jefe único en la última sala (ver "Jefe de mazmorra" más abajo):
- **Esqueleto Errante**: el original, stats parejas, solo ataque básico.
- **Rata Gigante**: rápida (mayor velocidad que todo el party) y frágil (poca vida y defensa) — un combate corto pensado como el más fácil de los tres.
- **Bandido Aturdidor**: más vida y defensa que los otros dos; en su turno, con 50% de probabilidad, usa "Golpe Aturdidor" (daño menor, 1d4) en vez del ataque básico, y si impacta aplica Aturdido a quien golpeó. Ver "Balance: ajuste del Bandido Aturdidor" más abajo por qué su ataque quedó igualado al del Esqueleto en vez de por encima.

### Encuentros multi-enemigo

Cada sala intermedia con contenido tiene un **grupo de 1 a 3 enemigos** (`main.cpp::CrearGrupoDeSala`), de tipos elegidos al azar entre los tres de arriba — pueden repetirse; si hay más de uno del mismo tipo en la sala, se le agrega un sufijo al nombre (" II", " III") para distinguirlos en el log y en las fichas de combate. Cada `Enemy` sabe a qué sala pertenece (`Enemy::Sala()`), asignado al crearlo.

### Jefe de mazmorra: Capitán Bandido

La última sala con contenido (`main.cpp::main`, `indiceSalaJefe = salas.size() - 1`) no tiene un grupo aleatorio: tiene un único **Capitán Bandido** (`TipoEnemigo::CapitanBandido`), el jefe de la run. Se distingue de los enemigos comunes en varios planos:

- **Stats**: el más resistente de largo (52 HP contra 24 del Bandido común), con ataque y defensa también por encima de cualquier enemigo regular.
- **IA propia** (`CombatEncounter::Actualizar` en `combat.cpp`): en vez de un patrón fijo, alterna entre tres opciones cada turno — ataque básico (30%), "Golpe Aturdidor" (30%, igual que el Bandido común: menos daño pero aplica Aturdido si impacta) y **"Doble Tajo"** (40%: dos ataques básicos en el mismo turno, re-eligiendo objetivo para el segundo golpe por si el primero se llevó puesto a quien tenía en la mira). Por debajo del **40% de HP entra en furia**: deja de aturdir y usa Doble Tajo siempre — la pelea se pone más agresiva justo en el tramo final, en vez de ir apagándose a medida que pierde vida como cualquier otro enemigo.
- **Visual**: círculo notablemente más grande, casi negro, con un anillo doble dorado (`ColorDeEnemigo`/`RadioDeEnemigo` en `renderer.cpp`) — se distingue a simple vista apenas se lo ve en el mapa, sin depender de leer el nombre.
- **Botín garantizado** (`TirarLootDeEnemigo` en `item.cpp`): a diferencia de los enemigos comunes (que sueltan botín con cierta probabilidad, y no siempre una mejora), el jefe **siempre** suelta una mejora permanente (Piedra de Fuerza o Amuleto de Protección al azar) — el premio grande de haber llegado hasta el final.
- **Cierre de la run**: derrotarlo dispara una pantalla de victoria distinta ("¡MAZMORRA DESPEJADA!", en `combat_ui.cpp`, detectada por ser el único enemigo del encuentro y de tipo `CapitanBandido`) en vez del cartel genérico de "¡VICTORIA!" — la mazmorra, hasta ahora, no tenía ningún cierre más allá de "no queda nada más que hacer".

Simulado con el mismo harness Monte Carlo del balance general (`test_balance.cpp`): solo y con party fresco, el Capitán Bandido se gana un 99.3% de las veces con ~81.5% de HP promedio restante (17 turnos en promedio) — comparable o algo más manejable que 3x Bandido Aturdidor (94.7%/72.9%), porque enfrentarlo solo, sin dividir el daño del party entre varios objetivos, compensa el golpe extra de Doble Tajo. La cifra que importa es la de la run completa hasta este jefe (ver más abajo): con el sistema de items activo, la mazmorra completa (4 salas + jefe) se termina un 87.5% de las veces (vs. 91.9% antes de agregar el jefe) — una dificultad final notoria pero ampliamente superable, en línea con la filosofía "duro pero justo" del resto del balance.

Al enganchar combate (tecla E cerca de cualquier enemigo vivo), `main.cpp` reúne a TODOS los enemigos vivos con la misma `Sala()` que el más cercano y arma un único `CombatEncounter` con ese grupo — un combate por sala, no por enemigo individual. `CombatEncounter` pasó de guardar una referencia a un solo `Enemy` a guardar `std::vector<Enemy*>`; el orden de turnos ahora intercala a los 4 del party con TODOS los enemigos del grupo según velocidad (antes era "los 4 del party + 1 enemigo"), y la victoria (`FaseCombate::Ganado`) requiere que **todos** los enemigos del grupo estén derrotados, no solo el primero.

Como puede haber más de un enemigo vivo a la vez, las acciones del aliado en turno (ataque básico, habilidad de rol) necesitan saber a cuál de ellos apuntar: `CombatEncounter` mantiene un `objetivoActual_` (índice dentro del grupo), que arranca en el primer enemigo vivo y se recalcula solo si el objetivo actual muere (`AsegurarObjetivoValido`). El jugador puede cambiarlo manualmente con **TAB** durante su turno (`CiclarObjetivo`), y la UI de combate (`combat_ui.cpp`) marca la ficha del objetivo actual con un borde dorado y "▶" delante del nombre, separado del resaltado rojo que indica de quién es el turno.

El primer encuentro implementado (antes de la mazmorra procedural) fue contra un único enemigo fijo, para validar el ciclo completo (enganchar combate, elegir acciones, terminar en victoria o derrota) antes de construir generación de mazmorra y de encuentros reales; ese camino sigue intacto (`CombatEncounter` con un vector de un solo elemento funciona igual que antes), pero ahora es el caso particular de un grupo de tamaño 1, no un tipo de dato distinto.

### Qué pasa al ganar o perder

Al ganar (`FaseCombate::Ganado`), se vuelve a la exploración tal cual — el party sigue con el HP/recurso que le quedó, y el enemigo derrotado queda marcado (`Enemy::Vencido()`) y no se puede volver a enganchar.

Al perder (`FaseCombate::Perdido`) se ve una pantalla de Game Over distinta a la de victoria, y al apretar una tecla el party **revive**: `Character::Revivir()` restaura HP y recurso al máximo y limpia todos los efectos de combate, y `Party::ReiniciarFormacion()` teletransporta a todo el party de vuelta al punto de partida de la mazmorra (y limpia el rastro de formación, para que los seguidores no "corran" desde el rastro viejo). Sin esto, perder dejaba al party con HP 0 para siempre — el próximo combate terminaba en derrota instantánea sin que el jugador pudiera hacer nada (softlock).

### Sistema de inventario y loot

Inventario único y compartido por todo el party (`game::Inventory`, dueño: `game::Party`), no uno por personaje — evita tener que decidir "a quién le doy este item" al momento de recogerlo, esa decisión se toma recién al usarlo. Apila items iguales en una sola entrada con cantidad (`PilaItem`, comparado por nombre) en vez de ocupar un slot por unidad.

Cada `game::Item` (catálogo fijo en `item.h`/`item.cpp`, definición del struct en `item_types.h`) es de uno de dos tipos:
- **Consumible** (`EfectoItem::CurarVida` / `CurarRecurso`), resuelto por `UsarItem`: tira dados (`RollDados(dados, caras, bono)`) y aplica el resultado a HP o recurso, clampeado al máximo (`AplicarCuracion` para vida; un `std::min` manual para recurso, que no tiene una función compartida como la curación). Se gasta una unidad de la pila al usarse (`Inventory::Usar`).
- **Mejora** (`MejorarAtaque` / `MejorarDefensa`): suma `bono` directo a la stat correspondiente, **permanente**. A diferencia de los Consumibles, **no se "usa" instantáneo — se equipa** (`Character::Equipar` / `Inventory::Equipar`), ver la subsección de abajo. `UsarItem` rechaza explícitamente cualquier item que no sea `TipoItem::Consumible` (devuelve `exitoso=false` sin tocar stats), como red de seguridad si algo lo llama por error con una Mejora.

`ResultadoUsoItem` devuelve si se pudo usar, el valor numérico aplicado y un texto ya formateado (p.ej. "Poción de Curación Menor sobre Milo: recupera 6 de vida.") pensado para mostrarse directo como mensaje flotante, sin que la capa de render tenga que reconstruir la frase.

Dos fuentes de items:
- **Cofres** (`game::Cofre`: posición + `Item` fijo + `abierto`): objetos estáticos ubicados por `main.cpp` en la generación de la mazmorra — uno garantizado en la sala inicial (para que el sistema se vea sin depender del azar) y, por cada sala con contenido, un 40% (`Roll(10) <= kChanceCofrePorSalaDe10`) de tener uno adicional, en la esquina de la sala (lejos del grupo de enemigos, que suele estar cerca del centro). `ItemAleatorioDeCofre()` decide el contenido con una tabla de probabilidad fija (50% poción, 30% elixir, 20% una mejora al azar).
- **Botín de combate** (`TirarLootDeEnemigo(TipoEnemigo)`): al entrar en `FaseCombate::Ganado`, `main.cpp` tira una vez por cada enemigo que participó del encuentro (`CombatEncounter::Enemigos()`), con una tabla de drop por tipo — Esqueleto Errante 60%, Rata Gigante 40% (el más débil, tabla más floja a propósito), Bandido Aturdidor 70% con 1/4 de esas veces siendo una mejora permanente en vez de un consumible (recompensa extra por ser el enemigo más duro de pelear). Un flag `lootRepartido` en `main()` asegura que el botín se reparta una sola vez por combate, ya que `Ganado` se re-evalúa todos los frames hasta que el jugador aprieta una tecla.

**Interacción unificada con [E]**: antes, `main.cpp` solo buscaba el enemigo vivo más cercano; ahora busca el interactuable más cercano entre enemigos vivos y cofres sin abrir (`kDistanciaInteraccion`, el mismo radio de antes), y el cartel de abajo ("[E] Atacar" / "[E] Abrir cofre") depende de cuál sea. Un enemigo cercano siempre gana la prioridad sobre un cofre a la misma distancia — no debería haber ambigüedad real en la práctica, ya que los cofres se ubican lejos de los grupos de enemigos a propósito.

**Ranuras de equipo** (`RanuraEquipo` en `item_types.h`; `Character::Equipar`/`Arma()`/`Accesorio()` en `character.h`/`.cpp`): cada `Item` de tipo Mejora declara a qué ranura va (`Arma` para `MejorarAtaque`, `Accesorio` para `MejorarDefensa`), y cada `Character` tiene como mucho un `ItemEquipado` por ranura (`{bool ocupado; Item item;}`). El motivo del cambio: antes una Mejora se aplicaba directo y se perdía como concepto (no había forma de ver qué tenía puesto un personaje, ni nada que impidiera "equiparle 5 Piedras de Fuerza" al mismo, cada una sumando +1 de ataque sin límite). `Character::Equipar(nuevo)` resuelve la ranura según `nuevo.ranura`, si ya había algo puesto le revierte el `bono` a la stat (resta exacta, no un reset a un valor base — no hace falta separar "stats base" de "stats con equipo" porque el `bono` que se sumó es el mismo que se resta), aplica el `bono` del nuevo, dejarlo equipado, y devuelve el `ItemEquipado` anterior (con `ocupado=false` si la ranura estaba vacía) para que el llamador decida qué hacer con él. `Inventory::Equipar(indice, personaje)` es ese llamador: saca una unidad de la pila, llama a `Character::Equipar`, y si vuelve algo ocupado lo reinserta con `Agregar` — así lo reemplazado no se pierde, vuelve al inventario compartido. `Inventory::Usar` e `Inventory::Equipar` se rechazan mutuamente por tipo (`Usar` solo actúa sobre Consumibles, `Equipar` solo sobre Mejoras), y `main.cpp` decide cuál llamar mirando `pilas[indice].item.tipo` antes de procesar la tecla numérica.

**Pantalla de inventario** (**I** la abre/cierra, `render/inventory_ui.cpp`): reemplaza por completo el frame de exploración mientras está abierta (congela movimiento e interacción — `main.cpp` solo procesa TAB/números en ese estado). Muestra una ficha por miembro del party (nombre, rol por color, HP/recurso, Arma/Accesorio equipados — "-" si la ranura está vacía —, y un borde dorado + "<" en quien es el objetivo actual) y la lista de pilas de items con un `[N]` al lado de cada una; las de tipo Mejora llevan además un tag `[Equipar: Arma]` / `[Equipar: Accesorio]` en dorado, para que quede claro que van a reemplazar lo que haya en esa ranura en vez de sumarse sin límite. **TAB** cicla el objetivo entre **todos** los miembros, vivos o no — a propósito, para poder curar/revivir a un caído sin tener que cerrar el inventario y reordenar nada. **1-9** llama a `Inventory::Usar` o `Inventory::Equipar` según el tipo del item en esa fila. El panel de party expandido (`render/ui.cpp::DibujarPanelExpandido`, TAB en exploración) también muestra el Arma/Accesorio de cada uno en una línea aparte, para poder chusmear el equipo sin abrir el inventario.

**Mensajes flotantes**: un `std::string mensajeFlotante` + `float timerMensaje` en `main()` (no en el renderer — la capa de lógica decide el texto, `renderer.cpp` solo lo dibuja) muestran por `kDuracionMensaje` (3s) el resultado de abrir un cofre, usar/equipar un item o el resumen de botín tras un combate, en un cartel arriba del prompt de interacción.

## Arquitectura de código (pensando en la portabilidad a Unreal)

Para que la migración futura a Unreal sea lo más parecida a "portar lógica" y no "reescribir el juego", conviene separar desde el principio:

- **Capa de lógica de juego** (game layer): stats, sistema de turnos, IA de combate, inventario, generación de mazmorras, guardado, posición/colisión de entidades. Esta capa no debería depender directamente de raylib — trabaja con sus propias estructuras de datos y expone funciones/eventos.
- **Capa de presentación** (render layer): todo lo que sí depende de raylib — dibujar sprites, manejar input, reproducir audio, UI, la cámara. Esta capa consume la capa de lógica, nunca al revés.

Esta separación es la diferencia entre "reemplazar raylib por Unreal en la capa de presentación" y "reescribir todo el juego". No hace falta una arquitectura perfecta desde el día uno, pero sí mantener esa frontera clara desde los primeros archivos — la generación de mazmorra procedural, por ejemplo, vive enteramente en `game/dungeon.cpp` sin tocar raylib; lo único que la capa de render agrega encima es la cámara que sigue al líder.

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
│   │   ├── dungeon.h/.cpp       # generación procedural por cadena de salas
│   │   ├── item_types.h         # Item, TipoItem, EfectoItem, RanuraEquipo (sin depender de item.h)
│   │   ├── item.h/.cpp          # catálogo de items, UsarItem, loot de enemigos, Cofre
│   │   └── inventory.h/.cpp     # Inventory: pilas de items, Usar (Consumibles) / Equipar (Mejoras)
│   └── render/           # capa de presentación: usa raylib
│       ├── renderer.h/.cpp      # incluye la Camera2D que sigue al líder
│       ├── input.h/.cpp
│       ├── ui.h/.cpp
│       ├── combat_ui.h/.cpp
│       └── inventory_ui.h/.cpp  # pantalla de inventario (tecla I)
├── assets/
│   ├── sprites/
│   ├── audio/
│   └── tilesets/
└── docs/
    └── design.md
```

### Balance: ajuste del Bandido Aturdidor

Para iterar sobre balance general se armó un simulador Monte Carlo standalone (`test_balance.cpp`, no se shippea) que corre combates completos usando las clases reales del juego (`Party`, `Enemy`, `CombatEncounter`) con una IA heurística del lado del jugador (foco en el enemigo con menos HP, uso de habilidad según rol y recurso disponible), en vez de calcular probabilidades a mano. Esto permite medir tasa de victoria, HP restante promedio y duración promedio de cada combinación de enemigos con miles de repeticiones.

La corrida inicial (N=3000-4000 por escenario) mostró que los tres tipos de enemigo estaban parejos en solitario y en grupos de a dos, pero **3x Bandido Aturdidor** era un caso claramente fuera de línea frente a 3x de cualquier otro tipo:

| Escenario | Victorias (antes) | HP del party al ganar (antes) |
|---|---|---|
| 3x Esqueleto Errante | 99.1-99.6% | 78.5-78.8% |
| 3x Rata Gigante | 100% | 87.7-88.0% |
| 3x Bandido Aturdidor | 76-79% | ~59% |

Se probaron dos hipótesis por separado, cambiando una sola variable a la vez en el simulador:
1. **Frecuencia de "Golpe Aturdidor"** (de 50% a ~33%, y luego prácticamente desactivada): la tasa de victoria subió de 76.3% a un máximo de 84.1%, con el HP restante casi sin moverse (~59%). Conclusión: el aturdimiento aporta, pero no es la causa principal.
2. **Stats base** (`ataque` 8→7, `hp`/`hpMax` 26→24, dejando el aturdimiento intacto): la tasa de victoria subió a 94.7% y el HP restante a 72.7% — muy cerca del 99.5%/78.5% del Esqueleto. Conclusión: el desgaste acumulado de tener más HP y más ataque que el Esqueleto durante una pelea de 3 enemigos (que dura ~25 turnos en vez de ~21) era el factor dominante, no la habilidad especial.

Cambio aplicado (`main.cpp::CrearEnemigoDeTipo`, caso `BanditoAturdidor`): `ataque` 8→7 (ahora igual al Esqueleto) y `hpMax`/`hp` 26→24 (sigue siendo el más resistente de los tres). La defensa (4, la más alta) y el "Golpe Aturdidor" quedan sin tocar — sigue siendo el enemigo "tanque con aturdimiento", solo que ya no es desproporcionadamente más duro en grupos de tres. 1x y 2x Bandido no se resienten (ya estaban en 100%/99.6-100% de victorias y quedan igual o mejor).

De paso, la simulación de una mazmorra completa (4 salas, sin curación entre combates, peor caso) dio 71-72% de clears sin usar el sistema de inventario y 91-92% modelando cofres y botín — un dato a favor de que el sistema de loot ya cumple su rol de red de contención, no algo que haya hecho falta tocar en esta pasada.

## Roadmap y estado actual

1. ✅ **Proyecto base**: ventana con raylib (1280x720), loop principal, grilla de tiles de referencia y panel con la party de ejemplo (Bruna/tanque, Kael/daño, Sara/soporte, Milo/control). Compilando y corriendo en Windows vía VS Code + CMake + GCC/MinGW.
2. ✅ **Movimiento**: personaje controlable con movimiento libre/continuo (no por grilla), con colisión contra las paredes. Probado: direcciones, diagonales normalizadas, colisión de frente y en ángulo, todo OK.
3. ✅ **Sistema de party básico**: 4 personajes con stats y un rol cada uno (uno por rol). Un personaje "líder" controlado directamente, los demás siguiéndolo en formación (estilo tren/conga); UI mostrando HP/rol de cada uno, con modo compacto/expandido (TAB) para no tapar el mapa.
4. ✅ **Combate por turnos contra un enemigo**: orden por velocidad, ataque básico + habilidad de rol (una por cada uno de los 4 roles, Control incluido), sistema de dados (d20 para impactar, dados de daño, ventaja, críticos) y los cinco efectos de estado (Aturdido, Veneno, Escudo, Debilitado, Marcado) conectados a contenido real, tanto del party como de un enemigo.
5. ✅ **Variedad de enemigos**: tres tipos con stats e IA distintos (Esqueleto Errante, Rata Gigante, Bandido Aturdidor).
6. ✅ **Pantalla de Game Over**: perder ya no deja al party en HP 0 para siempre — se ve una pantalla distinta a la de victoria y, al apretar una tecla, el party revive a full HP/recurso (sin efectos) y vuelve al punto de partida.
7. ✅ **Generación de mazmorra por salas conectadas**: cadena de 5 salas (4 room templates de tamaño, extensión Este/Sur al azar, pasillos de conexión), paredes calculadas por diferencia contra el piso, cámara que sigue al líder para navegar el layout completo.
8. ✅ **Encuentros multi-enemigo**: cada sala con contenido tiene un grupo de 1 a 3 enemigos que se engancha entero en un solo `CombatEncounter` (turnos intercalados entre todo el party y todos los enemigos vivos, selector de objetivo con TAB, victoria solo cuando cae todo el grupo). Marcado pasó de ser un efecto sin consecuencia observable a hacer que el enemigo marcado priorice atacar al Tanque.
9. ✅ **Inventario y loot**: catálogo de items (pociones de vida, elixires de recurso, mejoras permanentes de ataque/defensa), inventario único compartido por todo el party con apilado, cofres en la mazmorra (uno garantizado + chance por sala), tablas de botín por tipo de enemigo al ganar un combate, e interacción [E] unificada (enemigo o cofre, el que esté más cerca) más una pantalla de inventario ([I], selector de objetivo con TAB, uso con 1-9) — ver "Sistema de inventario y loot" arriba.
10. ✅ **Ranuras de equipo**: las mejoras permanentes dejaron de aplicarse instantáneo y perderse como concepto — ahora se equipan (`Character::Equipar`) en una ranura de Arma o Accesorio por personaje, una sola de cada, visible en el inventario y en el panel de party expandido; equipar algo nuevo en una ranura ocupada reemplaza lo que había y lo devuelve al inventario compartido en vez de acumular el bono sin límite. Surgió como feedback directo: nada impedía antes ponerle la misma mejora varias veces al mismo personaje, ni había forma de ver qué tenía puesto cada uno — ver "Ranuras de equipo" arriba.
11. ✅ **Balance general (primera pasada)**: simulador Monte Carlo (`test_balance.cpp`) sobre las clases reales de combate detectó que los grupos de 3x Bandido Aturdidor eran notablemente más duros que cualquier otro grupo de 3 (76-79% de victorias y ~59% de HP restante vs. 99-100%/78-88% del resto); se corrigió ajustando sus stats base (`ataque` 8→7, HP 26→24) — ver "Balance: ajuste del Bandido Aturdidor" arriba.
12. ✅ **Jefe de mazmorra**: la última sala con contenido ya no tiene un grupo aleatorio más — tiene al Capitán Bandido, un enemigo único con más stats que cualquier otro, IA propia ("Doble Tajo" y furia por debajo del 40% de HP), botín garantizado y una pantalla de cierre distinta ("¡MAZMORRA DESPEJADA!") — ver "Jefe de mazmorra: Capitán Bandido" arriba. Balanceado con el mismo simulador: 87.5% de clears totales con el sistema de items activo (vs. 91.9% antes de agregar el jefe), una caída de dificultad esperable y buscada para el cierre de la run.
13. Pendiente: seguir iterando sobre contenido (más tipos de enemigo comunes, más variedad de salas o una mazmorra más larga) y sobre balance (curva de poder con mejoras permanentes acumuladas a lo largo de una run completa) antes de evaluar el salto a mobile (build de Android vía NDK).

## Notas sobre la futura migración a Unreal

Cuando llegue el momento, lo que se traslada más directo es la capa de lógica de juego (si se mantuvo separada de raylib como se describe arriba): stats, reglas de combate, generación de mazmorras. Lo que se descarta o rehace por completo es la capa de presentación (sprites, tilemap 2D casero, la cámara) — en Unreal eso pasa a resolverse con sus propios sistemas (Blueprints/C++, Niagara, el editor de niveles, etc.), y ahí también se decide si el salto es a 2D dentro de Unreal (Paper2D) o directamente a 3D.
