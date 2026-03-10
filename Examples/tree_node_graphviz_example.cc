#include <iostream>
#include <fstream>
#include <string>
#include "tpl_tree_node.H"
#include "generate_tree.H"

using namespace Aleph;
using namespace std;

// Estructura de ejemplo para almacenar en los nodos
struct ProcessInfo {
    int pid;
    string name;
    string user;

    ProcessInfo() : pid(0), name(""), user("") {}
    ProcessInfo(int p, const string& n, const string& u) : pid(p), name(n), user(u) {}
};

// Functor personalizado para decirle a generate_tree_graphviz cómo formatear la etiqueta del nodo
struct ProcessWriter {
    string operator()(Tree_Node<ProcessInfo>* node) const {
        const ProcessInfo& info = node->get_key();
        return info.name + "\\nPID: " + to_str(info.pid) + "\\nUser: " + info.user;
    }
};

int main() {
    cout << "Generando un Tree_Node complejo (arbol de procesos)..." << endl;

    // 1. Crear la raiz (systemd)
    auto root = new Tree_Node<ProcessInfo>(ProcessInfo(1, "systemd", "root"));

    // 2. Crear hijos principales
    auto bash_login = new Tree_Node<ProcessInfo>(ProcessInfo(1024, "bash", "lrleon"));
    auto cron_daemon = new Tree_Node<ProcessInfo>(ProcessInfo(401, "cron", "root"));
    auto xorg_server = new Tree_Node<ProcessInfo>(ProcessInfo(850, "Xorg", "root"));
    auto dbus_daemon = new Tree_Node<ProcessInfo>(ProcessInfo(900, "dbus-daemon", "messagebus"));

    // Insertar hijos de root
    root->insert_rightmost_child(bash_login);
    root->insert_rightmost_child(cron_daemon);
    root->insert_rightmost_child(xorg_server);
    root->insert_rightmost_child(dbus_daemon);

    // 3. Crear nietos (procesos bajo bash)
    auto htop = new Tree_Node<ProcessInfo>(ProcessInfo(2048, "htop", "lrleon"));
    auto nvim = new Tree_Node<ProcessInfo>(ProcessInfo(2050, "nvim", "lrleon"));
    auto gcc_comp = new Tree_Node<ProcessInfo>(ProcessInfo(2055, "g++", "lrleon"));
    
    bash_login->insert_rightmost_child(htop);
    bash_login->insert_rightmost_child(nvim);
    bash_login->insert_rightmost_child(gcc_comp);

    // 4. Crear bisnietos (procesos lanzados por el compilador gcc)
    auto cc1plus = new Tree_Node<ProcessInfo>(ProcessInfo(2056, "cc1plus", "lrleon"));
    auto as = new Tree_Node<ProcessInfo>(ProcessInfo(2057, "as", "lrleon"));
    
    gcc_comp->insert_rightmost_child(cc1plus);
    gcc_comp->insert_rightmost_child(as);

    // 5. Mas hijos bajo otros nodos (tareas de cron)
    auto daily_backup = new Tree_Node<ProcessInfo>(ProcessInfo(3001, "tar", "root"));
    auto log_rotate = new Tree_Node<ProcessInfo>(ProcessInfo(3002, "logrotate", "root"));

    cron_daemon->insert_rightmost_child(daily_backup);
    cron_daemon->insert_rightmost_child(log_rotate);

    // 6. Procesos graficos
    auto wm = new Tree_Node<ProcessInfo>(ProcessInfo(910, "gnome-shell", "lrleon"));
    auto terminal = new Tree_Node<ProcessInfo>(ProcessInfo(950, "gnome-terminal", "lrleon"));
    auto browser = new Tree_Node<ProcessInfo>(ProcessInfo(980, "firefox", "lrleon"));

    xorg_server->insert_rightmost_child(wm);
    wm->insert_rightmost_child(terminal);
    wm->insert_rightmost_child(browser);

    // 7. Generar el archivo DOT usando nuestra funcion y el functor personalizado
    string filename = "process_tree.dot";
    ofstream out(filename);
    if (!out) {
        cerr << "Error al abrir el archivo " << filename << " para escritura." << endl;
        return 1;
    }

    // Llamada con el Functor de personalizacion
    generate_tree_graphviz<Tree_Node<ProcessInfo>, ProcessWriter>(root, out);
    out.close();
    
    cout << "Archivo '" << filename << "' generado exitosamente." << endl;
    cout << "Puedes visualizarlo ejecutando:" << endl;
    cout << "  dot -Tpng " << filename << " -o process_tree.png" << endl;

    // Limpiar memoria
    destroy_tree(root);

    return 0;
}
