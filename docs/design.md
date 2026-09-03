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

Mazmorras en 2D, vista cenital, basadas en grilla/tiles. La generación es procedural por cadena de salas (`game::Dungeon`, en `src/game/dungeon.cpp`): en vez de una sola sala fija, cada partida arma una secuencia de 5 salas, cada una elegida al azar entre 6 "room templates" de tamaño y forma (chica 8x8, grande 14x10, alargada 6x14, mediana 10x10, más dos formas no rectangulares — ver "Variedad de formas de sala" más abajo), donde cada sala nueva se ubica pegada a la anterior extendiéndose al Este o al Sur (elegido al azar), conectada por un pasillo de 3 tiles de ancho centrado en el solape entre ambas. La sala 0 es siempre el punto de partida del party, sin enemigos; las otras 4 tienen contenido (un grupo de 1 a 3 enemigos de tipo aleatorio cada una — ver "Encuentros multi-enemigo" más abajo).

Las paredes se calculan de forma indirecta y robusta: primero se arma el conjunto completo de tiles de piso (unión de todas las salas más los pasillos que las conectan), y recién al final cualquier tile dentro del área total ocupada que no sea piso se convierte en una pared (un `Rect` de 1 tile por celda — más pared de la estrictamente necesaria comparado con fusionar rects, pero mucho más simple y difícil de romper a esta escala). Esto evita calcular "aberturas" a mano donde un pasillo conecta con una sala: al ser piso de los dos lados, ya no queda pared en el medio.

Como el layout generado es más grande que la ventana (1280x720), la capa de presentación agrega una cámara (`Camera2D` en `render/renderer.cpp`) que sigue al líder del party; la grilla de referencia se dibuja sólo dentro de los límites de cada sala, no en los pasillos ni fuera del layout ocupado.

El movimiento del personaje dentro de la mazmorra sigue siendo **libre/continuo** (no por grilla): el personaje se desplaza en cualquier ángulo, con colisión contra las paredes resuelta por rectángulos (eje por eje) en vez de solo la celda destino. Se eligió así pensando en que se sienta más natural, sobre todo en mobile con un joystick virtual más adelante.

Este generador procedural fue validado con fuzz-testing: un test standalone (`test_game_layer.cpp`, fuera del build de raylib) construye muchas mazmorras con semillas al azar y verifica que el centro de cada sala generada quede en piso transitable (no solapado con una pared) y que la colisión contra la pared izquierda de la sala inicial se comporte de forma consistente. Un hallazgo de ese proceso: con el movimiento en pasos discretos (velocidad × delta-tiempo por frame), la posición final contra una pared puede quedar hasta un paso entero más allá del borde exacto — no es un bug de colisión ni de generación, así que el test tolera ese margen en vez de exigir precisión sub-píxel.

### Variedad de formas de sala

Hasta esta vuelta las 4 salas eran siempre rectángulos (variaban en tamaño, nunca en forma). Se agregaron dos formas nuevas — L y con pilares — sin tocar `Habitacion` (sigue siendo el bounding box de siempre) ni ningún otro lugar del código que ya asume que una sala "es su rectángulo" (`CentroDeSala`, la cámara, la grilla, el cálculo de paredes). Esto fue posible por la arquitectura de piso-primero descripta arriba: como las paredes se derivan de "todo lo que no sea piso" dentro del bounding box, agregar una forma nueva es sólo cuestión de no insertar ciertos tiles en el set de piso al armar la sala — el resto del pipeline (paredes, colisión, cámara, grilla) no se entera de que la sala no es un rectángulo completo.

**Datos**: `enum class FormaSala { Rectangular, LForma, ConPilares }`, interno a `dungeon.cpp` (no se expone en `dungeon.h`). `RoomTemplate` ahora lleva `forma` más `muescaAncho`/`muescaAlto` (solo para `LForma`). `kTemplates[]` pasó de 4 a 6 entradas: las 4 rectangulares de siempre más `{14, 14, FormaSala::LForma, 4, 4}` (una L de 14x14 con una muesca de 4x4) y `{12, 12, FormaSala::ConPilares}` (un cuadrado de 12x12 con 4 pilares de 2x2 adentro). `GenerarTilesDeSala()` reemplaza el viejo doble-loop que insertaba el rectángulo entero: recorre el mismo rango pero, según la forma, salta los tiles que caen en la muesca (`LForma`) o en uno de los pilares (`ConPilares`, vía `EsCeldaDePilar`).

**La restricción de diseño real no era geométrica sino de gameplay**: `main.cpp` asume que cualquier sala tiene dos zonas garantizadas como piso — `CrearGrupoDeSala` desparrama de 1 a 3 enemigos alrededor del centro geométrico del bounding box (offsets de hasta ±70px horizontal, ±18px vertical) y `CrearCofreEnEsquina` pone un cofre cerca de la esquina superior izquierda (`sala.x+1.5, sala.y+1.5` en tiles). Ninguna forma nueva podía arriesgar que un enemigo o el cofre aparecieran incrustados en una pared. Por eso la muesca de la L siempre recorta la esquina **inferior derecha** (nunca toca la esquina superior izquierda del cofre) y queda lo bastante lejos del centro del bounding box; y los 4 pilares de `EsCeldaDePilar` se calculan simétricos y bien adentro del cuadrado (a un cuarto y tres cuartos del ancho/alto), nunca tocando un borde ni la esquina superior izquierda, así que tampoco pueden interferir con un pasillo (que siempre se conecta centrado en un borde, nunca cerca de una esquina).

**Verificación**: se agregó un Test 6 a `test_game_layer.cpp` que fuerza el problema estadístico de "6 templates al azar, una sola `Dungeon` puede no tocar nunca las 2 formas nuevas" — fuzzea 300 mazmorras (1500 salas en total) y para cada sala verifica dos cosas con colisión real contra `d.Paredes()`, no solo el punto exacto del centro: el rectángulo completo donde `CrearGrupoDeSala` puede desparramar enemigos, y la celda exacta de la esquina del cofre. Además cuenta cuántas salas terminan con al menos una pared estrictamente interior a su bounding box (algo que sólo puede pasar en una L o con pilares) para confirmar que las formas nuevas se están generando de verdad y no silenciosamente nunca — 0 hubiera sido indicio de un bug, no de que las formas estaban "bien". Corridas de esta suite: cero fallos de assert en 7500 chequeos de sala (5 corridas de 1500 cada una), con entre 470 y 503 salas con pared interior detectada por corrida (~31-34%, consistente con 2 de 6 templates no rectangulares). Verificado también visualmente con un harness temporal bajo Xvfb (después borrado, no se shipea) que confirmó ambas formas renderizando y conectándose bien por pasillo.

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
- **Botín garantizado** (`TirarLootDeEnemigo` en `item.cpp`): a diferencia de los enemigos comunes (que sueltan botín con cierta probabilidad, y no siempre una mejora), el jefe **siempre** suelta una mejora permanente (una de las 4 del catálogo al azar, ver "Sistema de inventario y loot" abajo) — el premio grande de haber llegado hasta el final.
- **Cierre de la run**: derrotarlo dispara una pantalla de victoria distinta ("¡MAZMORRA DESPEJADA!", en `combat_ui.cpp`, detectada por ser el único enemigo del encuentro y de tipo `CapitanBandido`) en vez del cartel genérico de "¡VICTORIA!" — la mazmorra, hasta ahora, no tenía ningún cierre más allá de "no queda nada más que hacer".

Simulado con el mismo harness Monte Carlo del balance general (`test_balance.cpp`): solo y con party fresco, el Capitán Bandido se gana un 99.3% de las veces con ~81.5% de HP promedio restante (17 turnos en promedio) — comparable o algo más manejable que 3x Bandido Aturdidor (94.7%/72.9%), porque enfrentarlo solo, sin dividir el daño del party entre varios objetivos, compensa el golpe extra de Doble Tajo. La cifra que importa es la de la run completa hasta este jefe (ver más abajo): con el sistema de items activo, la mazmorra completa (4 salas + jefe) se termina un 87.5% de las veces (vs. 91.9% antes de agregar el jefe) — una dificultad final notoria pero ampliamente superable, en línea con la filosofía "duro pero justo" del resto del balance.

### Balance: curva de poder de las mejoras permanentes

Pregunta pendiente del roadmap: a lo largo de una run completa (4 salas + jefe), ¿el party llega a acumular tanto equipo que se vuelve desproporcionadamente fuerte de cara al Capitán Bandido? (Medido cuando el catálogo de Mejoras todavía era Piedra de Fuerza / Amuleto de Protección nomás — el catálogo creció después a 4 piezas, ver "Sistema de inventario y loot" arriba, pero eso no cambia la conclusión: la cantidad de Mejoras que caen por run sigue siendo la misma, solo cambió cuál de las 4 te toca.) Se instrumentó `test_balance.cpp` (`SimularCurvaDePoder`) para medir el **poder equipado total** (suma de todos los bonos de ataque + defensa puestos en todo el party) justo antes de cada uno de los 4 combates, promediado sobre miles de corridas, y también para probar una política de inventario más realista que la original: en vez de repartir mejoras siempre a los mismos 2 personajes (Kael/Arma, Bruna/Accesorio, que después de la primera ya solo reemplazan sin ganancia neta), `GestionarInventarioV2` llena la ranura vacía de **cualquiera** de los 4 primero — el límite real es una por ranura por personaje, no por party, y es lo que haría un jugador que reparte a propósito por el TAB del inventario.

Resultado (N=5000): repartir mejor solo sube el clear rate de 86.5% a 88.4% — una diferencia chica. Lo más revelador es la curva en sí:

| Antes de... | Poder equipado promedio |
|---|---|
| Sala 1 | +0.28 |
| Sala 2 | +0.48 |
| Sala 3 | +0.67 |
| Jefe (sala 4) | +0.86 |

El party llega al Capitán Bandido con **menos de 1 punto** de bono acumulado en promedio (repartido entre ataque y defensa, entre los 4 personajes). Separando las victorias contra el jefe por el poder equipado en ese momento (0, exactamente 1, o 2 o más) la tasa de victoria no se mueve de forma consistente (91.3% / 93.2% / 91.4%) — el equipo no está cambiando el resultado de esa pelea de forma medible, el ruido estadístico domina.

**Conclusión: no hay curva de poder que corregir.** Con la duración actual de la run (4 salas de contenido + jefe) y las probabilidades de drop actuales, las mejoras permanentes son un plus menor y coleccionable, no un factor de dificultad — ni corre riesgo de romper el balance del jefe, ni aporta una progresión de poder que se sienta significativa a lo largo de la partida. Si en algún momento la mazmorra se alarga (más salas, más de una por corrida) o se quiere que el looteo se sienta más impactante, la palanca más directa es subir la probabilidad de mejora en las tablas de botín (`TirarLootDeEnemigo`) y de cofre (`ItemAleatorioDeCofre`) — hoy pensadas para una run corta. Por ahora se deja como está: no hay nada roto que arreglar, es una decisión de diseño (¿se busca más impacto del loot, a costa de posible curva de poder más adelante?) que conviene tomar cuando la mazmorra sea más larga, no antes.

Al enganchar combate (tecla E cerca de cualquier enemigo vivo), `main.cpp` reúne a TODOS los enemigos vivos con la misma `Sala()` que el más cercano y arma un único `CombatEncounter` con ese grupo — un combate por sala, no por enemigo individual. `CombatEncounter` pasó de guardar una referencia a un solo `Enemy` a guardar `std::vector<Enemy*>`; el orden de turnos ahora intercala a los 4 del party con TODOS los enemigos del grupo según velocidad (antes era "los 4 del party + 1 enemigo"), y la victoria (`FaseCombate::Ganado`) requiere que **todos** los enemigos del grupo estén derrotados, no solo el primero.

Como puede haber más de un enemigo vivo a la vez, las acciones del aliado en turno (ataque básico, habilidad de rol) necesitan saber a cuál de ellos apuntar: `CombatEncounter` mantiene un `objetivoActual_` (índice dentro del grupo), que arranca en el primer enemigo vivo y se recalcula solo si el objetivo actual muere (`AsegurarObjetivoValido`). El jugador puede cambiarlo manualmente con **TAB** durante su turno (`CiclarObjetivo`), y la UI de combate (`combat_ui.cpp`) marca la ficha del objetivo actual con un borde dorado y "▶" delante del nombre, separado del resaltado rojo que indica de quién es el turno.

El primer encuentro implementado (antes de la mazmorra procedural) fue contra un único enemigo fijo, para validar el ciclo completo (enganchar combate, elegir acciones, terminar en victoria o derrota) antes de construir generación de mazmorra y de encuentros reales; ese camino sigue intacto (`CombatEncounter` con un vector de un solo elemento funciona igual que antes), pero ahora es el caso particular de un grupo de tamaño 1, no un tipo de dato distinto.

### Qué pasa al ganar o perder

Al ganar (`FaseCombate::Ganado`), se vuelve a la exploración tal cual — el party sigue con el HP/recurso que le quedó, y el enemigo derrotado queda marcado (`Enemy::Vencido()`) y no se puede volver a enganchar.

Al perder (`FaseCombate::Perdido`) se ve una pantalla de Game Over distinta a la de victoria, y al apretar una tecla el party **revive**: `Character::Revivir()` restaura HP y recurso al máximo y limpia todos los efectos de combate, y `Party::ReiniciarFormacion()` teletransporta a todo el party de vuelta al punto de partida de la mazmorra (y limpia el rastro de formación, para que los seguidores no "corran" desde el rastro viejo). Sin esto, perder dejaba al party con HP 0 para siempre — el próximo combate terminaba en derrota instantánea sin que el jugador pudiera hacer nada (softlock).

### Feedback visual de combate

Hasta esta vuelta, lo único que comunicaba lo que pasaba en combate era el log de texto — un golpe, una curación y un fallo se veían todos igual (una línea más abajo del todo). Se agregó un segundo canal, puramente visual: numeritos flotantes de daño/curación/fallo sobre la ficha correspondiente, más un flash breve en quien recibe un golpe.

**Capa de juego (`game/combat.h`/`.cpp`), datos estructurados, sin nada de animación ni de tiempo real:**
- `enum class TipoEventoVisual { Dano, Curacion, Fallo }` y `struct EventoVisual { esAliado, indice, tipo, monto, critico }` — el "qué pasó", sin ningún dato de posición en pantalla ni de temporizado (eso es 100% de la capa de render).
- `CombatEncounter::UltimosEventos()` (el `std::vector<EventoVisual>` del paso más reciente) y `SecuenciaEventos()` (un contador que se incrementa cada vez que ese vector se repuebla, **incluso si queda vacío**) — mismo patrón de "dirty flag" que ya usaba `lootRepartido`, para que quien consume los datos (el render) sepa si ya vio la tanda actual o hay una nueva sin mostrar, sin tener que comparar el contenido del vector.
- `RegistrarEventos(std::vector<EventoVisual>)`, privado, es el único punto que escribe esos dos campos — se llama desde los tres lugares donde se resuelve una acción: `AccionAtaqueBasico()` (un evento), `AccionHabilidadDeRol()` (un evento — el aliado que se sana si es Soporte, o el enemigo atacado en los otros tres roles; nada si la habilidad no se pudo usar por falta de recurso, para no generar un "paso" fantasma), y `Actualizar()` (el turno del enemigo, que junta un `std::vector` local a medida que resuelve — puede ser más de un golpe con el Doble Tajo del jefe — y lo registra una sola vez al final, en cada punto de salida de la función incluidos los `return` tempranos por fin de combate).

**Capa de render (`render/combat_ui.cpp`), toda la animación/temporizado, nada de reglas de juego:**
- Estado a nivel de archivo (namespace anónimo): `g_numerosFlotantes` (posición, edad, texto ya formateado, color, tamaño) y `g_flashesActivos` (a quién, cuánto le queda), más `g_ultimaSecuenciaVista` para detectar cuándo `SecuenciaEventos()` cambió desde el frame anterior.
- Cada numerito nace justo **encima** del borde superior de la ficha (no adentro — se probó adentro primero y tapaba el nombre/turno que ya se dibuja ahí, ver más abajo), sube 42px/seg y se desvanece en 1 segundo. Un golpe (`Dano`) además dispara un flash: un rectángulo rojo semitransparente sobre toda la ficha que decae en 0.22s. Crítico usa texto más grande y dorado en vez de rojo (`"-N!"` en vez de `"-N"`); `Fallo` es un texto gris fijo (`"FALLO"`) sin flash ni monto.
- Las posiciones de las fichas (`Rectangle` por aliado/enemigo) se capturan en el mismo bucle de `DibujarCombate` que ya las dibujaba (antes no se guardaban en ningún lado, se calculaban y usaban al toque) — así el spawn de un numerito no duplica el cálculo de layout, usa exactamente donde se dibujó la ficha ese frame.

**Bug encontrado y corregido durante la verificación visual** (con un harness de prueba que fuerza golpes/curaciones/críticos/fallos y saca capturas bajo Xvfb, después borrado — no se shipea): el primer diseño intentaba detectar "empezó un combate nuevo" comparando `SecuenciaEventos()` contra el último valor visto (si bajó, se asumía combate nuevo) y, como refuerzo, comparando la dirección de memoria del `CombatEncounter&`. Ninguna de las dos señales alcanza por sí sola: dos combates distintos pueden coincidir en el mismo número de secuencia (si ambos llevan resuelto un solo paso), y un `CombatEncounter` nuevo puede perfectamente reusar la memoria de uno recién destruido (confirmado en la práctica: el harness lo reprodujo con dos `CombatEncounter` locales en bloques `{}` consecutivos). El resultado, sin la corrección, era un numerito viejo (de un combate ya terminado) reapareciendo superpuesto sobre el primer evento del combate siguiente. La solución: en vez de adivinar, `main.cpp` avisa explícitamente — `ui::ReiniciarFeedbackVisual()` se llama una sola vez, justo después de `encuentro = std::make_unique<CombatEncounter>(...)`, y limpia `g_numerosFlotantes`/`g_flashesActivos`/`g_ultimaSecuenciaVista` sin ambigüedad. Deja además una lección para el resto del proyecto: si en algún punto se necesita otro estado "por combate" en la capa de render, seguir este mismo patrón (reset explícito desde quien crea el `CombatEncounter`) en vez de inferirlo.

Verificado con capturas de pantalla bajo Xvfb (un harness que fuerza cada uno de los cuatro casos — golpe normal, curación, crítico, fallo — sobre un `CombatEncounter` real y saca screenshots en distintos puntos de la animación) más una corrida completa del juego (`cmake --build build`, smoke test bajo Xvfb) antes de shippear.

### Sonido

El juego era 100% mudo hasta esta vuelta. Se agregó música de fondo (dos pistas en loop) y efectos de combate, con la misma filosofía de capas que el resto del proyecto: `render::Audio` (`render/audio.h`/`.cpp`) es una clase RAII — mismo patrón que `render::Renderer` con la ventana — que en el constructor llama `InitAudioDevice()` y carga todos los assets, y en el destructor los descarga y cierra el dispositivo.

**De dónde salen los assets**: en vez de samples o música con licencia de terceros (fuera de alcance para un prototipo hecho enteramente por código), los 8 clips de `assets/audio/` son sintetizados por `tools/generar_audio.py` (numpy para la síntesis, scipy solo para escribir el WAV) — tonos simples con envolventes exponenciales tipo "pluck" para los efectos (`golpe`, `critico`, `curacion`, `fallo`, `victoria`, `derrota`), y dos loops musicales (`musica_exploracion`, 8s, calma, Re menor; `musica_combate`, 4s, más urgente, con un ostinato de bajo y percusión de ruido) armados como una serie de notas discretas cuya cola ya decayó a silencio antes del punto de loop, más un fade de seguridad de 15-20ms en los bordes — así el loop no clickea sin tener que cuadrar frecuencias a ciclos enteros. Es un placeholder deliberado: mismo nombre de archivo y misma carpeta, así que reemplazarlos por música/SFX definitivos más adelante no toca una línea de código.

**Reutilización del sistema de eventos visuales**: en vez de armar un camino de datos paralelo para el audio, `Audio::ProcesarEventos(const CombatEncounter&)` consume exactamente `UltimosEventos()`/`SecuenciaEventos()` — la misma data que ya alimenta los numeritos flotantes (ver "Feedback visual de combate" arriba) — y reproduce `sonidoGolpe_`/`sonidoCritico_`/`sonidoCuracion_`/`sonidoFallo_` según `TipoEventoVisual` y el flag `critico`. Esto significa que agregar sonido no le agregó ningún dato nuevo a `game/combat.h`: la capa de juego ya exponía todo lo necesario para el feedback visual, y el audio es "otro consumidor" de la misma data estructurada.

Por la misma razón que `ui::ReiniciarFeedbackVisual()` (ver el bug documentado arriba: `SecuenciaEventos()` por sí sola no distingue con certeza "combate nuevo" de "mismo combate, mismo paso"), `Audio::ReiniciarCombate()` se llama en el mismo punto exacto de `main.cpp`, justo después de crear el `CombatEncounter` — sin esto, un combate nuevo que coincide en secuencia con el anterior podría perderse su primer efecto de sonido. Verificado con un harness dedicado (50 `CombatEncounter` efímeros consecutivos en bloques `{}`, el mismo escenario adversarial que rompió el sistema visual) sin crashear.

**Música**: `Audio::Actualizar(bool enCombate)`, llamada una vez por frame sin importar el estado del juego (raylib necesita `UpdateMusicStream` todos los frames para que el streaming no se corte), cambia de pista — para/arranca la música, corte directo, sin crossfade — apenas `enCombate` difiere del frame anterior.

**Victoria/derrota**: `Audio::ReproducirVictoria()`/`ReproducirDerrota()` se llaman una sola vez al entrar a `Ganado`/`Perdido`, reusando el mismo patrón de flag "ya hecho una vez" que `lootRepartido` (para `Ganado`) y un flag nuevo, `derrotaSonada` (para `Perdido`, que hasta ahora no necesitaba ningún flag porque no hacía nada mientras se veía la pantalla de Game Over salvo esperar una tecla).

**Sin hardware de audio**: si `InitAudioDevice()` falla (probado en este mismo sandbox de desarrollo, que no tiene tarjeta de sonido — ALSA tira una serie de warnings pero raylib no crashea), `Audio` lo detecta (`IsAudioDeviceReady()`) y todos sus métodos se vuelven no-ops — el juego sigue jugable en silencio en vez de crashear al arrancar. En la máquina real del usuario (con audio de verdad) este camino no debería activarse nunca.

### Sistema de inventario y loot

Inventario único y compartido por todo el party (`game::Inventory`, dueño: `game::Party`), no uno por personaje — evita tener que decidir "a quién le doy este item" al momento de recogerlo, esa decisión se toma recién al usarlo. Apila items iguales en una sola entrada con cantidad (`PilaItem`, comparado por nombre) en vez de ocupar un slot por unidad.

Cada `game::Item` (catálogo fijo en `item.h`/`item.cpp`, definición del struct en `item_types.h`) es de uno de dos tipos:
- **Consumible** (`EfectoItem::CurarVida` / `CurarRecurso`), resuelto por `UsarItem`: tira dados (`RollDados(dados, caras, bono)`) y aplica el resultado a HP o recurso, clampeado al máximo (`AplicarCuracion` para vida; un `std::min` manual para recurso, que no tiene una función compartida como la curación). Se gasta una unidad de la pila al usarse (`Inventory::Usar`).
- **Mejora** (`MejorarAtaque` / `MejorarDefensa` / `MejorarVelocidad` / `MejorarVidaMaxima`): suma `bono` directo a la stat correspondiente, **permanente**. A diferencia de los Consumibles, **no se "usa" instantáneo — se equipa** (`Character::Equipar` / `Inventory::Equipar`), ver la subsección de abajo. `UsarItem` rechaza explícitamente cualquier item que no sea `TipoItem::Consumible` (devuelve `exitoso=false` sin tocar stats), como red de seguridad si algo lo llama por error con una Mejora.

El catálogo de Mejoras tiene 4 piezas, dos por ranura, para que equipar sea una elección real y no un solo camino obligado: **Arma** es Piedra de Fuerza (`MejorarAtaque`, +1 ataque) o **Daga Veloz** (`MejorarVelocidad`, +10 velocidad — actuar más seguido/romper empates de turno en vez de pegar más fuerte); **Accesorio** es Amuleto de Protección (`MejorarDefensa`, +1 defensa) o **Talismán de Vitalidad** (`MejorarVidaMaxima`, +5 vida máxima — aguantar más golpes en vez de que te conecten menos). `MejoraAleatoria()` en `item.cpp` sortea entre las 4 con probabilidad pareja (25% cada una) y centraliza el sorteo para que `ItemAleatorioDeCofre` y `TirarLootDeEnemigo` no dupliquen la lógica. `Character::Equipar` cubre `MejorarVidaMaxima` como caso especial: al aplicarlo, la vida actual sube junto con el máximo (se siente como una mejora real al toque, no solo una barra más grande para rellenar después); al revertirlo (reemplazo por otra Mejora en esa ranura), baja el máximo y solo recorta la vida actual si de verdad quedó por encima del nuevo máximo — no le resta el bono a ciegas, porque la vida actual pudo haber cambiado por combate desde que se equipó.

`ResultadoUsoItem` devuelve si se pudo usar, el valor numérico aplicado y un texto ya formateado (p.ej. "Poción de Curación Menor sobre Milo: recupera 6 de vida.") pensado para mostrarse directo como mensaje flotante, sin que la capa de render tenga que reconstruir la frase.

Dos fuentes de items:
- **Cofres** (`game::Cofre`: posición + `Item` fijo + `abierto`): objetos estáticos ubicados por `main.cpp` en la generación de la mazmorra — uno garantizado en la sala inicial (para que el sistema se vea sin depender del azar) y, por cada sala con contenido, un 40% (`Roll(10) <= kChanceCofrePorSalaDe10`) de tener uno adicional, en la esquina de la sala (lejos del grupo de enemigos, que suele estar cerca del centro). `ItemAleatorioDeCofre()` decide el contenido con una tabla de probabilidad fija (50% poción, 30% elixir, 20% una mejora al azar).
- **Botín de combate** (`TirarLootDeEnemigo(TipoEnemigo)`): al entrar en `FaseCombate::Ganado`, `main.cpp` tira una vez por cada enemigo que participó del encuentro (`CombatEncounter::Enemigos()`), con una tabla de drop por tipo — Esqueleto Errante 60%, Rata Gigante 40% (el más débil, tabla más floja a propósito), Bandido Aturdidor 70% con 1/4 de esas veces siendo una mejora permanente en vez de un consumible (recompensa extra por ser el enemigo más duro de pelear). Un flag `lootRepartido` en `main()` asegura que el botín se reparta una sola vez por combate, ya que `Ganado` se re-evalúa todos los frames hasta que el jugador aprieta una tecla.

**Interacción unificada con [E]**: antes, `main.cpp` solo buscaba el enemigo vivo más cercano; ahora busca el interactuable más cercano entre enemigos vivos y cofres sin abrir (`kDistanciaInteraccion`, el mismo radio de antes), y el cartel de abajo ("[E] Atacar" / "[E] Abrir cofre") depende de cuál sea. Un enemigo cercano siempre gana la prioridad sobre un cofre a la misma distancia — no debería haber ambigüedad real en la práctica, ya que los cofres se ubican lejos de los grupos de enemigos a propósito.

**Ranuras de equipo** (`RanuraEquipo` en `item_types.h`; `Character::Equipar`/`Arma()`/`Accesorio()` en `character.h`/`.cpp`): cada `Item` de tipo Mejora declara a qué ranura va (`Arma` para `MejorarAtaque`, `Accesorio` para `MejorarDefensa`), y cada `Character` tiene como mucho un `ItemEquipado` por ranura (`{bool ocupado; Item item;}`). El motivo del cambio: antes una Mejora se aplicaba directo y se perdía como concepto (no había forma de ver qué tenía puesto un personaje, ni nada que impidiera "equiparle 5 Piedras de Fuerza" al mismo, cada una sumando +1 de ataque sin límite). `Character::Equipar(nuevo)` resuelve la ranura según `nuevo.ranura`, si ya había algo puesto le revierte el `bono` a la stat (resta exacta, no un reset a un valor base — no hace falta separar "stats base" de "stats con equipo" porque el `bono` que se sumó es el mismo que se resta), aplica el `bono` del nuevo, dejarlo equipado, y devuelve el `ItemEquipado` anterior (con `ocupado=false` si la ranura estaba vacía) para que el llamador decida qué hacer con él. `Inventory::Equipar(indice, personaje)` es ese llamador: saca una unidad de la pila, llama a `Character::Equipar`, y si vuelve algo ocupado lo reinserta con `Agregar` — así lo reemplazado no se pierde, vuelve al inventario compartido. `Inventory::Usar` e `Inventory::Equipar` se rechazan mutuamente por tipo (`Usar` solo actúa sobre Consumibles, `Equipar` solo sobre Mejoras), y `main.cpp` decide cuál llamar mirando `pilas[indice].item.tipo` antes de procesar la tecla numérica.

**Pantalla de inventario** (**I** la abre/cierra, `render/inventory_ui.cpp`): reemplaza por completo el frame de exploración mientras está abierta (congela movimiento e interacción — `main.cpp` solo procesa TAB/números en ese estado). Muestra una ficha por miembro del party (nombre, rol por color, HP/recurso, Arma/Accesorio equipados — "-" si la ranura está vacía —, y un borde dorado + "<" en quien es el objetivo actual) y la lista de pilas de items con un `[N]` al lado de cada una; las de tipo Mejora llevan además un tag `[Equipar: Arma]` / `[Equipar: Accesorio]` en dorado, para que quede claro que van a reemplazar lo que haya en esa ranura en vez de sumarse sin límite. **TAB** cicla el objetivo entre **todos** los miembros, vivos o no — a propósito, para poder curar/revivir a un caído sin tener que cerrar el inventario y reordenar nada. **1-9** llama a `Inventory::Usar` o `Inventory::Equipar` según el tipo del item en esa fila. El panel de party expandido (`render/ui.cpp::DibujarPanelExpandido`, TAB en exploración) también muestra el Arma/Accesorio de cada uno en una línea aparte, para poder chusmear el equipo sin abrir el inventario.

**Mensajes flotantes**: un `std::string mensajeFlotante` + `float timerMensaje` en `main()` (no en el renderer — la capa de lógica decide el texto, `renderer.cpp` solo lo dibuja) muestran por `kDuracionMensaje` (3s) el resultado de abrir un cofre, usar/equipar un item o el resumen de botín tras un combate, en un cartel arriba del prompt de interacción.

## Menú de inicio

Hasta esta vuelta el ejecutable arrancaba directo en la mazmorra, sin ningún paso previo. Se agregó una pantalla de título simple: nombre del juego + dos opciones, **Jugar** y **Salir**, navegables con las flechas (o W/S) y confirmadas con ENTER o ESPACIO. Es deliberadamente mínima — no hay más opciones porque no hay nada más que ofrecer todavía (no existe guardado de partida, así que no puede haber un "Continuar"; ver Roadmap).

**Un estado más en la máquina de estados de `main.cpp`**: `EstadoJuego` pasó de `{Exploracion, Combate}` a `{MenuInicio, Exploracion, Combate}`, con `MenuInicio` como estado inicial. No hizo falta reordenar nada de la inicialización: la mazmorra, el party y los enemigos ya se generaban enteros antes de entrar al bucle principal (`while (!WindowShouldClose())`), así que el menú simplemente se antepone como un estado más que se resuelve antes de pasar a `Exploracion` — sin necesidad de "esperar" a generar nada cuando el jugador aprieta Jugar.

**Capa de render pura** (`render/menu_ui.h`/`.cpp`, namespace `ui`, mismo patrón que `combat_ui.cpp`): `DibujarMenuInicio(anchoVentana, altoVentana, opcionSeleccionada)` solo dibuja — no lee input ni decide nada, `main.cpp` es quien mueve `opcionSeleccionada` y decide qué hacer al confirmar. El número de opciones (`kNumOpcionesMenuInicio`) se expone desde el header en vez de duplicar la lista en `main.cpp`, para que ciclar la selección con el módulo (`(opcion + 1) % kNumOpcionesMenuInicio`) y dibujar las opciones usen siempre el mismo número — si el día de mañana se agrega "Continuar", alcanza con tocar un solo lugar.

**Detalle visual**: la mazmorra ya generada se dibuja "congelada" de fondo (reusando `Renderer::DibujarEscenarioSinUI`, el mismo recurso que ya usaba la pantalla de combate para "congelar" la escena detrás del overlay) y encima va un overlay oscuro semitransparente con el título, subtítulo y las dos opciones. Las dos opciones se dibujan siempre al mismo tamaño de fuente (solo cambia el color y el prefijo "> "/"  ") — un tamaño de fuente distinto para la opción resaltada hacía que el texto centrado saltara de posición cada vez que el jugador movía la selección, un detalle chico pero notorio al jugarlo.

**"Salir" no hace falta que sea nada especial**: alcanza con cortar el `while` principal (un `break` normal) — el resto de `main()` (el `return 0` de siempre) ya deja que `Renderer` y `Audio` se desarmen solos por RAII al salir de scope, el mismo camino que ya se ejecutaba al cerrar la ventana con la X. No hizo falta ningún método nuevo de cierre.

Verificado bajo Xvfb con un `xdotool` real (no solo un harness sintético): capturas de pantalla confirmando el título, el cambio de selección al mover el cursor entre las dos opciones (sin salto de posición), y las dos confirmaciones — ENTER lleva a explorar la mazmorra ya generada (incluida una sala con pilares, visible de fondo en una de las capturas) y ENTER sobre "Salir" cierra la ventana con un `INFO: Window closed successfully` limpio en el log, sin warnings nuevos más allá de los ya conocidos de ALSA en este sandbox sin audio. Además, la suite completa de `test_game_layer_bin` (3 corridas) sigue pasando sin cambios, porque el menú no toca ninguna lógica de juego — es puramente un estado más de la capa de render/flujo.

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
│       ├── inventory_ui.h/.cpp  # pantalla de inventario (tecla I)
│       ├── audio.h/.cpp         # musica + efectos, RAII (ver "Sonido" arriba)
│       └── menu_ui.h/.cpp       # pantalla de inicio (ver "Menú de inicio" arriba)
├── assets/
│   ├── sprites/
│   ├── audio/            # WAV generados por tools/generar_audio.py
│   └── tilesets/
├── tools/
│   └── generar_audio.py  # sintetiza los WAV de assets/audio/ (numpy/scipy)
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
13. ✅ **Curva de poder investigada**: se midió cuánto poder equipado (bonos de ataque/defensa de mejoras permanentes) acumula el party a lo largo de una run completa — con la duración actual (4 salas + jefe) llega al Capitán Bandido con menos de 1 punto de bono en promedio, sin efecto medible en la tasa de victoria contra él. Conclusión: no hay curva de poder que corregir por ahora — es un plus menor, no un factor de balance — ver "Balance: curva de poder de las mejoras permanentes" arriba. Sin acción tomada; queda como palanca disponible (subir drop rates) si el looteo necesita sentirse más impactante el día que la mazmorra sea más larga.
14. ✅ **Catálogo de Mejoras ampliado**: de 2 a 4 piezas — Daga Veloz (Arma, +10 velocidad) y Talismán de Vitalidad (Accesorio, +5 vida máxima) se suman a Piedra de Fuerza y Amuleto de Protección, dándole a cada ranura dos sabores reales para elegir (más daño o actuar más seguido; esquivar más o aguantar más golpes) en vez de un solo camino obligado — ver "Sistema de inventario y loot" arriba.
15. ✅ **Feedback visual de combate**: numeritos flotantes de daño (rojo, dorado y más grande en crítico), curación (verde) y fallo ("FALLO", gris) sobre la ficha correspondiente, que suben y se desvanecen en 1 segundo, más un flash rojo breve en quien recibe un golpe. La capa de juego solo expone los eventos como datos estructurados (`EventoVisual`/`SecuenciaEventos()`); toda la animación vive en `combat_ui.cpp` — ver "Feedback visual de combate" arriba, incluida una corrección de un bug real (numeritos de un combate anterior reapareciendo en el siguiente) encontrado durante la verificación visual.
16. ✅ **Sonido**: música de fondo (loop de exploración y de combate, cambia sola con `render::Audio::Actualizar`) y efectos de golpe/crítico/curación/fallo/victoria/derrota, reutilizando la misma data de `EventoVisual`/`SecuenciaEventos()` del feedback visual en vez de un camino paralelo. Todos los clips son sintetizados por código (`tools/generar_audio.py`), no assets con licencia de terceros. Sigue andando en silencio (no-op) si la máquina no tiene dispositivo de audio — ver "Sonido" arriba.
17. ✅ **Variedad de formas de sala**: además de los 4 templates rectangulares de siempre (que variaban en tamaño, no en forma), se agregaron dos formas nuevas — una sala en L (14x14 con una esquina recortada) y una sala con 4 pilares (12x12) — elegidas al azar junto con las 4 anteriores. Se implementó recortando tiles del set de piso antes de calcular las paredes (arquitectura piso-primero, ver "Sistema de mazmorras" más arriba), así que no hizo falta tocar cámara, grilla ni colisión; las dos formas garantizan piso libre tanto en el centro del bounding box (donde arranca el grupo de enemigos) como en la esquina superior izquierda (donde va el cofre de la sala). Verificado con un fuzz test de 300 mazmorras (1500 salas, 7500 chequeos entre las 5 corridas) sin ningún fallo — ver "Variedad de formas de sala" arriba.
18. ✅ **Menú de inicio**: pantalla de título antes de largar a explorar, con las opciones Jugar/Salir navegables por teclado — un estado más (`EstadoJuego::MenuInicio`) antepuesto a los dos que ya existían, sin tocar la inicialización de la mazmorra/party/enemigos. Sin opción de Continuar todavía porque no hay guardado de partida — ver "Menú de inicio" arriba.
19. Pendiente: seguir iterando sobre contenido (más tipos de enemigo comunes o una mazmorra más larga), o encarar el guardado de partida (que además habilitaría un "Continuar" real en el menú), antes de evaluar el salto a mobile (build de Android vía NDK).

## Notas sobre la futura migración a Unreal

Cuando llegue el momento, lo que se traslada más directo es la capa de lógica de juego (si se mantuvo separada de raylib como se describe arriba): stats, reglas de combate, generación de mazmorras. Lo que se descarta o rehace por completo es la capa de presentación (sprites, tilemap 2D casero, la cámara) — en Unreal eso pasa a resolverse con sus propios sistemas (Blueprints/C++, Niagara, el editor de niveles, etc.), y ahí también se decide si el salto es a 2D dentro de Unreal (Paper2D) o directamente a 3D.
