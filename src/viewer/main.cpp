// shoal_viewer: step through a game record (or a bare map) with an eval bar and the move list;
// --screenshot renders one frame offscreen and exits without opening a window
#include "board_draw.hpp"
#include "input.hpp"
#include "layout.hpp"
#include "palette.hpp"
#include "play_mode.hpp"
#include "replay.hpp"
#include "scripted_input.hpp"
#include "side_panel.hpp"
#include "state.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace shoal;

constexpr const char* kFontFile = "fonts/BarlowSemiCondensed-Medium.ttf";

struct Options {
    std::string file;
    std::string screenshot; // bmp path; empty opens the window
    int ply = -1;           // -1 is the last ply; other negatives count back from the end
    bool ply_given = false;
    int width = 1280;
    int height = 800;
    bool play = false;
    std::string select; // tile to pick in play mode, for screenshots
    std::string dest;   // then a destination, which opens the action menu
    std::string events; // scripted input played before the screenshot
};

void PrintUsage() {
    std::printf("usage: shoal_viewer <map|game.sexp> [--screenshot out.bmp] [--ply N]\n"
                "                    [--size WxH] [--play] [--select TILE] [--dest TILE]\n"
                "                    [--events right,up,click:X:Y,wheel:N:X:Y,...]\n");
}

bool ParseArgs(int argc, char** argv, Options* options) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        bool has_value = i + 1 < argc;
        if (arg == "--screenshot" && has_value) {
            options->screenshot = argv[++i];
        } else if (arg == "--ply" && has_value) {
            options->ply = std::atoi(argv[++i]);
            options->ply_given = true;
        } else if (arg == "--size" && has_value) {
            if (std::sscanf(argv[++i], "%dx%d", &options->width, &options->height) != 2) {
                return false;
            }
        } else if (arg == "--play") {
            options->play = true;
        } else if (arg == "--select" && has_value) {
            options->select = argv[++i];
            options->play = true;
        } else if (arg == "--dest" && has_value) {
            options->dest = argv[++i];
        } else if (arg == "--events" && has_value) {
            options->events = argv[++i];
        } else if (arg[0] == '-' || !options->file.empty()) {
            return false;
        } else {
            options->file = arg;
        }
    }
    return !options->file.empty() && options->width >= 200 && options->height >= 150;
}

bool LoadViewerFont(State* state) {
    std::string candidates[3] = {std::string(SHOAL_DATA_DIR) + "/" + kFontFile, "", ""};
    const char* base = SDL_GetBasePath();
    if (base) {
        candidates[1] = std::string(base) + "data/" + kFontFile;
        candidates[2] = std::string(base) + "../data/" + kFontFile;
    }
    for (const std::string& path : candidates) {
        if (!path.empty() && LoadFont(&state->font, state->renderer, path)) {
            return true;
        }
    }
    std::fprintf(stderr, "cannot find %s\n", kFontFile);
    return false;
}

void ApplyStartOptions(State* state, const Options& options) {
    int ply = LastPly(*state);
    if (options.ply_given) {
        ply = options.ply >= 0 ? options.ply : LastPly(*state) + 1 + options.ply;
    }
    SetPly(state, ply);
    state->scroll_to_ply = true;
    if (options.play) {
        state->mode = Mode::Play;
    }
}

// the play-mode picks need the layout, so they happen after the first layout pass
void ApplySelectOptions(State* state, const Options& options) {
    const Map& map = state->game.map;
    int tile = 0;
    if (options.select.empty() || !ParseTileName(map, options.select, &tile)) {
        return;
    }
    SelectTile(state, tile);
    int dest = 0;
    if (!options.dest.empty() && ParseTileName(map, options.dest, &dest)) {
        Rect rect = TileRect(*state, dest);
        PlayClick(state, rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f);
    }
}

void DrawFrame(State* state) {
    SDL_SetRenderDrawColor(state->renderer, kBackground.r, kBackground.g, kBackground.b, 255);
    SDL_RenderClear(state->renderer);
    DrawEvalBar(state);
    DrawBoard(state);
    DrawSidePanel(state);
    DrawPlayMenu(state);
}

void LayOut(State* state) {
    int w = 0;
    int h = 0;
    SDL_GetCurrentRenderOutputSize(state->renderer, &w, &h);
    ComputeLayout(state, static_cast<float>(w), static_cast<float>(h));
}

int RunScreenshot(State* state, const Options& options) {
    // a software renderer on a plain surface: no window, no display
    SDL_Surface* surface =
        SDL_CreateSurface(options.width, options.height, SDL_PIXELFORMAT_XRGB8888);
    if (!surface) {
        std::fprintf(stderr, "surface: %s\n", SDL_GetError());
        return 1;
    }
    state->renderer = SDL_CreateSoftwareRenderer(surface);
    if (!state->renderer || !LoadViewerFont(state)) {
        std::fprintf(stderr, "renderer: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetRenderDrawBlendMode(state->renderer, SDL_BLENDMODE_BLEND);
    LayOut(state);
    ApplySelectOptions(state, options);
    std::vector<SDL_Event> events;
    std::string error;
    if (!options.events.empty() && !ParseInputScript(options.events, &events, &error)) {
        std::fprintf(stderr, "--events: %s\n", error.c_str());
        return 1;
    }

    // a frame before each scripted event fills menu hit boxes and scroll, as on screen
    DrawFrame(state);
    for (SDL_Event& event : events) {
        HandleEvent(state, &event);
        LayOut(state);
        DrawFrame(state);
    }
    DrawFrame(state);
    SDL_Surface* pixels = SDL_RenderReadPixels(state->renderer, nullptr);
    bool ok = pixels && SDL_SaveBMP(pixels, options.screenshot.c_str());
    if (!ok) {
        std::fprintf(stderr, "screenshot: %s\n", SDL_GetError());
    } else {
        std::printf("%s: ply %d of %d\n", options.screenshot.c_str(), state->ply, LastPly(*state));
    }
    SDL_DestroySurface(pixels);
    FreeFont(&state->font);
    SDL_DestroyRenderer(state->renderer);
    SDL_DestroySurface(surface);
    return ok ? 0 : 1;
}

int RunWindow(State* state, const Options& options) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    std::string title = "shoal  " + state->path;
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (!SDL_CreateWindowAndRenderer(title.c_str(), options.width, options.height, flags,
                                     &state->window, &state->renderer)) {
        std::fprintf(stderr, "window: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetWindowMinimumSize(state->window, 640, 400);
    SDL_SetRenderVSync(state->renderer, 1);
    SDL_SetRenderDrawBlendMode(state->renderer, SDL_BLENDMODE_BLEND);
    if (!LoadViewerFont(state)) {
        return 1;
    }
    LayOut(state);
    ApplySelectOptions(state, options);

    while (state->running) {
        // pump input: sleep until something happens, then drain the queue
        SDL_Event event;
        if (SDL_WaitEventTimeout(&event, 500)) {
            HandleEvent(state, &event);
            while (SDL_PollEvent(&event)) {
                HandleEvent(state, &event);
            }
        }

        // lay out for the current window size
        LayOut(state);

        // draw frame
        DrawFrame(state);
        SDL_RenderPresent(state->renderer);
    }

    if (state->board_texture) {
        SDL_DestroyTexture(state->board_texture);
    }
    FreeFont(&state->font);
    SDL_DestroyRenderer(state->renderer);
    SDL_DestroyWindow(state->window);
    SDL_Quit();
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!ParseArgs(argc, argv, &options)) {
        PrintUsage();
        return 1;
    }
    auto state = std::make_unique<State>();
    std::string error;
    if (!LoadRecord(state.get(), options.file, &error)) {
        std::fprintf(stderr, "%s: %s\n", options.file.c_str(), error.c_str());
        return 1;
    }
    ApplyStartOptions(state.get(), options);
    if (!options.screenshot.empty()) {
        return RunScreenshot(state.get(), options);
    }
    return RunWindow(state.get(), options);
}
