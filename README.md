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

**Menú de inicio:**
- **Flechas arriba/abajo o W/S**: mover la selección entre las 4 opciones
  fijas, siempre en el mismo orden: "Nueva partida", "Cargar", "Sobre mi",
  "Salir". "Cargar" se dibuja atenuada si ningún slot tiene partida
  guardada (el cursor igual puede pararse ahí, pero confirmarla no hace
  nada); si hay al menos uno, lleva a la pantalla de elegir cuál de los 3
  cargar — ver "Guardado de partida" abajo.
- **ENTER o ESPACIO**: confirmar la opción resaltada. "Nueva partida"
  resetea el party y el mapa de mazmorras desde cero y lleva al **mapa**
  (ver abajo), no directo a una mazmorra.
- **"Sobre mi"** lleva a una pantalla placeholder (todavía sin contenido) —
  **ESC o ENTER** vuelve al menú desde ahí.

**Mapa de mazmorras:**
- **Flechas izquierda/derecha o A/D**: mover la selección entre las 3
  mazmorras — **Fácil**, **Media**, **Difícil** (dificultad creciente: más
  enemigos, enemigos más fuertes y mejor botín cuanto más difícil),
  elegibles en cualquier orden y rejugables sin límite (cada vez que se
  entra se genera un layout nuevo). Una mazmorra ya ganada muestra
  "(Superada)" pero se puede volver a jugar igual.
- **ENTER o ESPACIO**: entra a la mazmorra resaltada. El party conserva su
  HP, inventario y equipo tal cual estaban — no hay curación completa al
  cambiar de mazmorra.
- **ESC**: abre el menú de pausa (ver abajo).

**Exploración:**
- **WASD / flechas**: mover al líder del party (movimiento libre, no por
  grilla, con colisión contra las paredes). Los otros dos miembros siguen
  al líder en formación de fila. La velocidad de exploración es un 35% más
  rápida que la del stat base de cada personaje (no afecta el orden de
  turno en combate, que sigue usando el stat sin modificar).
- **E**: sobre el interactuable más cercano (enemigo o cofre) — engancha el
  combate de toda la sala, o abre el cofre y suma su contenido al
  inventario. El cartel de abajo indica cuál de los dos es ("[E] Atacar" /
  "[E] Abrir cofre") según lo que esté más cerca. No hace falta con los
  enemigos **agresivos** (nombre en rojo con un "!" arriba — por ahora,
  solo el Bandido Aturdidor): esos persiguen al líder solos y fuerzan el
  combate al alcanzarlo.
- **Ojo con las trampas de piso**: fuego (naranja) y ácido (charco verde),
  repartidas disperso por la mazmorra — pisarlas hace daño mientras te
  quedes parado encima (con un pequeño respiro entre golpe y golpe), tanto
  a vos como a cualquier enemigo que camine sobre ellas. Al líder no lo
  pueden matar directamente (queda en 1 HP como mínimo), pero a un enemigo
  agresivo sí — atraerlo sobre una trampa mientras te persigue es una forma
  válida de despacharlo sin pelear.
- **I**: abre/cierra el inventario (congela la exploración mientras está
  abierto).
- **TAB**: alterna el panel de party entre compacto (chiquito, no tapa el
  mapa) y expandido (con nombre/rol/HP detallado de cada uno).
- **F5**: abre la pantalla de elegir en cuál de los 3 slots guardar (la
  partida en curso: mazmorra, party, inventario, enemigos y cofres) — ver
  "Guardado de partida" abajo.
- **ESC**: con el inventario cerrado, abre el menú de pausa (ver abajo). Con
  el inventario abierto, lo cierra primero.

**Inventario (con [I] abierto):**
- **TAB**: cicla a cuál miembro del party se le va a aplicar el próximo
  item usado (se marca con "<" y un borde dorado en su ficha).
- **1-9**: usa (Consumibles) o equipa (Mejoras — reemplazan lo que haya en
  su ranura) el item de esa fila sobre el objetivo actual. Los consumibles
  de combate (Bomba de Veneno, Frasco de Escudo, Antídoto — marcados
  "[Solo en combate]") no hacen nada desde acá: avisan que hay que usarlos
  con [3] durante un combate.
- **I** o **ESC**: cierra el inventario y vuelve a la exploración.

**Pausa (ESC durante la exploración o parado en el mapa):**
- **Flechas arriba/abajo o W/S**: mover la selección entre "Continuar",
  "Guardar", "Volver al mapa", "Menú principal" y "Salir".
- **ENTER o ESPACIO**: confirmar la opción resaltada. "Guardar" abre la
  misma pantalla de selección de slot que F5; "Volver al mapa" abandona la
  mazmorra en curso sin marcarla como superada (el party conserva su HP,
  inventario y equipo — "sigue con el desgaste") y vuelve al mapa; si la
  pausa se abrió desde el propio mapa, esta opción no hace nada distinto de
  "Continuar"; "Menú principal" vuelve a la pantalla de título sin cerrar
  el juego y sin perder el progreso en curso (para retomarlo hay que haber
  guardado antes y elegir "Cargar" — "Nueva partida" en cambio resetea todo
  y descarta lo que había).
- **ESC**: vuelve directo a jugar (equivale a confirmar "Continuar") — a la
  exploración o al mapa, según desde dónde se haya abierto la pausa.

**Selección de slot (F5, o "Guardar"/"Cargar" según desde dónde se llegue):**
- **Flechas arriba/abajo o W/S**: mover la selección entre los 3 slots y
  "Volver".
- **ENTER o ESPACIO**: en modo Guardar, guarda en el slot resaltado (crea el
  archivo si estaba vacío, lo pisa si ya tenía partida) y muestra un cartel
  con el resultado; en modo Cargar, carga el slot resaltado si tiene
  partida — un slot vacío se ve atenuado y no hace nada al confirmarlo. Al
  cargar se vuelve a la exploración o al mapa según dónde estaba parada la
  partida guardada.
- **ESC**: vuelve a donde se abrió esta pantalla (exploración, mapa, pausa,
  o menú de inicio) sin hacer nada.

**Combate:**
- **1**: ataque básico (1d6 + bono de ataque, tirada de d20 vs la defensa
  del objetivo).
- **2**: habilidad de rol del personaje en turno (distinta por rol — ver
  "Sistema de combate" abajo).
- **3**: abre el sub-menú de "Usar item" con los Consumibles del inventario
  compartido — **1-9** usa el de esa fila (consume el turno, igual que un
  ataque), **TAB** cicla a qué aliado apunta (Bomba de Veneno en cambio
  siempre apunta al enemigo ya seleccionado como objetivo) y **ESC** cierra
  el sub-menú sin gastar el turno.
- **TAB** (solo si hay más de un enemigo vivo): cambia a cuál de los
  enemigos le apuntan las acciones del aliado en turno — se marca con "▶"
  y un borde dorado en su ficha.
- Cualquier tecla, al ganar: vuelve a la exploración — salvo que el
  enemigo derrotado fuera el **Capitán Bandido** (el jefe), en cuyo caso la
  mazmorra queda marcada "(Superada)" en el mapa y se vuelve ahí en vez de
  a la exploración (la mazmorra sigue siendo rejugable después).
- Cualquier tecla, al perder (pantalla de **Game Over**): reinicia la run
  entera — el party se recrea desde cero (stats de partida, sin items ni
  equipo) y las 3 mazmorras pierden su "(Superada)" — y vuelve al **mapa**
  a empezar de nuevo, no a la mazmorra donde se cayó.

## Sistema de combate (estilo BG3: dados + efectos)

Por turnos, con el orden decidido por la velocidad de cada unidad (se
calcula una sola vez al empezar el encuentro). Cada acción que ataca tira
un d20 + bono de ataque contra la "clase de defensa" del objetivo
(10 + su stat de defensa): 1 natural es pifia automática, 20 natural es
crítico automático (dobla los dados de daño). El daño baja la vida
directo, salvo que el objetivo tenga el efecto **Escudo** activo, que lo
absorbe primero.

Ya no hay un "maná" genérico: los roles físicos (Tanque, Daño) gastan
**Resistencia** al usar su habilidad — el cansancio de pegar un espadazo —
y los magos (Soporte, Control) gastan **Concentración**, que además se
rompe/reduce cuando reciben daño en combate (la misma cantidad que les
llegó a la vida), no solo al castear.

Habilidad de rol:
- **Tanque — Golpe Provocador** (5 de Resistencia): como un ataque básico,
  y si impacta aplica **Marcado** al enemigo (hace que priorice atacarlo a
  él en encuentros con varios enemigos). Además, se cubre a sí mismo con
  **Escudo** al usarla (impacte o no).
- **Daño — Golpe Certero** (5 de Resistencia): tira con ventaja (2d20, se
  queda con el mejor) y hace más daño (1d8 en vez de 1d6). Si sale crítico
  (20 natural), además aplica **Veneno** al enemigo.
- **Soporte — Curar** (8 de Concentración): sana 1d8+2 de vida al aliado
  con menos HP, sin tirada de ataque (no puede fallar).
- **Control — Grito Debilitante** (6 de Concentración): ataque menor (1d4)
  que, si impacta, aplica **Debilitado** al enemigo (resta a su bono de
  ataque mientras dura). El party de ejemplo ya incluye un personaje de
  este rol (Milo).

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
"Generación de mazmorra" más abajo) tiene un **grupo de 3 a 5 enemigos**,
de tipos elegidos al azar entre los tres siguientes (pueden repetirse — si
hay dos del mismo tipo en la sala, se distinguen con un sufijo, "Rata
Gigante II"):
- **Esqueleto Errante**: el original, parejo, solo ataque básico. Pasivo:
  espera a que el jugador se acerque y confirme con **E**.
- **Rata Gigante**: rápida y frágil (poca vida y defensa), ataque básico
  nomás — un combate corto y fácil. También pasiva.
- **Bandido Aturdidor**: el más resistente de los tres comunes (más HP y
  defensa); a veces, en vez de un golpe normal, usa **Golpe Aturdidor**,
  que si impacta aplica **Aturdido** (pierde el turno) — la primera fuente
  real de ese efecto en el juego. Su ataque quedó igualado al del
  Esqueleto tras un ajuste de balance (ver más abajo) — grupos de 3 eran
  desproporcionadamente más duros que con cualquier otro enemigo. Es el
  único tipo **agresivo**: persigue al líder si se acerca y fuerza el
  combate al alcanzarlo, sin esperar a **E** (se distingue por su nombre en
  rojo con un "!" arriba).

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
  vida) y **Elixir de Energía** (1d6+4 de Resistencia o Concentración,
  según a quién se le use).
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

**Consumibles de combate** (mismo tipo Consumible, pero solo tienen efecto
usados con **[3] Usar item** durante un combate, ver "Controles" arriba):
**Bomba de Veneno** (aplica Veneno a un enemigo sin necesidad de acertar un
golpe), **Frasco de Escudo** (da Escudo a un aliado) y **Antídoto** (cura
Aturdido y Veneno de un aliado). Se pueden juntar y ver desde el inventario
de exploración igual que cualquier otro item (con un tag `[Solo en
combate]`), pero usarlos ahí no hace nada más que avisar que hacen falta un
combate — Bomba de Veneno en particular necesita un enemigo como objetivo,
que no existe fuera de uno.

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
  la Rata Gigante, y ambos a veces sueltan un consumible de combate o (solo
  el Bandido) una mejora permanente en vez de una poción — recompensa extra
  por ser los más duros de pelear.

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

## Sonido

Música de fondo (dos pistas en loop: una calma para exploración, otra más
tensa para combate — cambia sola al enganchar/salir de un combate) y
efectos para golpe, crítico, curación, fallo, victoria y derrota. Todos los
clips (`assets/audio/*.wav`) son sintetizados 100% por código
(`tools/generar_audio.py`, con numpy/scipy) — sin samples ni música de
terceros —, pensados como placeholder de prototipo, fáciles de reemplazar
por assets definitivos más adelante sin tocar una línea de código (misma
ruta y nombre de archivo). Si la máquina no tiene dispositivo de audio
disponible, el juego lo detecta y sigue andando en silencio, sin crashear.
Ver "Sonido" en `docs/design.md` para el detalle técnico (`render/audio.h`).

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
15. ✅ Feedback visual de combate: numeritos flotantes de daño (rojo) /
    curación (verde) / fallo ("FALLO", gris) sobre la ficha correspondiente
    — más grandes y dorados en un crítico —, que suben y se desvanecen
    durante 1 segundo, más un flash rojo breve en la ficha de quien recibe
    un golpe. Toda la lógica de animación vive en `render/combat_ui.cpp`
    (game/combat.h sólo expone los eventos como datos estructurados) — ver
    "Feedback visual de combate" en `docs/design.md` para el detalle.
16. ✅ Sonido: música de fondo (loop de exploración/combate, cambia sola
    según el estado) y efectos de golpe/crítico/curación/fallo/victoria/
    derrota, todos generados por código (sin assets con licencia de
    terceros) — sigue andando en silencio si la máquina no tiene
    dispositivo de audio. Ver "Sonido" arriba y en `docs/design.md`.
17. ✅ Variedad de salas: además de los 4 templates rectangulares de
    siempre, ahora hay 2 formas nuevas — una sala en L (14x14 con una
    esquina recortada) y una con 4 pilares (12x12) — elegidas al azar
    junto con las demás. Se implementaron recortando tiles del set de piso
    antes de calcular las paredes, así que no hizo falta tocar cámara,
    grilla ni colisión; las dos formas garantizan piso libre alrededor del
    centro (donde arrancan los enemigos) y en la esquina superior
    izquierda (donde va el cofre). Verificado con un fuzz test de 300
    mazmorras (1500 salas) sin fallos — ver "Generación de mazmorra" abajo
    y "Variedad de formas de sala" en `docs/design.md`.
18. ✅ Menú de inicio: pantalla de título antes de largar a explorar, con 4
    opciones fijas navegables con las flechas (o W/S) y ENTER/ESPACIO para
    confirmar — "Nueva partida", "Cargar" (atenuada si no hay guardado),
    "Sobre mi" (placeholder por ahora) y "Salir" — ver "Menú de inicio" en
    `docs/design.md`.
19. ✅ Guardado de partida: **F5** guarda la partida en curso (mazmorra,
    party con stats/posición/equipo, inventario, enemigos y cofres) a un
    único slot en texto plano (`savegame.txt`, sin librerías externas). El
    menú de inicio ofrece "Cargar" habilitada apenas hay un guardado. Un
    archivo corrupto o truncado no rompe el juego — se ignora y el jugador
    puede arrancar de cero. Ver "Guardado de partida" abajo y en
    `docs/design.md`. (Ampliado a 3 slots en el ítem 25.)
20. ✅ Pausa y vuelta al menú: **ESC** durante la exploración abre un menú
    de pausa con Continuar/Guardar/Menú principal/Salir — antes, una vez
    adentro de una partida no había forma de volver a la pantalla de
    título sin cerrar el juego. "Nueva partida" ahora se puede elegir más
    de una vez por corrida (antes solo al arrancar el ejecutable) y
    siempre genera una mazmorra distinta. De paso se corrigió un bug real:
    ESC cerraba el juego entero en vez de volver de una pantalla, por la
    tecla de salida por defecto de raylib. Ver "Pausa y vuelta al menú" en
    `docs/design.md`.
21. ✅ Sprites pixel-art: personajes, enemigos, cofres y tiles de piso/pared
    dejaron de ser círculos y rectángulos de color liso — ahora son sprites
    pixel-art generados por código (`render/sprites.h`/`.cpp`, sin archivos
    de assets ni licencias de terceros, mismo criterio que el audio
    sintetizado) escalados con filtro nearest-neighbor para verse nítidos.
    Los retratos chicos del HUD, el inventario y las fichas de combate
    reusan la misma textura. Ver "Sprites pixel-art generados por código"
    en `docs/design.md`.
22. ✅ Ambientación de mapa: se sacó la grilla de referencia (era la
    principal causa de que el mapa se sintiera "a planilla vacía") y se
    sumó decoración suelta de piso (grieta/musgo/escombros/charco,
    repartida dispersa por tile) más antorchas encendidas en la fila
    superior de cada sala, con un parpadeo sutil — todo pixel-art generado
    por código. Ver "Ambientación de mapa" en `docs/design.md`.
23. ✅ Recursos: Resistencia y Concentración: el "maná" genérico de siempre
    se reemplazó por dos recursos con identidad propia — Resistencia
    (Tanque/Daño, cansancio físico) y Concentración (Soporte/Control,
    esfuerzo mental, que además se rompe/reduce al recibir daño en
    combate). El Tanque pasó a tener recurso propio y un costo real en
    Golpe Provocador (antes gratis). Ver "Sistema de combate" arriba y
    "Recursos: Resistencia y Concentración" en `docs/design.md`.
24. ✅ Enemigos agresivos y salas más pobladas: los grupos por sala pasaron
    de 1-3 a 3-5 enemigos, en grilla en vez de una fila. El Bandido
    Aturdidor ahora persigue al líder durante la exploración y fuerza el
    combate al alcanzarlo (sin pathfinding, aviso visual en rojo) — el
    resto sigue esperando **E** como antes. Ver "Sistema de combate" arriba
    y "Enemigos agresivos" en `docs/design.md`.
25. ✅ Guardado con 3 slots: F5 y "Guardar" en la pausa dejaron de pisar
    siempre el mismo archivo — ahora abren una pantalla para elegir entre 3
    slots independientes, y "Cargar" hace lo mismo para elegir cuál
    retomar. Ver "Guardado de partida" arriba y en `docs/design.md`.
26. ✅ Exploración más ágil: la velocidad de movimiento durante la
    exploración subió un 35% (líder y persecución de enemigos agresivos por
    igual), sin tocar el stat de velocidad que decide el orden de turno en
    combate. Ver "Sistema de combate" arriba y "Velocidad de movimiento en
    la exploración" en `docs/design.md`.
27. ✅ Trampas de piso: fuego y ácido, repartidas disperso por la mazmorra
    (una por sala como máximo, nunca en la inicial), afectando por igual al
    jugador y a los enemigos — un enemigo agresivo puede morir si lo atraen
    sobre una, mientras que al líder nunca lo matan fuera de combate. Subió
    la versión del archivo de guardado (v1 → v2): partidas guardadas antes
    de esta actualización ya no se pueden cargar. Ver "Controles" arriba y
    "Trampas de piso: fuego y ácido" en `docs/design.md`.
28. ✅ Consumibles de combate: nueva opción **[3] Usar item** durante el
    turno del aliado (consume el turno igual que un ataque), con tres items
    nuevos — Bomba de Veneno, Frasco de Escudo y Antídoto — que reutilizan
    el sistema de efectos de estado ya existente en vez de una mecánica
    aparte. Se suman al loot de cofres y de Esqueleto Errante/Bandido
    Aturdidor. Ver "Controles" y "Sistema de inventario y loot" arriba, y
    "Consumibles de combate: Bomba de Veneno, Frasco de Escudo, Antídoto"
    en `docs/design.md`.
29. ✅ Mapa de mazmorras: en vez de una única mazmorra por run, un mapa con 3
    mazmorras de dificultad creciente (**Fácil/Media/Difícil**, más enemigos
    y mejor loot cuanto más difícil), elegibles en cualquier orden y
    rejugables sin límite con un layout nuevo cada vez. El party persiste
    entre mazmorras (sin curación completa al cambiar); ganarle al jefe
    marca esa mazmorra "(Superada)" sin bloquearla; un Game Over ahora
    reinicia la run entera (party desde cero, todo el progreso del mapa
    perdido) en vez de revivir en el lugar — estilo roguelite, pedido
    directo del usuario. Subió la versión del archivo de guardado (v2 → v3):
    partidas guardadas antes de esta actualización ya no se pueden cargar.
    Ver "Controles" arriba y "Mapa de mazmorras: selección de dificultad y
    reinicio de la run" en `docs/design.md`.
30. Pendiente: contenido para "Sobre mi", seguir sumando contenido de
    juego (más enemigos comunes, mazmorras más largas, o retomar la
    historia de ambientación shelveada) antes del build de Android.

Ver `docs/design.md` para el detalle completo de arquitectura y roadmap.

## Generación de mazmorra

Cada partida arma la mazmorra de cero (`game::Dungeon`, en
`src/game/dungeon.cpp`): una cadena de 5 salas, cada una elegida al azar
entre 6 "templates" de tamaño y forma (chica, grande, alargada, mediana,
más una en L y una con pilares), donde cada sala se ubica pegada a la
anterior extendiéndose al Este o al Sur (al azar), conectada por un
pasillo de 3 tiles de ancho. La sala 0 es siempre el punto de partida del
party (sin enemigos); las otras 4 tienen un grupo de 3 a 5 enemigos de
tipo aleatorio cada una, repartidos en una grilla de hasta 3 por fila.

Las dos formas no rectangulares recortan tiles del rectángulo base (la L
pierde una esquina inferior derecha, la sala con pilares tiene 4 bloques
2x2 simétricos adentro) pero siempre dejan libres las dos zonas que el
resto del juego necesita: el centro (donde aparece el grupo de enemigos) y
la esquina superior izquierda (donde va el cofre de la sala).

Cada sala con contenido tiene además, con 40% de chance, **una trampa de
piso** (fuego o ácido al azar) en algún tile libre que no sea ni el centro
ni la esquina del cofre — ver "Controles del prototipo actual" arriba y
"Trampas de piso: fuego y ácido" en `docs/design.md`.

Las paredes se calculan solas: se arma primero el conjunto completo de
tiles de piso (salas + pasillos) y, al final, cualquier tile del área
total que no sea piso se convierte en pared — así una sala y su pasillo
quedan automáticamente "abiertos" entre sí, sin tener que calcular a mano
dónde va cada puerta.

Como el layout generado es más grande que la ventana (1280x720), la
cámara (`Camera2D` en `render/renderer.cpp`) sigue al líder del party;
la grilla de referencia se dibuja sólo dentro de cada sala, no en los
pasillos ni fuera del layout.

## Guardado de partida

**F5** (en cualquier momento de la exploración) o **Guardar** en el menú de
pausa (ESC) abren la misma pantalla: elegir en cuál de **3 slots**
guardar. Cada slot es un archivo de texto plano independiente
(`savegame_slot1.txt`, `savegame_slot2.txt`, `savegame_slot3.txt`, en la
misma carpeta desde donde se corre el ejecutable, sin ninguna librería
externa de serialización) — guardar en uno vacío lo crea, en uno ocupado lo
pisa; los otros dos quedan intactos. Se persiste todo lo necesario para
retomar exactamente donde quedaste: la mazmorra ya generada (salas +
paredes + trampas, formas incluidas), los 4 personajes del party (stats,
posición, Arma/Accesorio equipados), el inventario compartido, los
enemigos (con cuáles ya están derrotados), los cofres (con cuáles ya se
abrieron) y el progreso en el mapa de mazmorras (qué mazmorras ya se
ganaron esta run, y si la partida quedó guardada parada en el mapa o a
mitad de una mazmorra). No se puede guardar en medio de un combate.

El formato de archivo subió de versión dos veces: v1 → v2 al agregar las
trampas de piso, y v2 → v3 al agregar el mapa de mazmorras (de paso se
corrigió ahí un bug real: los consumibles de combate — Bomba de Veneno,
Frasco de Escudo, Antídoto — no se guardaban bien desde que se agregaron, y
un save/load los corrompía en silencio). Una partida guardada con una
versión anterior de este prototipo ya no se puede cargar (se rechaza igual
que un archivo corrupto, sin crashear).

El menú de inicio dibuja "Cargar" habilitada apenas **algún** slot tiene
partida guardada (atenuada y sin efecto si los 3 están vacíos) — al
confirmarla lleva a la misma pantalla de selección, ahora en modo Cargar:
los slots vacíos se ven atenuados y no hacen nada al confirmarlos, y elegir
uno ocupado carga y reemplaza por completo la mazmorra/party/enemigos/
cofres que ya se habían generado para el fondo del menú. Un archivo dañado
o de una versión vieja del formato no rompe el juego: `CargarPartida` lo
detecta y devuelve inválido, así que el jugador se queda en esa pantalla y
puede elegir otro slot o volver.

El patrón `savegame_slot*.txt` está en `.gitignore` — es el progreso de
quien juega, no parte del código fuente.

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
