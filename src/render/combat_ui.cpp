#include "combat_ui.h"
#include "raylib.h"
#include <algorithm>
#include <cstdio>
#include <vector>

namespace ui {

namespace {

// Mismo criterio que ui.cpp: el retrato reusa el sprite de personaje del
// mapa a un tamaño chico, en vez de un circulo de color.
constexpr float kEscalaRetrato = 1.0f;

// Dibuja una lista corta de "tags" de texto con los efectos activos (por
// ejemplo "Envenenado x2", "Escudo 5"), una debajo de otra.
void DibujarEfectos(const game::EstadoCombate& combate, int x, int y) {
    int filaY = y;
    for (const auto& efecto : combate.Efectos()) {
        char texto[64];
        if (efecto.tipo == game::TipoEfecto::Escudo) {
            // El Escudo no "dura" turnos: se consume por daño. Mostrar la
            // duracion interna (que usa un numero grande como centinela)
            // confundiria; en cambio se muestran los puntos que le quedan.
            std::snprintf(texto, sizeof(texto), "%s (%d pts)", game::NombreEfecto(efecto.tipo), efecto.magnitud);
        } else {
            std::snprintf(texto, sizeof(texto), "%s (%d)", game::NombreEfecto(efecto.tipo), efecto.duracionTurnos);
        }
        DrawText(texto, x, filaY, 12, Color{ 230, 200, 120, 255 });
        filaY += 14;
    }
}

void DibujarBarra(int x, int y, int ancho, int alto, float ratio, Color colorLleno, Color colorFondo) {
    DrawRectangle(x, y, ancho, alto, colorFondo);
    DrawRectangle(x, y, (int)(ancho * ratio), alto, colorLleno);
    DrawRectangleLines(x, y, ancho, alto, Color{ 20, 20, 20, 255 });
}

void DibujarFichaAliado(const game::Character& personaje, bool esSuTurno, int x, int y, int ancho,
                         const render::SpriteSet& sprites) {
    const auto& stats = personaje.GetStats();
    int alto = stats.recursoMax > 0 ? 78 : 60;

    Color fondo = esSuTurno ? Color{ 45, 45, 30, 230 } : Color{ 20, 20, 25, 200 };
    Color borde = esSuTurno ? Color{ 230, 200, 90, 255 } : Color{ 80, 80, 90, 255 };
    DrawRectangle(x, y, ancho, alto, fondo);
    DrawRectangleLines(x, y, ancho, alto, borde);

    render::DibujarSpriteCentrado(sprites.Personaje(personaje.Rol()), Vector2{ (float)(x + 18), (float)(y + 18) }, kEscalaRetrato);

    char nombre[48];
    std::snprintf(nombre, sizeof(nombre), "%s%s", personaje.Nombre().c_str(), esSuTurno ? " <-- turno" : "");
    DrawText(nombre, x + 36, y + 8, 15, RAYWHITE);
    DrawText(game::RoleName(personaje.Rol()), x + 36, y + 26, 12, LIGHTGRAY);

    if (!personaje.EstaVivo()) {
        DrawText("CAIDO", x + ancho - 70, y + 8, 14, Color{ 200, 60, 60, 255 });
    }

    float ratioHp = stats.hpMax > 0 ? (float)stats.hp / (float)stats.hpMax : 0.0f;
    DibujarBarra(x + 36, y + 42, ancho - 50, 8, ratioHp, Color{ 200, 60, 60, 255 }, Color{ 60, 20, 20, 255 });
    char textoHp[32];
    std::snprintf(textoHp, sizeof(textoHp), "HP %d/%d", stats.hp, stats.hpMax);
    DrawText(textoHp, x + 36, y + 52, 10, LIGHTGRAY);

    if (stats.recursoMax > 0) {
        float ratioRecurso = (float)stats.recurso / (float)stats.recursoMax;
        DibujarBarra(x + 36, y + 62, ancho - 50, 6, ratioRecurso, Color{ 90, 140, 220, 255 }, Color{ 25, 35, 55, 255 });
    }

    DibujarEfectos(personaje.Combate(), x + ancho - 90, y + 4);
}

// Dibuja la ficha de un enemigo. 'esSuTurno' resalta cuando la IA lo esta
// jugando; 'esObjetivo' resalta (con otro color) cuando es a quien le van a
// pegar las acciones del aliado en turno — con mas de un enemigo a la vez,
// el jugador necesita saber a cual le esta apuntando.
void DibujarFichaEnemigo(const game::Enemy& enemigo, bool esSuTurno, bool esObjetivo, int x, int y, int ancho) {
    int alto = 70;

    Color fondo = esSuTurno ? Color{ 45, 25, 25, 230 } : Color{ 20, 20, 25, 200 };
    Color borde = esSuTurno ? Color{ 220, 90, 90, 255 } : (esObjetivo ? Color{ 230, 200, 90, 255 } : Color{ 80, 80, 90, 255 });
    if (enemigo.Vencido()) {
        fondo = Color{ 15, 15, 18, 160 };
    }
    DrawRectangle(x, y, ancho, alto, fondo);
    DrawRectangleLines(x, y, ancho, alto, borde);
    if (esObjetivo && !esSuTurno && !enemigo.Vencido()) {
        // Borde extra para que el marcador de objetivo se note incluso con
        // varias fichas una debajo de otra.
        DrawRectangleLines(x - 1, y - 1, ancho + 2, alto + 2, Color{ 230, 200, 90, 180 });
    }

    char nombre[80];
    std::snprintf(nombre, sizeof(nombre), "%s%s%s", esObjetivo && !enemigo.Vencido() ? "> " : "",
                  enemigo.Nombre().c_str(), esSuTurno ? " <-- turno" : "");
    DrawText(nombre, x + 12, y + 8, 16, enemigo.Vencido() ? GRAY : RAYWHITE);

    if (enemigo.Vencido()) {
        DrawText("DERROTADO", x + ancho - 110, y + 8, 14, Color{ 140, 140, 140, 255 });
        return;
    }

    const auto& stats = enemigo.GetStats();
    float ratioHp = stats.hpMax > 0 ? (float)stats.hp / (float)stats.hpMax : 0.0f;
    DibujarBarra(x + 12, y + 32, ancho - 24, 10, ratioHp, Color{ 200, 60, 60, 255 }, Color{ 60, 20, 20, 255 });
    char textoHp[32];
    std::snprintf(textoHp, sizeof(textoHp), "HP %d/%d", stats.hp, stats.hpMax);
    DrawText(textoHp, x + 12, y + 46, 12, LIGHTGRAY);

    DibujarEfectos(enemigo.Combate(), x + ancho - 100, y + 4);
}

// --- Feedback visual (numeritos flotantes + flash al recibir un golpe) ---
//
// Se guarda como estado a nivel de archivo (solo hay un combate en pantalla
// a la vez) en vez de vivir en game::CombatEncounter, para respetar la
// separacion game/render: la capa de juego solo expone datos estructurados
// (EventoVisual/SecuenciaEventos), y toda la animacion/temporizacion vive
// aca. Se detectan eventos nuevos comparando SecuenciaEventos() contra el
// ultimo valor visto; si ese valor BAJA (un CombatEncounter nuevo empieza su
// cuenta en 0) se descartan las animaciones que quedaban del combate previo
// en vez de intentar reproducirlas.

struct NumeroFlotante {
    float x = 0.0f;
    float y = 0.0f;
    float edad = 0.0f;
    char texto[16] = "";
    Color color = WHITE;
    int tamano = 20;
};

struct FlashActivo {
    bool esAliado = false;
    int indice = -1;
    float restante = 0.0f;
};

std::vector<NumeroFlotante> g_numerosFlotantes;
std::vector<FlashActivo> g_flashesActivos;
int g_ultimaSecuenciaVista = -1;
// Identidad del CombatEncounter del frame anterior: SecuenciaEventos() por
// si sola no alcanza para detectar "empezo un combate nuevo": un encuentro
// nuevo puede llegar a cualquier numero de secuencia (si dos encuentros
// distintos llevan resuelto un solo paso, los dos muestran secuencia 1), y
// comparar la direccion del encuentro tampoco alcanza (un CombatEncounter
// nuevo puede perfectamente reusar la memoria de uno recien destruido). Por
// eso quien crea el encuentro (main.cpp) llama a ReiniciarFeedbackVisual()
// una vez, de forma explicita, apenas lo crea — ver combat_ui.h.

constexpr float kDuracionNumeroFlotante = 1.0f;   // segundos que vive un numerito
constexpr float kVelocidadSubidaNumero = 42.0f;   // px/seg que sube mientras vive
constexpr float kDuracionFlash = 0.22f;           // segundos que dura el flash de golpe

void SpawnEventosVisuales(const std::vector<game::EventoVisual>& eventos,
                           const std::vector<Rectangle>& rectAliados,
                           const std::vector<Rectangle>& rectEnemigos) {
    for (const auto& ev : eventos) {
        const std::vector<Rectangle>& rects = ev.esAliado ? rectAliados : rectEnemigos;
        if (ev.indice < 0 || ev.indice >= (int)rects.size()) continue;
        const Rectangle& r = rects[ev.indice];

        NumeroFlotante n;
        n.x = r.x + r.width * 0.5f;
        // Justo encima del borde superior de la ficha (no dentro) para no
        // taparse con el nombre/turno que ya se dibuja ahi.
        n.y = r.y - 6.0f;
        switch (ev.tipo) {
            case game::TipoEventoVisual::Dano:
                std::snprintf(n.texto, sizeof(n.texto), "-%d%s", ev.monto, ev.critico ? "!" : "");
                n.color = ev.critico ? Color{ 255, 210, 80, 255 } : Color{ 230, 70, 70, 255 };
                n.tamano = ev.critico ? 26 : 20;
                g_flashesActivos.push_back(FlashActivo{ ev.esAliado, ev.indice, kDuracionFlash });
                break;
            case game::TipoEventoVisual::Curacion:
                std::snprintf(n.texto, sizeof(n.texto), "+%d", ev.monto);
                n.color = Color{ 110, 220, 140, 255 };
                n.tamano = 20;
                break;
            case game::TipoEventoVisual::Fallo:
                std::snprintf(n.texto, sizeof(n.texto), "FALLO");
                n.color = Color{ 190, 190, 190, 255 };
                n.tamano = 16;
                break;
        }
        g_numerosFlotantes.push_back(n);
    }
}

// Bumpea las animaciones un frame y, si SecuenciaEventos() cambio desde el
// ultimo frame, hace nacer los numeritos/flash correspondientes al paso
// nuevo (usando las posiciones de ficha ya calculadas ese mismo frame).
void ActualizarFeedbackVisual(game::CombatEncounter& encuentro, float deltaSeconds,
                               const std::vector<Rectangle>& rectAliados,
                               const std::vector<Rectangle>& rectEnemigos) {
    int secuencia = encuentro.SecuenciaEventos();
    if (secuencia != g_ultimaSecuenciaVista) {
        SpawnEventosVisuales(encuentro.UltimosEventos(), rectAliados, rectEnemigos);
    }
    g_ultimaSecuenciaVista = secuencia;

    for (auto& n : g_numerosFlotantes) n.edad += deltaSeconds;
    g_numerosFlotantes.erase(
        std::remove_if(g_numerosFlotantes.begin(), g_numerosFlotantes.end(),
                        [](const NumeroFlotante& n) { return n.edad >= kDuracionNumeroFlotante; }),
        g_numerosFlotantes.end());

    for (auto& f : g_flashesActivos) f.restante -= deltaSeconds;
    g_flashesActivos.erase(
        std::remove_if(g_flashesActivos.begin(), g_flashesActivos.end(),
                        [](const FlashActivo& f) { return f.restante <= 0.0f; }),
        g_flashesActivos.end());
}

// Flash: un rectangulo rojo semitransparente encima de la ficha entera,
// cuya opacidad decae a lo largo de kDuracionFlash — se dibuja despues de
// las fichas (las tapa un poco) y antes de los numeritos.
void DibujarFlashes(const std::vector<Rectangle>& rectAliados, const std::vector<Rectangle>& rectEnemigos) {
    for (const auto& f : g_flashesActivos) {
        const std::vector<Rectangle>& rects = f.esAliado ? rectAliados : rectEnemigos;
        if (f.indice < 0 || f.indice >= (int)rects.size()) continue;
        const Rectangle& r = rects[f.indice];
        float ratio = f.restante / kDuracionFlash;
        if (ratio < 0.0f) ratio = 0.0f;
        unsigned char alpha = (unsigned char)(140 * ratio);
        DrawRectangle((int)r.x, (int)r.y, (int)r.width, (int)r.height, Color{ 255, 60, 60, alpha });
    }
}

void DibujarNumerosFlotantes() {
    for (const auto& n : g_numerosFlotantes) {
        float ratio = n.edad / kDuracionNumeroFlotante;
        if (ratio > 1.0f) ratio = 1.0f;
        float yOffset = -kVelocidadSubidaNumero * n.edad;
        unsigned char alpha = (unsigned char)(255 * (1.0f - ratio));
        Color c = n.color;
        c.a = alpha;
        int anchoTexto = MeasureText(n.texto, n.tamano);
        DrawText(n.texto, (int)(n.x - anchoTexto / 2), (int)(n.y + yOffset), n.tamano, c);
    }
}

} // namespace

void DibujarCombate(game::CombatEncounter& encuentro, int anchoVentana, int altoVentana, float deltaSeconds,
                     const render::SpriteSet& sprites) {
    // Fondo semitransparente para que se note que estamos "en combate".
    DrawRectangle(0, 0, anchoVentana, altoVentana, Color{ 10, 8, 15, 235 });

    // --- Party, columna izquierda ---
    int x = 24;
    int y = 24;
    int anchoFicha = 260;
    game::Character* enTurno = encuentro.AliadoEnTurno();
    std::vector<Rectangle> rectAliados;
    for (auto& personaje : encuentro.PartyRef().Miembros()) {
        bool esSuTurno = (enTurno == &personaje);
        int altoFicha = personaje.GetStats().recursoMax > 0 ? 78 : 60;
        rectAliados.push_back(Rectangle{ (float)x, (float)y, (float)anchoFicha, (float)altoFicha });
        DibujarFichaAliado(personaje, esSuTurno, x, y, anchoFicha, sprites);
        y += altoFicha + 10;
    }

    // --- Enemigos, columna derecha (uno o varios, uno debajo del otro) ---
    const auto& enemigos = encuentro.Enemigos();
    int anchoEnemigo = 280;
    int xEnemigo = anchoVentana - anchoEnemigo - 24;
    int yEnemigo = 24;
    game::Enemy* enemigoEnTurno = encuentro.EnemigoEnTurno();
    int indiceObjetivo = encuentro.IndiceObjetivo();
    std::vector<Rectangle> rectEnemigos;
    for (size_t i = 0; i < enemigos.size(); ++i) {
        const game::Enemy* enemigo = enemigos[i];
        bool esSuTurno = (enemigoEnTurno == enemigo);
        bool esObjetivo = (encuentro.Fase() == game::FaseCombate::TurnoAliado && (int)i == indiceObjetivo);
        int altoEnemigo = 70;
        rectEnemigos.push_back(Rectangle{ (float)xEnemigo, (float)yEnemigo, (float)anchoEnemigo, (float)altoEnemigo });
        DibujarFichaEnemigo(*enemigo, esSuTurno, esObjetivo, xEnemigo, yEnemigo, anchoEnemigo);
        yEnemigo += altoEnemigo + 10;
    }

    // --- Feedback visual: numeritos de daño/curacion + flash de golpe ---
    ActualizarFeedbackVisual(encuentro, deltaSeconds, rectAliados, rectEnemigos);
    DibujarFlashes(rectAliados, rectEnemigos);
    DibujarNumerosFlotantes();

    // --- Log de combate, abajo ---
    const auto& log = encuentro.Log();
    int altoLog = 150;
    int yLog = altoVentana - altoLog - 24;
    DrawRectangle(24, yLog, anchoVentana - 48, altoLog, Color{ 15, 15, 20, 220 });
    DrawRectangleLines(24, yLog, anchoVentana - 48, altoLog, Color{ 80, 80, 90, 255 });

    int maxLineas = (altoLog - 16) / 18;
    int desde = (int)log.size() > maxLineas ? (int)log.size() - maxLineas : 0;
    int filaY = yLog + 8;
    for (int i = desde; i < (int)log.size(); ++i) {
        DrawText(log[i].c_str(), 34, filaY, 14, RAYWHITE);
        filaY += 18;
    }

    // --- Menu de accion / mensaje de fin ---
    if (encuentro.Fase() == game::FaseCombate::TurnoAliado && enTurno != nullptr) {
        int enemigosVivos = 0;
        for (auto* e : enemigos) if (e->EstaVivo()) enemigosVivos++;

        char menu[200];
        if (enemigosVivos > 1) {
            std::snprintf(menu, sizeof(menu), "Turno de %s  -  [1] Atacar    [2] %s    [TAB] Cambiar objetivo",
                          enTurno->Nombre().c_str(), game::NombreHabilidadDeRol(enTurno->Rol()));
        } else {
            std::snprintf(menu, sizeof(menu), "Turno de %s  -  [1] Atacar    [2] %s",
                          enTurno->Nombre().c_str(), game::NombreHabilidadDeRol(enTurno->Rol()));
        }
        int anchoTexto = MeasureText(menu, 20);
        int xMenu = (anchoVentana - anchoTexto) / 2 - 16;
        DrawRectangle(xMenu, yLog - 44, anchoTexto + 32, 34, Color{ 45, 45, 30, 230 });
        DrawRectangleLines(xMenu, yLog - 44, anchoTexto + 32, 34, Color{ 230, 200, 90, 255 });
        DrawText(menu, xMenu + 16, yLog - 38, 20, Color{ 255, 235, 180, 255 });
    } else if (encuentro.Fase() == game::FaseCombate::TurnoEnemigo) {
        const char* texto = "El enemigo esta actuando...";
        int anchoTexto = MeasureText(texto, 20);
        DrawText(texto, (anchoVentana - anchoTexto) / 2, yLog - 38, 20, Color{ 220, 150, 150, 255 });
    } else if (encuentro.Fase() == game::FaseCombate::Ganado) {
        // El Capitan Bandido es el jefe de la mazmorra (unico enemigo de su
        // sala) — derrotarlo merece un cierre distinto al de un combate mas.
        bool esVictoriaFinal = encuentro.Enemigos().size() == 1
            && encuentro.Enemigos()[0]->Tipo() == game::TipoEnemigo::CapitanBandido;
        if (esVictoriaFinal) {
            const char* titulo = "¡MAZMORRA DESPEJADA!";
            int anchoTitulo = MeasureText(titulo, 40);
            DrawText(titulo, (anchoVentana - anchoTitulo) / 2, altoVentana / 2 - 130, 40, Color{ 230, 190, 80, 255 });
            const char* subtitulo = "El Capitan Bandido ha caido.";
            int anchoSub = MeasureText(subtitulo, 20);
            DrawText(subtitulo, (anchoVentana - anchoSub) / 2, altoVentana / 2 - 78, 20, Color{ 220, 220, 220, 255 });
            const char* prompt = "presiona cualquier tecla para continuar";
            int anchoPrompt = MeasureText(prompt, 18);
            DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana / 2 - 46, 18, Color{ 200, 200, 200, 255 });
        } else {
            const char* texto = "¡VICTORIA!  -  presiona cualquier tecla para continuar";
            int anchoTexto = MeasureText(texto, 26);
            DrawText(texto, (anchoVentana - anchoTexto) / 2, altoVentana / 2 - 100, 26, Color{ 140, 230, 140, 255 });
        }
    } else if (encuentro.Fase() == game::FaseCombate::Perdido) {
        const char* titulo = "GAME OVER";
        int anchoTitulo = MeasureText(titulo, 46);
        DrawText(titulo, (anchoVentana - anchoTitulo) / 2, altoVentana / 2 - 130, 46, Color{ 220, 60, 60, 255 });

        const char* subtitulo = "El party ha caido...";
        int anchoSub = MeasureText(subtitulo, 20);
        DrawText(subtitulo, (anchoVentana - anchoSub) / 2, altoVentana / 2 - 68, 20, Color{ 230, 150, 150, 255 });

        const char* prompt = "presiona cualquier tecla para revivir y volver al punto de partida";
        int anchoPrompt = MeasureText(prompt, 18);
        DrawText(prompt, (anchoVentana - anchoPrompt) / 2, altoVentana / 2 - 34, 18, Color{ 210, 210, 210, 255 });
    }
}

void ReiniciarFeedbackVisual() {
    g_numerosFlotantes.clear();
    g_flashesActivos.clear();
    g_ultimaSecuenciaVista = -1;
}

} // namespace ui
