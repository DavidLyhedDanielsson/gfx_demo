#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <iostream>
#include <thread>

// clang-format off
// Include current demo only
#define XSTR(x) #x
#define STR(x) XSTR(x)
#include STR(graphics/dx12/demo/DEMO_NAME.hpp)
// clang-format on

#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_syswm.h>

int main(int argc, char** argv)
{
    std::srand(time(NULL));

    if(SDL_Init(SDL_INIT_VIDEO) < 0)
        return 1;

    const uint32_t windowWidth = 1280;
    const uint32_t windowHeight = 720;

    SDL_Window* sdlWindow =
        SDL_CreateWindow("Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, 0);

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version)
    SDL_GetWindowWMInfo(sdlWindow, &wmInfo);

    dx12_demo::DEMO_NAME::init(wmInfo.info.win.window, windowWidth, windowHeight);

    bool running = true;
    while(running)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                running = false;
                break;
            }

            if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                running = false;
                break;
            }
        }

        if(!running)
            break;

        dx12_demo::DEMO_NAME::render(windowWidth, windowHeight);
    }

    return 0;
}