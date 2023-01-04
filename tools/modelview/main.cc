/*
    neat - simple graphics engine
    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <memory>

#include <interface.hh>
#include <Log.hh>

#include "App.hh"

static constexpr auto Width = 1920;
static constexpr auto Height = 1080;

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
