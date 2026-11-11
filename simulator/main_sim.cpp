#include <lvgl.h>
#include <SDL2/SDL.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include "ui.h"

#define SIM_WIDTH 800
#define SIM_HEIGHT 480

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SIM_WIDTH * 40];
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

static void hal_init() {
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SIM_WIDTH * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SIM_WIDTH;
    disp_drv.ver_res = SIM_HEIGHT;
    disp_drv.flush_cb = [](lv_disp_drv_t *d, const lv_area_t *area, lv_color_t *px) {
        SDL_Rect r{area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1};
        SDL_UpdateTexture(texture, &r, px, (area->x2 - area->x1 + 1) * sizeof(lv_color_t));
        lv_disp_flush_ready(d);
    };
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = [](lv_indev_drv_t *d, lv_indev_data_t *data) {
        int x, y;
        uint32_t btn = SDL_GetMouseState(&x, &y);
        data->point.x = x;
        data->point.y = y;
        data->state = (btn & SDL_BUTTON_LMASK) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    };
    lv_indev_drv_register(&indev_drv);
}

static void sendRelaisMock(int relais, bool state) {
    printf("[SIM] Relais %d -> %s\n", relais + 1, state ? "ON" : "OFF");
    uiUpdateRelais(relais, state);
}

static void sendScenarioMock(const char *scenario) {
    printf("[SIM] Scenario: %s\n", scenario);
}

int main(int argc, char **argv) {
    srand(time(NULL));
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    window = SDL_CreateWindow("Atelier SIM - LVGL 800x480",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SIM_WIDTH, SIM_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, SIM_WIDTH, SIM_HEIGHT);

    hal_init();
    uiInit(sendRelaisMock, sendScenarioMock);

    float simCurrent = 0.0f;
    int tick = 0;
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) if (e.type == SDL_QUIT) running = false;

        lv_timer_handler();
        uiTick();

        static int clockTick = 0;
        if (++clockTick % 200 == 0) {
            char t[8];
            time_t now = time(NULL);
            struct tm *tm = localtime(&now);
            snprintf(t, sizeof(t), "%02d:%02d", tm->tm_hour, tm->tm_min);
            uiUpdateClock(t);
        }

        if (++tick % 20 == 0) {
            simCurrent = 2.0f + (rand() % 250) / 10.0f;
            uiUpdateCurrent(simCurrent);
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        usleep(5000);
    }
    SDL_Quit();
    return 0;
}
