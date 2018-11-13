#include <SDL.h>
#include <SDL_ttf.h>

struct grid_cell {
    char c;
    SDL_Color *fg;
    SDL_Color *bg;
};

void boat_move(int *x, int *y, int dx, int dy, unsigned time,
               unsigned *drift_timeout) {
    static unsigned timeout = 0;

    if (timeout < time) {
        *x += dx;
        *y += dy;

        timeout = time + (dx < 0 ? 60 : 120);

        if (*drift_timeout < time)
            *drift_timeout = time + 1000;
    }
}

int main() {
    int grid_w = 83;
    int grid_h = 28;
    int grid_size = grid_w * grid_h;
    int grid_cell_w = 0;
    int grid_cell_h = 0;

    struct grid_cell *grid =
        calloc((size_t)grid_size, sizeof(struct grid_cell));

    char waves[] = {'/', '\\'};
    int waves_pos = 0;
    int waves_pos_last = waves_pos;
    unsigned waves_pos_time = 0;
    int waves_gap = 10;
    int waves_interval = 1000;

    //    _
    //   |o|
    // \_____/
    char *boat[] = {"   _   ", "  |o|  ", "\\_____/"};
    int boat_w = 7;
    int boat_h = 3;
    int boat_x = (grid_w - boat_w) / 2;
    int boat_y = (grid_h - boat_h) / 2;
    unsigned boat_drift_timeout = 0;

    // Light
    // SDL_Color bg = {255, 255, 255, 255};
    // SDL_Color waves_colors[] = {{55, 55, 55, 255}, {22, 22, 22, 255}};
    // SDL_Color boat_color = {0, 0, 0, 255};

    // Dark
    SDL_Color bg = {22, 22, 22, 255};
    SDL_Color waves_colors[] = {{200, 200, 200, 255}, {99, 99, 99, 255}};
    SDL_Color boat_color = {255, 255, 255, 255};

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Initialize SDL: %s",
                     SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Initialize SDL_ttf: %s",
                     TTF_GetError());
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("assets/FiraMono-Regular.ttf", 20);
    if (font == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Open font: %s",
                     TTF_GetError());
        return 1;
    }

    if (TTF_SizeText(font, " ", &grid_cell_w, &grid_cell_h) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Get text metrics: %s",
                     TTF_GetError());
        return 1;
    }

    SDL_Cursor *cursor;
    cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    if (cursor == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Get the system hand cursor: %s", SDL_GetError());
    }
    SDL_SetCursor(cursor);

    int window_w = grid_w * grid_cell_w;
    int window_h = grid_h * grid_cell_h;
    SDL_Window *window;
    SDL_Renderer *renderer;
    if (SDL_CreateWindowAndRenderer(window_w, window_h, 0, &window, &renderer) <
        0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Create window and renderer: %s", SDL_GetError());
        return 1;
    }

    SDL_SetWindowTitle(window, "SDL ASCII");

    SDL_bool done = SDL_FALSE;
    while (!done) {
        unsigned time_now = SDL_GetTicks();

        int boat_move_y = 0;
        int boat_move_x = 0;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_w:
                case SDLK_UP:
                    boat_move_y = -1;
                    break;
                case SDLK_s:
                case SDLK_DOWN:
                    boat_move_y = 1;
                    break;
                case SDLK_a:
                case SDLK_LEFT:
                    boat_move_x = -1;
                    break;
                case SDLK_d:
                case SDLK_RIGHT:
                    boat_move_x = 1;
                    break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.motion.x > (boat_x + boat_w) * grid_cell_w)
                    boat_move_x = 1;
                else if (event.motion.x < boat_x * grid_cell_w)
                    boat_move_x = -1;

                if (event.motion.y > (boat_y + boat_h) * grid_cell_h)
                    boat_move_y = 1;
                else if (event.motion.y < boat_y * grid_cell_h)
                    boat_move_y = -1;
                break;
            case SDL_QUIT:
                done = SDL_TRUE;
                break;
            }
        }

        if (boat_move_x != 0 || boat_move_y != 0)
            boat_move(&boat_x, &boat_y, boat_move_x, boat_move_y, time_now,
                      &boat_drift_timeout);

        // Update and draw waves to grid.
        waves_pos_last = waves_pos;
        if (waves_pos_time + (unsigned)waves_interval < time_now) {
            waves_pos += 1;
            waves_pos_time = time_now;
        }

        for (int x = 0; x < grid_w; x++) {
            for (int y = 0; y < grid_h; y++) {
                struct grid_cell *cell = &grid[x + grid_w * y];

                cell->bg = &bg;

                if (waves_pos + x + y % waves_gap > 0) {
                    cell->c = waves[0];
                    cell->fg = &waves_colors[0];
                } else {
                    cell->c = waves[1];
                    cell->fg = &waves_colors[1];
                }
            }
        }

        // Update and draw boat to grid.
        if (waves_pos > waves_pos_last && boat_drift_timeout < time_now)
            boat_x -= 1;

        for (int x = 0; x < boat_w; x++) {
            for (int y = 0; y < boat_h; y++) {
                if (boat[y][x] != ' ') {
                    struct grid_cell *cell =
                        &grid[boat_x + x + (grid_w * (boat_y + y))];

                    cell->c = boat[y][x];
                    cell->fg = &boat_color;
                    cell->bg = &bg;
                }
            }
        }

        // Update and draw grid to window.
        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderClear(renderer);

        SDL_Rect dest = {
            .x = 0,
            .y = 0,
            .w = grid_cell_w,
            .h = grid_cell_h,
        };

        for (int x = 0; x < grid_w; x++) {
            for (int y = 0; y < grid_h; y++) {
                dest.x = grid_cell_w * x;
                dest.y = grid_cell_h * y;

                struct grid_cell *cell = &grid[x + grid_w * y];

                SDL_Surface *surface = TTF_RenderGlyph_Shaded(
                    font, (Uint16)cell->c, *cell->fg, cell->bg);

                SDL_Texture *texture =
                    SDL_CreateTextureFromSurface(renderer, surface);

                SDL_SetRenderDrawColor(renderer, cell->bg->r, cell->bg->g,
                                       cell->bg->b, cell->bg->a);
                SDL_RenderFillRect(renderer, &dest);

                SDL_RenderCopy(renderer, texture, NULL, &dest);

                SDL_DestroyTexture(texture);
                SDL_FreeSurface(surface);
            }
        }

        SDL_RenderPresent(renderer);
    }

    free(grid);

    TTF_CloseFont(font);
    TTF_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
