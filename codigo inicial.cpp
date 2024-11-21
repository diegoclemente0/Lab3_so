#include <iostream>
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <map>

using namespace std;

struct Pagina {
    int id_proceso;  // ID del proceso (-1 si está libre)
    bool en_ram;     // Indica si está en RAM o swap
    int tiempo;      // Marca de tiempo para FIFO
};

struct Proceso {
    int id;                         // ID único del proceso
    int tamano;                     // Tamaño en bytes
    vector<int> paginas_asignadas;  // Índices de las páginas
};

class Memoria {
private:
    vector<Pagina> ram, swap;
    int tamano_pagina;
    int tiempo_global = 0;
    queue<int> fifo_ram;  // Cola FIFO para páginas en RAM

public:
    Memoria(int tamano_ram, int tamano_swap, int tamano_pagina)
        : tamano_pagina(tamano_pagina) {
        ram.resize(tamano_ram / tamano_pagina, {-1, true, 0});
        swap.resize(tamano_swap / tamano_pagina, {-1, false, 0});
    }

    int asignarPaginas(Proceso& proceso, int paginas_necesarias) {
        for (int i = 0; i < paginas_necesarias; ++i) {
            if (!asignarPagina(proceso.id)) {
                return -1;  // No hay espacio disponible
            }
        }
        return 0;
    }

    bool asignarPagina(int id_proceso) {
        for (auto& pagina : ram) {
            if (pagina.id_proceso == -1) {  // Página libre en RAM
                pagina.id_proceso = id_proceso;
                pagina.en_ram = true;
                pagina.tiempo = tiempo_global++;
                fifo_ram.push(&pagina - &ram[0]);
                return true;
            }
        }

        // Si no hay espacio en RAM, mover una página a swap
        return reemplazarPagina(id_proceso);
    }

    bool reemplazarPagina(int id_proceso) {
        if (fifo_ram.empty()) return false;

        int pagina_a_reemplazar = fifo_ram.front();
        fifo_ram.pop();

        Pagina& pagina = ram[pagina_a_reemplazar];
        for (auto& p : swap) {
            if (p.id_proceso == -1) {  // Página libre en swap
                p = pagina;  // Mover a swap
                pagina.id_proceso = id_proceso;
                pagina.tiempo = tiempo_global++;
                fifo_ram.push(pagina_a_reemplazar);
                return true;
            }
        }
        return false;  // No hay espacio en swap
    }

    void liberarProceso(int id_proceso) {
        for (auto& pagina : ram) {
            if (pagina.id_proceso == id_proceso) {
                pagina.id_proceso = -1;
            }
        }

        for (auto& pagina : swap) {
            if (pagina.id_proceso == id_proceso) {
                pagina.id_proceso = -1;
            }
        }
    }

    void mostrarEstado() {
        cout << "RAM:\n";
        for (size_t i = 0; i < ram.size(); ++i) {
            cout << "Página " << i << ": "
                 << (ram[i].id_proceso == -1 ? "Libre" : "Proceso " + to_string(ram[i].id_proceso)) << endl;
        }
        cout << "\nSwap:\n";
        for (size_t i = 0; i < swap.size(); ++i) {
            cout << "Página " << i << ": "
                 << (swap[i].id_proceso == -1 ? "Libre" : "Proceso " + to_string(swap[i].id_proceso)) << endl;
        }
    }
};

int main() {
    srand(time(nullptr));

    int tamano_ram, tamano_swap, tamano_pagina;
    cout << "Ingrese tamaño de RAM (MB): ";
    cin >> tamano_ram;
    cout << "Ingrese tamaño de swap (MB): ";
    cin >> tamano_swap;
    cout << "Ingrese tamaño de página (KB): ";
    cin >> tamano_pagina;

    tamano_pagina *= 1024;  // Convertir a bytes
    Memoria memoria(tamano_ram * 1024 * 1024, tamano_swap * 1024 * 1024, tamano_pagina);

    int id_proceso = 0;
    vector<Proceso> procesos;

    auto generarDireccion = [](int tamano_virtual) {
        return rand() % tamano_virtual;
    };

    for (int tiempo = 0;; tiempo += 2) {
        // Crear un nuevo proceso
        Proceso nuevo_proceso;
        nuevo_proceso.id = id_proceso++;
        nuevo_proceso.tamano = (rand() % 500 + 1) * tamano_pagina;  // Tamaño entre 1 y 500 páginas
        int paginas_necesarias = (nuevo_proceso.tamano + tamano_pagina - 1) / tamano_pagina;

        if (memoria.asignarPaginas(nuevo_proceso, paginas_necesarias) == -1) {
            cout << "Memoria insuficiente. Terminando simulación.\n";
            break;
        }

        procesos.push_back(nuevo_proceso);
        cout << "Se creó el proceso " << nuevo_proceso.id << " con " << paginas_necesarias << " páginas.\n";

        // Cada 30 segundos, finalizar un proceso aleatorio
        if (tiempo >= 30 && tiempo % 5 == 0) {
            int indice_proceso = rand() % procesos.size();
            int id_a_liberar = procesos[indice_proceso].id;
            memoria.liberarProceso(id_a_liberar);
            procesos.erase(procesos.begin() + indice_proceso);
            cout << "Se liberó el proceso " << id_a_liberar << ".\n";
        }

        // Mostrar estado de la memoria
        memoria.mostrarEstado();

        this_thread::sleep_for(chrono::seconds(2));
    }

    return 0;
}
