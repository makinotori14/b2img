#include "b2img/gui/main_window.hpp"

#include <gtk/gtk.h>

namespace {

void activate(GtkApplication* application, gpointer) {
    auto* window = new b2img::gui::MainWindow(application);
    g_object_set_data_full(
        G_OBJECT(window->window()),
        "b2img-main-window",
        window,
        [](gpointer data) {
            delete static_cast<b2img::gui::MainWindow*>(data);
        }
    );
    window->present();
}

}  // namespace

int main(int argc, char** argv) {
    GtkApplication* application = gtk_application_new("com.b2img.frontend", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(application, "activate", G_CALLBACK(activate), nullptr);
    const int status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);
    return status;
}
