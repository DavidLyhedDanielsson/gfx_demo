#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <iostream>
#include <thread>

#include <graphics/dx12/demos/cubed_cat.hpp>
#include <graphics/dx12/demos/ndc_triangle.hpp>
#include <graphics/dx12/demos/perspective_cat.hpp>
#include <graphics/dx12/demos/spinning_cat.hpp>
#include <graphics/dx12/demos/spinning_quad.hpp>
#include <graphics/dx12/demos/spinning_triangle.hpp>
#include <graphics/dx12/demos/vertex_triangle_ia.hpp>
#include <graphics/dx12/demos/vertex_triangle_pull.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

struct Texture
{
    static constexpr int ID = __COUNTER__;
};

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

    // DX12Demo::NDCTriangle::init(wmInfo.info.win.window, 1280, 720);
    // DX12Demo::VertexTriangleIA::init(wmInfo.info.win.window, 1280, 720);
    // DX12Demo::VertexTrianglePull::init(wmInfo.info.win.window, 1280, 720);
    // DX12Demo::SpinningTriangle::init(wmInfo.info.win.window, 1280, 720);
    // DX12Demo::SpinningQuad::init(wmInfo.info.win.window, 1280, 720);
    // DX12Demo::SpinningCat::init(wmInfo.info.win.window, 1280, 720);
    // DX12Demo::PerspectiveCat::init(wmInfo.info.win.window, 1280, 720);
    DX12Demo::CubedCat::init(wmInfo.info.win.window, 1280, 720);

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
        }

        if(!running)
            break;

        DX12Demo::CubedCat::render(windowWidth, windowHeight);
    }

    return 0;
}