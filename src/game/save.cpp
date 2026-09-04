#include "save.h"
#include <fstream>
#include <sstream>

namespace game {

namespace {

// Version del formato de archivo, en la primera linea. Si algun dia el
// formato cambia de forma incompatible (se agrega/saca un campo), subir
// este numero y rechazar versiones viejas en vez de intentar adivinar el
// formato — un archivo corrupto o de otra version simplemente hace que
// CargarPartida devuelva 'valido=false' (el jugador arranca partida nueva
// en vez de crashear).
constexpr const char* kEncabezado = "RPGMAZMORRAS_SAVE";
// v2: se agrego el bloque de trampas de piso (ver game::Trampa en
// dungeon.h), justo despues del de paredes.
// v3: se agrego el bloque del mapa de mazmorras (mazmorraSuperada/enMapa/
// mazmorraActivaIndice, ver DatosPartida en save.h) al final del archivo, y
// se agregaron los dos campos nuevos de Item (estado/apuntaAEnemigo, ver
// item_types.h — sumados para los consumibles de combate: Bomba de Veneno,
// Frasco de Escudo, Antidoto) a EscribirItem/LeerItem. Un archivo de una
// version anterior no tiene ninguno de los dos, asi que no se puede leer
// con el parser actual sin desalinear todo lo que viene despues -- se
// rechaza como cualquier otra version vieja/desconocida (ver CargarPartida),
// el jugador simplemente arranca una partida nueva en vez de crashear.
constexpr int kVersion = 3;
constexpr char kDelimitador = '|';

// --- Escritura: cada registro es una linea con campos separados por '|'.
// Los enums (Role, TipoItem, EfectoItem, RanuraEquipo, TipoEnemigo) se
// guardan como el entero subyacente (static_cast<int>) en vez de como texto
// — son todos chicos y fijos, y evita tener que mantener una tabla de
// nombres en paralelo solo para el archivo de guardado. Mientras el orden
// de esos enums no cambie, el numero sigue siendo valido; si algun dia se
// reordenan, hay que subir kVersion.

void EscribirItem(std::ostream& out, const Item& item) {
    out << item.nombre << kDelimitador << item.descripcion << kDelimitador
        << static_cast<int>(item.tipo) << kDelimitador << static_cast<int>(item.efecto) << kDelimitador
        << static_cast<int>(item.ranura) << kDelimitador << item.dados << kDelimitador
        << item.caras << kDelimitador << item.bono << kDelimitador
        << static_cast<int>(item.estado) << kDelimitador << (item.apuntaAEnemigo ? 1 : 0);
}

void EscribirItemEquipado(std::ostream& out, const ItemEquipado& equipado) {
    out << (equipado.ocupado ? 1 : 0) << kDelimitador;
    EscribirItem(out, equipado.item);  // si no esta ocupado, son campos placeholder (se ignoran al leer)
}

// --- Lectura: separa una linea en sus campos y los va devolviendo en
// orden. Devuelve string vacio (nunca lanza) si se piden mas campos de los
// que habia en la linea — quien llama es responsable de chequear
// LineaCompleta() para detectar un archivo truncado en vez de seguir con
// datos vacios silenciosamente.

std::vector<std::string> Partir(const std::string& linea, char delim) {
    std::vector<std::string> campos;
    std::string actual;
    for (char c : linea) {
        if (c == delim) {
            campos.push_back(actual);
            actual.clear();
        } else if (c != '\r') {  // por si el archivo tiene finales de linea CRLF
            actual += c;
        }
    }
    campos.push_back(actual);
    return campos;
}

struct LectorCampos {
    std::vector<std::string> campos;
    size_t cursor = 0;
    bool huboFaltante = false;

    explicit LectorCampos(std::string linea) : campos(Partir(linea, kDelimitador)) {}

    std::string Str() {
        if (cursor >= campos.size()) { huboFaltante = true; ++cursor; return {}; }
        return campos[cursor++];
    }
    int Int() {
        try { return std::stoi(Str()); } catch (...) { huboFaltante = true; return 0; }
    }
    float Float() {
        try { return std::stof(Str()); } catch (...) { huboFaltante = true; return 0.0f; }
    }
    bool Bool() { return Str() == "1"; }
};

Item LeerItem(LectorCampos& l) {
    Item item;
    item.nombre = l.Str();
    item.descripcion = l.Str();
    item.tipo = static_cast<TipoItem>(l.Int());
    item.efecto = static_cast<EfectoItem>(l.Int());
    item.ranura = static_cast<RanuraEquipo>(l.Int());
    item.dados = l.Int();
    item.caras = l.Int();
    item.bono = l.Int();
    item.estado = static_cast<TipoEfecto>(l.Int());
    item.apuntaAEnemigo = l.Bool();
    return item;
}

ItemEquipado LeerItemEquipado(LectorCampos& l) {
    ItemEquipado equipado;
    equipado.ocupado = l.Bool();
    equipado.item = LeerItem(l);
    return equipado;
}

// Lee una linea entera del archivo, devolviendo false (sin lanzar) si ya no
// hay mas lineas — el llamador lo trata como archivo truncado.
bool LeerLinea(std::istream& in, std::string& out) {
    return static_cast<bool>(std::getline(in, out));
}

}  // namespace

std::string RutaGuardado(int slot) {
    return "savegame_slot" + std::to_string(slot + 1) + ".txt";
}

bool HayPartidaGuardada(int slot) {
    std::ifstream archivo(RutaGuardado(slot));
    return archivo.good();
}

bool GuardarPartida(int slot, const Dungeon& mazmorra, const Party& party,
                     const std::vector<Enemy>& enemigos, const std::vector<Cofre>& cofres,
                     const bool mazmorraSuperada[kNumMazmorrasMapa], bool enMapa, int mazmorraActivaIndice) {
    std::ofstream out(RutaGuardado(slot), std::ios::trunc);
    if (!out.is_open()) return false;

    out << kEncabezado << kDelimitador << kVersion << "\n";

    // --- Mazmorra: salas (bounding box) + paredes ya resueltas tal cual
    // quedaron (ver el comentario de Dungeon::Dungeon en dungeon.h sobre
    // por que no alcanza con guardar solo la receta de generacion).
    const auto& habitaciones = mazmorra.Habitaciones();
    out << habitaciones.size() << "\n";
    for (const auto& h : habitaciones) {
        out << h.x << kDelimitador << h.y << kDelimitador << h.ancho << kDelimitador << h.alto << "\n";
    }
    const auto& paredes = mazmorra.Paredes();
    out << paredes.size() << "\n";
    for (const auto& p : paredes) {
        out << p.x << kDelimitador << p.y << kDelimitador << p.width << kDelimitador << p.height << "\n";
    }
    const auto& trampas = mazmorra.Trampas();
    out << trampas.size() << "\n";
    for (const auto& t : trampas) {
        out << static_cast<int>(t.tipo) << kDelimitador << t.area.x << kDelimitador << t.area.y << kDelimitador
            << t.area.width << kDelimitador << t.area.height << "\n";
    }

    // --- Party: stats actuales (ya incluyen cualquier bono de equipo
    // aplicado), posicion y equipo de cada miembro, en el mismo orden que
    // CrearPartyDeEjemplo en main.cpp.
    const auto& miembros = party.Miembros();
    out << miembros.size() << "\n";
    for (const auto& personaje : miembros) {
        const auto& stats = personaje.GetStats();
        const auto& pos = personaje.Posicion();
        out << personaje.Nombre() << kDelimitador << static_cast<int>(personaje.Rol()) << kDelimitador
            << stats.hpMax << kDelimitador << stats.hp << kDelimitador
            << stats.recursoMax << kDelimitador << stats.recurso << kDelimitador
            << stats.ataque << kDelimitador << stats.defensa << kDelimitador << stats.velocidad << kDelimitador
            << pos.x << kDelimitador << pos.y << kDelimitador;
        EscribirItemEquipado(out, personaje.Arma());
        out << kDelimitador;
        EscribirItemEquipado(out, personaje.Accesorio());
        out << "\n";
    }

    // --- Inventario compartido: una linea por pila (item + cantidad).
    const auto& pilas = party.Inventario().Pilas();
    out << pilas.size() << "\n";
    for (const auto& pila : pilas) {
        out << pila.cantidad << kDelimitador;
        EscribirItem(out, pila.item);
        out << "\n";
    }

    // --- Enemigos: estado completo (para que uno a medio pegar en un
    // combate anterior que se abandono siga con el HP que tenia — aunque en
    // la practica solo se puede guardar durante la exploracion, con ningun
    // combate en curso).
    out << enemigos.size() << "\n";
    for (const auto& enemigo : enemigos) {
        const auto& stats = enemigo.GetStats();
        const auto& pos = enemigo.Posicion();
        out << enemigo.Nombre() << kDelimitador << static_cast<int>(enemigo.Tipo()) << kDelimitador
            << stats.hpMax << kDelimitador << stats.hp << kDelimitador
            << stats.recursoMax << kDelimitador << stats.recurso << kDelimitador
            << stats.ataque << kDelimitador << stats.defensa << kDelimitador << stats.velocidad << kDelimitador
            << pos.x << kDelimitador << pos.y << kDelimitador << enemigo.Sala() << kDelimitador
            << (enemigo.Vencido() ? 1 : 0) << "\n";
    }

    // --- Cofres: posicion, si ya se abrio, y su contenido (aunque ya este
    // abierto — no hace falta, pero mantiene el registro simple y completo).
    out << cofres.size() << "\n";
    for (const auto& cofre : cofres) {
        out << cofre.posicion.x << kDelimitador << cofre.posicion.y << kDelimitador
            << (cofre.abierto ? 1 : 0) << kDelimitador;
        EscribirItem(out, cofre.contenido);
        out << "\n";
    }

    // --- Mapa de mazmorras (ver DatosPartida en save.h) ---
    for (int i = 0; i < kNumMazmorrasMapa; ++i) {
        out << (mazmorraSuperada[i] ? 1 : 0) << (i + 1 < kNumMazmorrasMapa ? kDelimitador : '\n');
    }
    out << (enMapa ? 1 : 0) << kDelimitador << mazmorraActivaIndice << "\n";

    return static_cast<bool>(out);
}

ResultadoCarga CargarPartida(int slot) {
    ResultadoCarga resultado;

    std::ifstream in(RutaGuardado(slot));
    if (!in.is_open()) return resultado;  // sin archivo: valido=false

    std::string linea;

    // Encabezado: "RPGMAZMORRAS_SAVE|1". Cualquier otra cosa (archivo de
    // otro programa, version futura, archivo vacio) se rechaza sin
    // intentar interpretarlo.
    if (!LeerLinea(in, linea)) return resultado;
    LectorCampos encabezado(linea);
    if (encabezado.Str() != kEncabezado) return resultado;
    if (encabezado.Int() != kVersion) return resultado;

    DatosPartida datos;

    // --- Mazmorra ---
    if (!LeerLinea(in, linea)) return resultado;
    int numHabitaciones = 0;
    try { numHabitaciones = std::stoi(linea); } catch (...) { return resultado; }
    if (numHabitaciones < 0) return resultado;
    for (int i = 0; i < numHabitaciones; ++i) {
        if (!LeerLinea(in, linea)) return resultado;
        LectorCampos l(linea);
        Habitacion h;
        h.x = l.Int(); h.y = l.Int(); h.ancho = l.Int(); h.alto = l.Int();
        if (l.huboFaltante) return resultado;
        datos.habitaciones.push_back(h);
    }

    if (!LeerLinea(in, linea)) return resultado;
    int numParedes = 0;
    try { numParedes = std::stoi(linea); } catch (...) { return resultado; }
    if (numParedes < 0) return resultado;
    for (int i = 0; i < numParedes; ++i) {
        if (!LeerLinea(in, linea)) return resultado;
        LectorCampos l(linea);
        Rect r;
        r.x = l.Float(); r.y = l.Float(); r.width = l.Float(); r.height = l.Float();
        if (l.huboFaltante) return resultado;
        datos.paredes.push_back(r);
    }

    if (!LeerLinea(in, linea)) return resultado;
    int numTrampas = 0;
    try { numTrampas = std::stoi(linea); } catch (...) { return resultado; }
    if (numTrampas < 0) return resultado;
    for (int i = 0; i < numTrampas; ++i) {
        if (!LeerLinea(in, linea)) return resultado;
        LectorCampos l(linea);
        Trampa t;
        t.tipo = static_cast<TipoTrampa>(l.Int());
        t.area.x = l.Float();
        t.area.y = l.Float();
        t.area.width = l.Float();
        t.area.height = l.Float();
        if (l.huboFaltante) return resultado;
        datos.trampas.push_back(t);
    }

    // --- Party ---
    if (!LeerLinea(in, linea)) return resultado;
    int numMiembros = 0;
    try { numMiembros = std::stoi(linea); } catch (...) { return resultado; }
    if (numMiembros < 0) return resultado;
    for (int i = 0; i < numMiembros; ++i) {
        if (!LeerLinea(in, linea)) return resultado;
        LectorCampos l(linea);
        std::string nombre = l.Str();
        Role rol = static_cast<Role>(l.Int());
        Stats stats;
        stats.hpMax = l.Int();
        stats.hp = l.Int();
        stats.recursoMax = l.Int();
        stats.recurso = l.Int();
        stats.ataque = l.Int();
        stats.defensa = l.Int();
        stats.velocidad = l.Float();
        Vec2 pos;
        pos.x = l.Float();
        pos.y = l.Float();
        ItemEquipado arma = LeerItemEquipado(l);
        ItemEquipado accesorio = LeerItemEquipado(l);
        if (l.huboFaltante) return resultado;

        Character personaje(nombre, rol, stats, pos);
        personaje.CargarEquipoGuardado(std::move(arma), std::move(accesorio));
        datos.miembros.push_back(std::move(personaje));
    }

    // --- Inventario ---
    if (!LeerLinea(in, linea)) return resultado;
    int numPilas = 0;
    try { numPilas = std::stoi(linea); } catch (...) { return resultado; }
    if (numPilas < 0) return resultado;
    for (int i = 0; i < numPilas; ++i) {
        if (!LeerLinea(in, linea)) return resultado;
        LectorCampos l(linea);
        int cantidad = l.Int();
        Item item = LeerItem(l);
        if (l.huboFaltante) return resultado;
        datos.pilasInventario.push_back(PilaItem{std::move(item), cantidad});
    }

    // --- Enemigos ---
    if (!LeerLinea(in, linea)) return resultado;
    int numEnemigos = 0;
    try { numEnemigos = std::stoi(linea); } catch (...) { return resultado; }
    if (numEnemigos < 0) return resultado;
    for (int i = 0; i < numEnemigos; ++i) {
        if (!LeerLinea(in, linea)) return resultado;
        LectorCampos l(linea);
        std::string nombre = l.Str();
        TipoEnemigo tipo = static_cast<TipoEnemigo>(l.Int());
        Stats stats;
        stats.hpMax = l.Int();
        stats.hp = l.Int();
        stats.recursoMax = l.Int();
        stats.recurso = l.Int();
        stats.ataque = l.Int();
        stats.defensa = l.Int();
        stats.velocidad = l.Float();
        Vec2 pos;
        pos.x = l.Float();
        pos.y = l.Float();
        int sala = l.Int();
        bool vencido = l.Bool();
        if (l.huboFaltante) return resultado;

        Enemy enemigo(nombre, tipo, stats, pos, sala);
        if (vencido) enemigo.MarcarVencido();
        datos.enemigos.push_back(std::move(enemigo));
    }

    // --- Cofres ---
    if (!LeerLinea(in, linea)) return resultado;
    int numCofres = 0;
    try { numCofres = std::stoi(linea); } catch (...) { return resultado; }
    if (numCofres < 0) return resultado;
    for (int i = 0; i < numCofres; ++i) {
        if (!LeerLinea(in, linea)) return resultado;
        LectorCampos l(linea);
        Cofre cofre;
        cofre.posicion.x = l.Float();
        cofre.posicion.y = l.Float();
        cofre.abierto = l.Bool();
        cofre.contenido = LeerItem(l);
        if (l.huboFaltante) return resultado;
        datos.cofres.push_back(std::move(cofre));
    }

    // --- Mapa de mazmorras ---
    if (!LeerLinea(in, linea)) return resultado;
    {
        LectorCampos l(linea);
        for (int i = 0; i < kNumMazmorrasMapa; ++i) datos.mazmorraSuperada[i] = l.Bool();
        if (l.huboFaltante) return resultado;
    }
    if (!LeerLinea(in, linea)) return resultado;
    {
        LectorCampos l(linea);
        datos.enMapa = l.Bool();
        datos.mazmorraActivaIndice = l.Int();
        if (l.huboFaltante) return resultado;
    }

    resultado.valido = true;
    resultado.datos = std::move(datos);
    return resultado;
}

}  // namespace game
