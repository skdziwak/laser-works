#include <gtkmm.h>

#include "interface.h"
#include <fstream>

using namespace std;

int main(int argc, char **argv) {
    Gtk::Main::init_gtkmm_internals();

    Interface interface;
    interface.run();
    interface.saveConfig();

}