#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

#ifdef __ANDROID__
#include <sys/system_properties.h>
#endif

using namespace std;

// =============================================
// COLORS
// =============================================
const string VERDE = "\033[0;32m";
const string AMARILLO = "\033[1;33m";
const string ROJO = "\033[0;31m";
const string AZUL = "\033[0;34m";
const string MAGENTA = "\033[0;35m";
const string CYAN = "\033[0;36m";
const string BLANCO = "\033[1;37m";
const string NC = "\033[0m";

// =============================================
// GLOBAL VARIABLES
// =============================================
string ROUTER = "192.168.1.1";
string MI_IP = "";
mutex cout_mutex;
atomic<int> active_threads(0);

// =============================================
// UTILITY FUNCTIONS
// =============================================
void clear_screen() {
    cout << "\033[2J\033[1;1H";
}

bool is_valid_ip(const string& ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
}

int ping_host(const string& ip, int timeout_sec = 1) {
    string cmd = "ping -c 1 -W " + to_string(timeout_sec) + " " + ip + " > /dev/null 2>&1";
    return system(cmd.c_str());
}

string exec_command(const string& cmd) {
    char buffer[128];
    string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

int check_port(const string& ip, int port, int timeout_ms = 1000) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 0;
    
    // Set non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    if (ret < 0) {
        fd_set fdset;
        struct timeval tv;
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        
        if (select(sock + 1, nullptr, &fdset, nullptr, &tv) == 1) {
            int so_error;
            socklen_t len = sizeof(so_error);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
            ret = (so_error == 0) ? 0 : -1;
        } else {
            ret = -1;
        }
    }
    
    close(sock);
    return (ret == 0);
}

// =============================================
// NETWORK FUNCTIONS
// =============================================
void obtener_ip_local() {
    // Try different methods to get local IP
    string result = exec_command("ip -4 addr show 2>/dev/null | grep -oP '(?<=inet\\s)\\d+(\\.\\d+){3}' | grep -v '127.0.0.1' | head -1");
    
    if (result.empty()) {
        result = exec_command("ifconfig 2>/dev/null | grep -Eo 'inet (addr:)?([0-9]*\\.){3}[0-9]*' | grep -Eo '([0-9]*\\.){3}[0-9]*' | grep -v '127.0.0.1' | head -1");
    }
    
    if (!result.empty()) {
        MI_IP = result;
        result.erase(remove(result.begin(), result.end(), '\n'), result.end());
        cout << VERDE << "[+] IP Local: " << MI_IP << NC << endl;
    } else {
        cout << AMARILLO << "[!] No se pudo detectar IP automáticamente" << NC << endl;
        cout << "Introduce tu IP local (ej: 192.168.1.10): ";
        getline(cin, MI_IP);
    }
    
    cout << AMARILLO << "[!] MAC no disponible (normal en Termux sin root)" << NC << endl;
}

void ping_test() {
    cout << "\n" << AZUL << "[*] Probando conectividad con el router..." << NC << endl;
    
    string cmd = "ping -c 4 " + ROUTER + " 2>&1";
    string result = exec_command(cmd);
    
    if (result.find("bytes from") != string::npos) {
        cout << result;
        cout << VERDE << "[+] Conexión establecida correctamente" << NC << endl;
    } else {
        cout << ROJO << "[!] No se pudo conectar con el router" << NC << endl;
        cout << AMARILLO << "   Verifica que estés en la misma red" << NC << endl;
    }
}

void escanear_puertos() {
    cout << "\n" << AZUL << "[*] Escaneando puertos comunes en " << ROUTER << "..." << NC << endl;
    cout << CYAN << "────────────────────────────────────────" << NC << endl;
    
    vector<int> puertos = {21, 22, 23, 53, 80, 443, 8080, 8443, 8000, 8081};
    int abiertos = 0;
    
    for (int port : puertos) {
        if (check_port(ROUTER, port, 1000)) {
            cout << VERDE << "[+] Puerto " << port << ": ABIERTO" << NC << endl;
            abiertos++;
            
            switch(port) {
                case 21: cout << "     └─ Servicio: FTP" << endl; break;
                case 22: cout << "     └─ Servicio: SSH" << endl; break;
                case 23: cout << "     └─ " << ROJO << "Servicio: TELNET (inseguro)" << NC << endl; break;
                case 53: cout << "     └─ Servicio: DNS" << endl; break;
                case 80: cout << "     └─ Servicio: HTTP" << endl; break;
                case 443: cout << "     └─ Servicio: HTTPS" << endl; break;
                case 8080: cout << "     └─ Servicio: HTTP alternativo" << endl; break;
                case 8443: cout << "     └─ Servicio: HTTPS alternativo" << endl; break;
                case 8000: cout << "     └─ Servicio: Puerto de aplicaciones" << endl; break;
                case 8081: cout << "     └─ Servicio: HTTP alternativo" << endl; break;
            }
        }
    }
    
    if (abiertos == 0) {
        cout << AMARILLO << "[-] No se encontraron puertos abiertos" << NC << endl;
    } else {
        cout << "\n" << VERDE << "[+] Total puertos abiertos: " << abiertos << NC << endl;
    }
}

string identificar_por_puertos(const string& ip) {
    vector<int> puertos_check = {22, 23, 80, 443, 445, 548, 554, 515, 631, 8080, 8443, 1883, 5000, 5001, 9100, 7547, 8888, 3389, 5900, 62078};
    vector<int> puertos_abiertos;
    
    for (int port : puertos_check) {
        if (check_port(ip, port, 500)) {
            puertos_abiertos.push_back(port);
        }
    }
    
    string tipo = "";
    
    // Check for router
    if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 80) != puertos_abiertos.end() &&
        find(puertos_abiertos.begin(), puertos_abiertos.end(), 443) != puertos_abiertos.end() &&
        find(puertos_abiertos.begin(), puertos_abiertos.end(), 53) != puertos_abiertos.end()) {
        tipo = "🌐 Router / Gateway";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 7547) != puertos_abiertos.end()) {
        tipo = "🌐 Router (TR-069)";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 445) != puertos_abiertos.end() &&
             find(puertos_abiertos.begin(), puertos_abiertos.end(), 548) != puertos_abiertos.end()) {
        tipo = "🗄️  NAS (Samba + AFP)";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 445) != puertos_abiertos.end() &&
             find(puertos_abiertos.begin(), puertos_abiertos.end(), 80) != puertos_abiertos.end()) {
        tipo = "🗄️  NAS / Servidor";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 554) != puertos_abiertos.end()) {
        tipo = "📷 Cámara IP (RTSP)";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 8080) != puertos_abiertos.end() && puertos_abiertos.size() <= 2) {
        tipo = "📷 Posible Cámara IP";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 9100) != puertos_abiertos.end() ||
             find(puertos_abiertos.begin(), puertos_abiertos.end(), 515) != puertos_abiertos.end() ||
             find(puertos_abiertos.begin(), puertos_abiertos.end(), 631) != puertos_abiertos.end()) {
        tipo = "🖨️  Impresora";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 8008) != puertos_abiertos.end() ||
             find(puertos_abiertos.begin(), puertos_abiertos.end(), 8009) != puertos_abiertos.end() ||
             find(puertos_abiertos.begin(), puertos_abiertos.end(), 5000) != puertos_abiertos.end()) {
        tipo = "📺 Smart TV / Chromecast";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 445) != puertos_abiertos.end() &&
             find(puertos_abiertos.begin(), puertos_abiertos.end(), 3389) != puertos_abiertos.end()) {
        tipo = "💻 PC Windows (RDP activo)";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 445) != puertos_abiertos.end()) {
        tipo = "💻 PC Windows";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 3389) != puertos_abiertos.end()) {
        tipo = "💻 PC Windows (RDP)";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 22) != puertos_abiertos.end() &&
             find(puertos_abiertos.begin(), puertos_abiertos.end(), 80) != puertos_abiertos.end()) {
        tipo = "🐧 Servidor Linux (SSH+Web)";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 22) != puertos_abiertos.end()) {
        tipo = "🐧 Linux / Mac (SSH)";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 1883) != puertos_abiertos.end()) {
        tipo = "🏠 Dispositivo IoT (MQTT)";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 5900) != puertos_abiertos.end()) {
        tipo = "🖥️  Escritorio remoto (VNC)";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 62078) != puertos_abiertos.end()) {
        tipo = "🍎 iPhone / iPad";
    }
    else if (find(puertos_abiertos.begin(), puertos_abiertos.end(), 80) != puertos_abiertos.end() ||
             find(puertos_abiertos.begin(), puertos_abiertos.end(), 443) != puertos_abiertos.end()) {
        tipo = "🌐 Dispositivo con web";
    }
    else if (puertos_abiertos.empty()) {
        // Try TTL detection
        string ping_cmd = "ping -c 1 -W 1 " + ip + " 2>/dev/null | grep -oP 'ttl=\\K[0-9]+'";
        string ttl_str = exec_command(ping_cmd);
        if (!ttl_str.empty()) {
            int ttl = stoi(ttl_str);
            if (ttl <= 64) tipo = "📱 Smartphone / Tablet";
            else if (ttl <= 128) tipo = "💻 PC / Laptop";
            else tipo = "📡 Dispositivo de red";
        }
    }
    
    if (!puertos_abiertos.empty()) {
        tipo += " [puertos: ";
        for (size_t i = 0; i < puertos_abiertos.size(); i++) {
            tipo += to_string(puertos_abiertos[i]);
            if (i < puertos_abiertos.size() - 1) tipo += ", ";
        }
        tipo += "]";
    }
    
    return tipo.empty() ? "❓ Dispositivo desconocido" : tipo;
}

string obtener_hostname(const string& ip) {
    string hostname = exec_command("getent hosts " + ip + " 2>/dev/null | awk '{print $2}' | head -1");
    if (hostname.empty()) {
        hostname = exec_command("host " + ip + " 2>/dev/null | grep 'domain name' | awk '{print $NF}' | sed 's/\\.$//'");
    }
    hostname.erase(remove(hostname.begin(), hostname.end(), '\n'), hostname.end());
    return hostname;
}

void scan_host(int i, const string& red, vector<string>& resultados, mutex& mtx) {
    string ip = red + "." + to_string(i);
    active_threads++;
    
    if (ping_host(ip, 1) == 0) {
        lock_guard<mutex> lock(mtx);
        resultados.push_back(ip);
    }
    
    active_threads--;
}

void ver_dispositivos() {
    cout << "\n" << AZUL << "[*] BUSCANDO DISPOSITIVOS EN LA RED" << NC << endl;
    cout << CYAN << "════════════════════════════════════════════════════════" << NC << endl;
    
    string red = ROUTER.substr(0, ROUTER.find_last_of('.'));
    vector<string> resultados;
    mutex resultados_mutex;
    vector<thread> threads;
    
    cout << VERDE << "[*] Escaneando red " << red << ".0/24..." << NC << "\n" << endl;
    cout << CYAN << "📡 Fase 1: Descubriendo hosts con ping..." << NC << endl;
    
    for (int i = 1; i <= 254; i++) {
        threads.emplace_back(scan_host, i, ref(red), ref(resultados), ref(resultados_mutex));
        if (threads.size() >= 30) {
            for (auto& t : threads) t.join();
            threads.clear();
        }
    }
    
    for (auto& t : threads) t.join();
    
    cout << CYAN << "🔍 Fase 2: Identificando dispositivos por puertos..." << NC << "\n" << endl;
    
    if (!resultados.empty()) {
        sort(resultados.begin(), resultados.end(),
             [](const string& a, const string& b) {
                 int ia = stoi(a.substr(a.find_last_of('.') + 1));
                 int ib = stoi(b.substr(b.find_last_of('.') + 1));
                 return ia < ib;
             });
        
        cout << VERDE << "📱 DISPOSITIVOS ENCONTRADOS:" << NC << endl;
        cout << CYAN << "────────────────────────────────────────" << NC << endl;
        
        int contador = 0;
        for (const string& ip : resultados) {
            contador++;
            string hostname = obtener_hostname(ip);
            string info = identificar_por_puertos(ip);
            
            if (ip == ROUTER) {
                cout << VERDE << "[" << contador << "] " << ip << NC << " " << AMARILLO << "(ROUTER)" << NC << endl;
            } else if (ip == MI_IP) {
                cout << AZUL << "[" << contador << "] " << ip << NC << " " << CYAN << "(ESTE DISPOSITIVO)" << NC << endl;
            } else {
                cout << BLANCO << "[" << contador << "] " << ip << NC << endl;
            }
            
            if (!hostname.empty()) {
                cout << "   ├─ Nombre: " << hostname << endl;
            }
            cout << "   └─ " << info << endl;
            cout << endl;
        }
        
        cout << VERDE << "[+] Total dispositivos detectados: " << resultados.size() << NC << endl;
    } else {
        cout << ROJO << "[-] No se encontraron dispositivos" << NC << endl;
    }
}

void estadisticas_red() {
    cout << "\n" << AZUL << "[*] Estadísticas de conexión..." << NC << endl;
    cout << CYAN << "────────────────────────────────────────" << NC << endl;
    
    string result = exec_command("ping -c 10 -q " + ROUTER + " 2>/dev/null");
    
    size_t rtt_pos = result.find("rtt");
    size_t loss_pos = result.find("packet loss");
    
    if (rtt_pos != string::npos && loss_pos != string::npos) {
        string rtt_line = result.substr(rtt_pos);
        string loss_line = result.substr(loss_pos);
        
        float min_rtt, avg_rtt, max_rtt;
        sscanf(rtt_line.c_str(), "rtt min/avg/max/mdev = %f/%f/%f", &min_rtt, &avg_rtt, &max_rtt);
        
        int loss_percent;
        sscanf(loss_line.c_str(), "%d%% packet loss", &loss_percent);
        
        cout << VERDE << "[+] Latencia:" << NC << endl;
        cout << "   ├─ Mínima: " << min_rtt << "ms" << endl;
        cout << "   ├─ Media:  " << avg_rtt << "ms" << endl;
        cout << "   └─ Máxima: " << max_rtt << "ms" << endl;
        cout << VERDE << "[+] Pérdida de paquetes: " << loss_percent << "%" << NC << endl;
        
        if (avg_rtt < 10) {
            cout << "\n" << VERDE << "[+] Calidad: EXCELENTE 🚀" << NC << endl;
        } else if (avg_rtt < 30) {
            cout << "\n" << VERDE << "[+] Calidad: BUENA ✓" << NC << endl;
        } else if (avg_rtt < 60) {
            cout << "\n" << AMARILLO << "[+] Calidad: ACEPTABLE ⚠" << NC << endl;
        } else {
            cout << "\n" << ROJO << "[+] Calidad: MALA ✗" << NC << endl;
        }
    } else {
        cout << ROJO << "[-] No se pudieron obtener estadísticas" << NC << endl;
    }
}

void recomendaciones_inteligentes() {
    cout << "\n" << MAGENTA << "════════════════════════════════════════════════════════" << NC << endl;
    cout << MAGENTA << "🔒 RECOMENDACIONES PERSONALIZADAS" << NC << endl;
    cout << MAGENTA << "════════════════════════════════════════════════════════" << NC << endl;
    
    int riesgos = 0;
    
    if (check_port(ROUTER, 23, 1000)) {
        cout << ROJO << "[!] RIESGO CRÍTICO: Telnet (puerto 23) ABIERTO" << NC << endl;
        cout << "     └═ Deshabilita Telnet inmediatamente (inseguro)" << endl;
        riesgos++;
    }
    
    if (check_port(ROUTER, 21, 1000)) {
        cout << AMARILLO << "[!] RIESGO MEDIO: FTP (puerto 21) ABIERTO" << NC << endl;
        cout << "     └═ Usa SFTP en su lugar" << endl;
        riesgos++;
    }
    
    bool http_open = check_port(ROUTER, 80, 1000);
    bool https_open = check_port(ROUTER, 443, 1000);
    
    if (http_open && !https_open) {
        cout << AMARILLO << "[!] Solo HTTP disponible (sin cifrar)" << NC << endl;
        cout << "     └═ Habilita HTTPS para administración segura" << endl;
        riesgos++;
    }
    
    if (riesgos == 0) {
        cout << VERDE << "✅ No se detectaron riesgos inmediatos" << NC << endl;
        cout << "   Tu router tiene buena configuración básica" << endl;
    } else {
        cout << "\n" << AMARILLO << "⚠ Se detectaron " << riesgos << " puntos de mejora" << NC << endl;
    }
    
    cout << MAGENTA << "════════════════════════════════════════════════════════" << NC << endl;
    
    if (riesgos > 0) {
        cout << "\n" << CYAN << "📋 Consejos rápidos:" << NC << endl;
        cout << "   • Actualiza el firmware del router" << endl;
        cout << "   • Cambia la contraseña por defecto" << endl;
        cout << "   • Usa WPA2/WPA3 para WiFi" << endl;
    }
}

void exportar_resultados() {
    cout << "\n" << AZUL << "[*] Exportando resultados..." << NC << endl;
    
    time_t now = time(nullptr);
    char fecha[20];
    strftime(fecha, sizeof(fecha), "%Y%m%d_%H%M%S", localtime(&now));
    
    string archivo = "escaneo_router_" + string(fecha) + ".txt";
    ofstream file(archivo);
    
    if (file.is_open()) {
        file << "==============================================" << endl;
        file << "       ESCANEO DE ROUTER - INFORME" << endl;
        file << "==============================================" << endl;
        file << "Fecha: " << ctime(&now);
        file << "Router: " << ROUTER << endl;
        file << "IP Local: " << MI_IP << endl;
        file << "==============================================" << endl;
        file << endl;
        
        file << "--- PUERTOS ABIERTOS ---" << endl;
        vector<int> puertos = {21, 22, 23, 53, 80, 443, 8080, 8443};
        for (int port : puertos) {
            if (check_port(ROUTER, port, 1000)) {
                file << "Puerto " << port << ": ABIERTO" << endl;
            }
        }
        
        file << endl << "--- ESTADÍSTICAS ---" << endl;
        string stats = exec_command("ping -c 5 " + ROUTER + " 2>&1 | grep -E 'packet loss|rtt'");
        file << stats;
        
        file.close();
        cout << VERDE << "[+] Informe guardado en: " << archivo << NC << endl;
    } else {
        cout << ROJO << "[-] Error al crear el archivo" << NC << endl;
    }
}

void escaneo_completo() {
    cout << "\n" << MAGENTA << "╔════════════════════════════════════╗" << NC << endl;
    cout << MAGENTA << "║   INICIANDO ESCANEO COMPLETO      ║" << NC << endl;
    cout << MAGENTA << "╚════════════════════════════════════╝" << NC << endl;
    
    ping_test();
    escanear_puertos();
    estadisticas_red();
    ver_dispositivos();
    recomendaciones_inteligentes();
    
    cout << "\n" << VERDE << "[+] Escaneo completo finalizado" << NC << endl;
}

void cambiar_ip() {
    cout << "\n" << AZUL << "[*] IP actual: " << ROUTER << NC << endl;
    cout << "Nueva IP del router: ";
    string nueva_ip;
    getline(cin, nueva_ip);
    
    if (!nueva_ip.empty() && is_valid_ip(nueva_ip)) {
        ROUTER = nueva_ip;
        cout << VERDE << "[+] Router cambiado a: " << ROUTER << NC << endl;
    } else if (!nueva_ip.empty()) {
        cout << ROJO << "[!] Formato de IP inválido" << NC << endl;
    }
}

void mostrar_banner() {
    clear_screen();
    cout << CYAN;
    cout << "╔════════════════════════════════════════════════════╗" << endl;
    cout << "║              ESCANEARUTER v3.0                     ║" << endl;
    cout << "║           Versión C++ para Termux/Ubuntu           ║" << endl;
    cout << "║       Detección avanzada de dispositivos           ║" << endl;
    cout << "╚════════════════════════════════════════════════════╝" << endl;
    cout << NC;
}

// =============================================
// MAIN
// =============================================
int main() {
    mostrar_banner();
    obtener_ip_local();
    
    cout << "\n" << AZUL << "[*] Verificando conectividad..." << NC << endl;
    if (ping_host(ROUTER, 2) == 0) {
        cout << VERDE << "[+] Conectividad OK con " << ROUTER << NC << endl;
    } else {
        cout << ROJO << "[!] No se puede alcanzar " << ROUTER << NC << endl;
        cout << AMARILLO << "   Verifica que estés en la red WiFi" << NC << endl;
        cout << "\n" << AMARILLO << "¿Cambiar IP del router? (s/n): " << NC;
        string cambiar;
        getline(cin, cambiar);
        if (cambiar == "s" || cambiar == "S") {
            cambiar_ip();
        }
    }
    
    while (true) {
        cout << "\n" << CYAN << "════════════════════════════════════════════════════════" << NC << endl;
        cout << AMARILLO << "   📡 MENÚ PRINCIPAL - Router: " << VERDE << ROUTER << NC << endl;
        cout << CYAN << "════════════════════════════════════════════════════════" << NC << endl;
        cout << "1)  📶 Ping test" << endl;
        cout << "2)  🔌 Escanear puertos" << endl;
        cout << "3)  📊 Estadísticas de red" << endl;
        cout << "4)  📱 Buscar dispositivos" << endl;
        cout << "5)  🔒 Recomendaciones" << endl;
        cout << "6)  💾 Exportar resultados" << endl;
        cout << "7)  🔍 Escaneo completo" << endl;
        cout << "8)  ⚙️  Cambiar IP" << endl;
        cout << "9)  🚪 Salir" << endl;
        cout << CYAN << "────────────────────────────────────────────────────────" << NC << endl;
        cout << VERDE << "Selecciona una opción [1-9]: " << NC;
        
        string opcion;
        getline(cin, opcion);
        
        if (opcion == "1") ping_test();
        else if (opcion == "2") escanear_puertos();
        else if (opcion == "3") estadisticas_red();
        else if (opcion == "4") ver_dispositivos();
        else if (opcion == "5") recomendaciones_inteligentes();
        else if (opcion == "6") exportar_resultados();
        else if (opcion == "7") escaneo_completo();
        else if (opcion == "8") cambiar_ip();
        else if (opcion == "9") {
            cout << "\n" << VERDE << "¡Hasta luego!" << NC << endl;
            break;
        }
        else {
            cout << ROJO << "Opción no válida (1-9)" << NC << endl;
        }
        
        cout << "\n" << AMARILLO << "Presiona ENTER para continuar..." << NC;
        cin.ignore();
        mostrar_banner();
        cout << VERDE << "[+] Router: " << ROUTER << " | IP Local: " << MI_IP << NC << endl;
    }
    
    return 0;
}
