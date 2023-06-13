#include "mengubahui.h"


int main() {
    nanogui::init();

    Mengu::MengubahEngine engine;
    Mengu::MengubahUI *app = new Mengu::MengubahUI(&engine);
    app->set_visible(true);
    app->draw_all();

    try {
        nanogui::mainloop((1.0 / Mengu::MengubahUI::FramesPerSecond) * 1'000);
    }
    catch(std::exception e) {
        std::cout << e.what() << std::endl;
    }
    nanogui::shutdown();

    delete app;

    return 0;
}