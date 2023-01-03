#include <memory>

#include <interface.hh>
#include <Log.hh>

#include "App.hh"

static constexpr auto Width = 1920;
static constexpr auto Height = 1080;
static constexpr auto Dpi = 250;

static std::unique_ptr<App> _app;

void init(int argc, char* argv[]) {
    if (argc < 2) {
        throw std::runtime_error(
            std::string("usage: ") + argv[0] + " <model_file>");
    }
    _app = std::make_unique<App>(Width, Height, argv[1]);
}

int action(neat::Actions act, int x, int y) {
    return _app->action(act, x, y);
}

void queryData(NeatWindowData* data) {
    data->width = Width;
    data->height = Height;
    data->dpi = Dpi;
    data->caption = "Model Viewer";
    neat::Log::tag = data->caption;
}

enum { UpdateRate = 8 };

void draw(uint64_t time) {
    static uint64_t lasthit = time;
    size_t count = (time - lasthit) / UpdateRate;

    if (count > 0) {
        lasthit = time;
        _app->update(count);
    }
    _app->draw();
}
