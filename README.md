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
- **E**: sobre el interactuable más cercano (enemigo o cofre) — engancha el
  combate de toda la sala, o abre el cofre y suma su contenido al
  inventario. El cartel de abajo indica cuál de los dos es ("[E] Atacar" /
  "[E] Abrir cofre") según lo que esté más cerca.
- **I**: abre/cierra el inventario (congela la exploración mientras está
  abierto).
- **TAB**: alterna el panel de party entre compacto (chiquito, no tapa el
  mapa) y expandido (con nombre/rol/HP detallado de cada uno).

**Inventario (con [I] abierto):**
- **TAB**: cicla a cuál miembro del party se le va a aplicar el próximo
  item usado (se marca con "<" y un borde dorado en su ficha).
- **1-9**: usa (Consumibles) o equipa (Mejoras — reemplazan lo que haya en
  su ranura) el item de esa fila sobre el objetivo actual.
- **I**: cierra el inventario y vuelve a la exploración.

**Combate:**
- **1**: ataque básico (1d6 + bono de ataque, tirada de d20 vs la defensa
  del objetivo).
- **2**: habilidad de rol del personaje en turno (distinta por rol — ver
  "Sistema de combate" abajo).
- **TAB** (solo si hay más de un enemigo vivo): cambia a cuál de los
  enemigos le apuntan las acciones del aliado en turno — se marca con "▶"
  y un borde dorado en su ficha.
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
party, y Aturdido desde el Golpe Aturdidor del Bandido Aturdidor. Marcado
ahora tiene un efecto observable de verdad: un enemigo marcado prioriza
atacar al Tanque en su turno en vez de al aliado con menos vida (antes,
con un solo enemigo posible en pantalla, "a quién prioriza" nunca se
notaba).

Cada sala **intermedia** con contenido de la mazmorra procedural (ver
"Generación de mazmorra" más abajo) tiene un **grupo de 1 a 3 enemigos**,
de tipos elegidos al azar entre los tres siguientes (pueden repetirse — si
hay dos del mismo tipo en la sala, se distinguen con un sufijo, "Rata
Gigante II"):
- **Esqueleto Errante**: el original, parejo, solo ataque básico.
- **Rata Gigante**: rápida y frágil (poca vida y defensa), ataque básico
  nomás — un combate corto y fácil.
- **Bandido Aturdidor**: el más resistente de los tres comunes (más HP y
  defensa); a veces, en vez de un golpe normal, usa **Golpe Aturdidor**,
  que si impacta aplica **Aturdido** (pierde el turno) — la primera fuente
  real de ese efecto en el juego. Su ataque quedó igualado al del
  Esqueleto tras un ajuste de balance (ver más abajo) — grupos de 3 eran
  desproporcionadamente más duros que con cualquier otro enemigo.

La **última sala** con contenido, en cambio, tiene un único **Capitán
Bandido**: el jefe de la mazmorra. Más HP, ataque y defensa que cualquier
enemigo común, un círculo bien más grande y oscuro con anillo dorado para
distinguirlo a simple vista, e IA propia que alterna ataque básico, Golpe
Aturdidor y **Doble Tajo** (dos golpes en el mismo turno) — por debajo del
40% de HP entra en furia y usa Doble Tajo siempre. Siempre suelta una
mejora permanente al caer, y derrotarlo muestra una pantalla de cierre
distinta ("¡MAZMORRA DESPEJADA!") en vez del cartel genérico de victoria —
ver "Jefe de mazmorra" en `docs/design.md` para el detalle completo.

Al acercarse y apretar E sobre cualquiera de los enemigos de una sala, se
engancha un solo combate contra **todo el grupo de esa sala** (no uno por
uno, y contra el jefe es 1 contra todo el party igual): el orden de turnos
intercala a los 4 del party con todos los enemigos vivos según velocidad,
cada enemigo actúa por separado en su turno, y la victoria requiere
derrotarlos a todos. Las salas de otras partes de la mazmorra quedan de
fondo, sin participar, hasta que se las engancha.

## Sistema de inventario y loot

Inventario único, compartido por todo el party (`game::Inventory`, dentro
de `game::Party`), que apila items iguales en una sola fila con cantidad
(`PilaItem`). Cada item (`game::Item`) es de uno de dos tipos:
- **Consumible**: cura vida o recurso tirando dados (`RollDados`), y se
  gasta una unidad al usarse — **Poción de Curación Menor** (2d6+2 de
  vida) y **Elixir de Energía** (1d6+4 de recurso).
- **Mejora**: sube una stat en un monto fijo **para siempre**, pero en vez
  de gastarse al toque se **equipa** en una ranura del personaje elegido.
  Cada ranura tiene dos sabores para elegir, no un solo camino: **Arma**
  es **Piedra de Fuerza** (+1 ataque) o **Daga Veloz** (+10 velocidad —
  actuar más seguido en vez de pegar más fuerte); **Accesorio** es
  **Amuleto de Protección** (+1 defensa) o **Talismán de Vitalidad** (+5
  vida máxima, que sube la vida actual junto con el máximo — aguantar más
  golpes en vez de que te conecten menos). Cada personaje tiene una sola
  ranura de Arma y una de Accesorio: no se le puede poner "5 espadas" al
  mismo, la sexta vez que equipa un arma reemplaza a la anterior en vez de
  sumarse. Si la ranura ya tenía algo puesto, lo reemplazado vuelve solo
  al inventario compartido (no se pierde).

Fuentes de items:
- **Cofres** (`game::Cofre`): objetos fijos en la mazmorra, dibujados como
  un cuadrado dorado (se ponen grises/vacíos una vez abiertos). Hay uno
  garantizado en la sala inicial y, además, un 40% de chance por cada sala
  con contenido de tener uno extra ubicado en una esquina (lejos de su
  grupo de enemigos). Se abren con **E**, igual que se engancha combate
  con un enemigo — el cartel de abajo indica cuál de las dos acciones
  corresponde según qué esté más cerca.
- **Botín de combate**: al ganar un encuentro, se tira una vez por cada
  enemigo derrotado (`TirarLootDeEnemigo`), con tablas de drop distintas
  por tipo — Esqueleto Errante y Bandido Aturdidor sueltan más seguido que
  la Rata Gigante, y el Bandido a veces suelta una mejora permanente en
  vez de un consumible (recompensa extra por ser el más duro de pelear).

La pantalla de inventario (**I**, `render/inventory_ui.cpp`) muestra una
ficha por miembro del party — con su Arma y Accesorio equipados, "-" si no
tiene nada puesto en esa ranura — resaltando a quién se le va a aplicar el
próximo item (**TAB** para cambiar), y la lista de items apilados con un
número al lado de cada uno; los de tipo Mejora se marcan con un tag
`[Equipar: Arma]` / `[Equipar: Accesorio]` para dejar claro que van a
reemplazar lo que haya en esa ranura, no a sumarse. **1-9** usa (los
Consumibles) o equipa (las Mejoras) ese item sobre el objetivo actual. El
objetivo puede ser cualquier miembro, vivo o no — pensado para poder
revivir/curar a alguien caído sin salir del inventario para reordenar el
party. Abrir el inventario congela el movimiento y las interacciones de
exploración hasta cerrarlo. El equipo también se puede ver sin abrir el
inventario: el panel de party expandido (**TAB** en exploración) muestra
el Arma/Accesorio de cada uno debajo de su barra de HP.

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
   Errante, Rata Gigante, Bandido Aturdidor).
6. ✅ Pantalla de Game Over: al perder un combate, se ve una pantalla
   distinta a la de victoria y, al apretar una tecla, el party revive a
   full HP/recurso (sin efectos) y vuelve al punto de partida — antes,
   perder dejaba al party en HP 0 para siempre (softlock).
7. ✅ Generación de mazmorra por salas conectadas: en vez de una sala fija,
   cada partida arma una cadena de 5 salas (una inicial sin enemigo + 4
   con contenido) de tamaños variados, unidas por pasillos, con una
   cámara que sigue al líder para poder navegar el layout completo (ver
   "Generación de mazmorra" abajo).
8. ✅ Encuentros con varios enemigos a la vez: cada sala con contenido
   tiene un grupo de 1 a 3 enemigos (tipos al azar, pueden repetirse) que
   se engancha entero en un solo combate — turnos intercalados entre todo
   el party y todos los enemigos vivos, selector de objetivo con TAB, y
   Marcado ahora hace que un enemigo marcado priorice atacar al Tanque en
   vez de al aliado más débil.
9. ✅ Inventario y loot: catálogo de items (pociones, elixires, mejoras
   permanentes de ataque/defensa), cofres ubicados en la mazmorra (uno
   garantizado en la sala inicial, más una chance por sala con contenido),
   botín al derrotar enemigos (distinto por tipo), y una pantalla de
   inventario ([I]) para ver y usar lo que se junta — ver "Sistema de
   inventario y loot" abajo.
10. ✅ Ranuras de equipo: las mejoras permanentes ya no se consumen al
    toque — se equipan en una ranura de Arma o Accesorio por personaje (una
    sola de cada una), reemplazando lo que hubiera antes en vez de
    acumularse sin límite, y visibles tanto en el inventario como en el
    panel de party expandido.
11. ✅ Balance general (primera pasada): un simulador Monte Carlo sobre
    combates reales detectó que los grupos de 3x Bandido Aturdidor eran
    mucho más duros que cualquier otro grupo de 3 (76-79% de victorias y
    ~59% de HP restante vs. 99-100%/78-88% del resto); se corrigió
    bajándole ataque (8→7) y HP (26→24), sin tocar su defensa ni el
    aturdimiento — ver `docs/design.md` para el detalle de la
    investigación.
12. ✅ Jefe de mazmorra: la última sala ya no tiene un grupo más — tiene al
    Capitán Bandido, único, con más stats que cualquier enemigo común, IA
    propia (Doble Tajo + furia bajo 40% HP), botín garantizado, y una
    pantalla de cierre distinta ("¡MAZMORRA DESPEJADA!"). Simulado igual
    que el resto del balance: 87.5% de clears totales con items (vs. 91.9%
    antes de agregar el jefe) — más difícil, como corresponde al cierre de
    la run, pero ampliamente superable.
13. ✅ Curva de poder investigada: se midió cuánto poder equipado acumula
    el party a lo largo de una run completa — con la duración actual (4
    salas + jefe) llega al Capitán Bandido con menos de 1 punto de bono en
    promedio, sin efecto medible en la tasa de victoria contra él. No hay
    curva de poder que corregir por ahora — ver `docs/design.md` para el
    detalle. Sin cambios de código esta vuelta.
14. ✅ Catálogo de Mejoras ampliado: de 2 a 4 piezas — Daga Veloz (Arma,
    +10 velocidad) y Talismán de Vitalidad (Accesorio, +5 vida máxima) se
    suman a Piedra de Fuerza y Amuleto de Protección, dándole a cada
    ranura dos sabores reales para elegir en vez de un solo camino.
15. Pendiente: seguir sumando contenido (más enemigos comunes, más
    variedad de salas o una mazmorra más larga) antes del build de
    Android.

Ver `docs/design.md` para el detalle completo de arquitectura y roadmap.

## Generación de mazmorra

Cada partida arma la mazmorra de cero (`game::Dungeon`, en
`src/game/dungeon.cpp`): una cadena de 5 salas, cada una elegida al azar
entre 4 "templates" de tamaño (chica, grande, alargada, mediana), donde
cada sala se ubica pegada a la anterior extendiéndose al Este o al Sur (al
azar), conectada por un pasillo de 3 tiles de ancho. La sala 0 es siempre
el punto de partida del party (sin enemigos); las otras 4 tienen un grupo
de 1 a 3 enemigos de tipo aleatorio cada una.

Las paredes se calculan solas: se arma primero el conjunto completo de
tiles de piso (salas + pasillos) y, al final, cualquier tile del área
total que no sea piso se convierte en pared — así una sala y su pasillo
quedan automáticamente "abiertos" entre sí, sin tener que calcular a mano
dónde va cada puerta.

Como el layout generado es más grande que la ventana (1280x720), la
cámara (`Camera2D` en `render/renderer.cpp`) sigue al líder del party;
la grilla de referencia se dibuja sólo dentro de cada sala, no en los
pasillos ni fuera del layout.

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
