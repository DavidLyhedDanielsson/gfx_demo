#include <SDL_keycode.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <iostream>
#include <thread>

#ifdef DEMO_NDC_TRIANGLE
    #include <graphics/dx12/demos/ndc_triangle.hpp>
#endif
#ifdef DEMO_VERTEX_TRIANGLE_IA
    #include <graphics/dx12/demos/vertex_triangle_ia.hpp>
#endif
#ifdef DEMO_VERTEX_TRIANGLE_PULL
    #include <graphics/dx12/demos/vertex_triangle_pull.hpp>
#endif
#ifdef DEMO_SPINNING_TRIANGLE
    #include <graphics/dx12/demos/spinning_triangle.hpp>
#endif
#ifdef DEMO_SPINNING_QUAD
    #include <graphics/dx12/demos/spinning_quad.hpp>
#endif
#ifdef DEMO_SPINNING_CAT
    #include <graphics/dx12/demos/spinning_cat.hpp>
#endif
#ifdef DEMO_PERSPECTIVE_CAT
    #include <graphics/dx12/demos/perspective_cat.hpp>
#endif
#ifdef DEMO_CUBED_CAT
    #include <graphics/dx12/demos/cubed_cat.hpp>
#endif

#include <SDL2/SDL.h>
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

#ifdef DEMO_NDC_TRIANGLE
    DX12Demo::NDCTriangle::init(wmInfo.info.win.window, windowWidth, windowHeight);
#endif
#ifdef DEMO_VERTEX_TRIANGLE_IA
    DX12Demo::VertexTriangleIA::init(wmInfo.info.win.window, windowWidth, windowHeight);
#endif
#ifdef DEMO_VERTEX_TRIANGLE_PULL
    DX12Demo::VertexTrianglePull::init(wmInfo.info.win.window, windowWidth, windowHeight);
#endif
#ifdef DEMO_SPINNING_TRIANGLE
    DX12Demo::SpinningTriangle::init(wmInfo.info.win.window, windowWidth, windowHeight);
#endif
#ifdef DEMO_SPINNING_QUAD
    DX12Demo::SpinningQuad::init(wmInfo.info.win.window, windowWidth, windowHeight);
#endif
#ifdef DEMO_SPINNING_CAT
    DX12Demo::SpinningCat::init(wmInfo.info.win.window, windowWidth, windowHeight);
#endif
#ifdef DEMO_PERSPECTIVE_CAT
    DX12Demo::PerspectiveCat::init(wmInfo.info.win.window, windowWidth, windowHeight);
#endif
#ifdef DEMO_CUBED_CAT
    DX12Demo::CubedCat::init(wmInfo.info.win.window, windowWidth, windowHeight);
#endif

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

#ifdef DEMO_NDC_TRIANGLE
        DX12Demo::NDCTriangle::render(windowWidth, windowHeight);
#endif
#ifdef DEMO_VERTEX_TRIANGLE_IA
        DX12Demo::VertexTriangleIA::render(windowWidth, windowHeight);
#endif
#ifdef DEMO_VERTEX_TRIANGLE_PULL
        DX12Demo::VertexTrianglePull::render(windowWidth, windowHeight);
#endif
#ifdef DEMO_SPINNING_TRIANGLE
        DX12Demo::SpinningTriangle::render(windowWidth, windowHeight);
#endif
#ifdef DEMO_SPINNING_QUAD
        DX12Demo::SpinningQuad::render(windowWidth, windowHeight);
#endif
#ifdef DEMO_SPINNING_CAT
        DX12Demo::SpinningCat::render(windowWidth, windowHeight);
#endif
#ifdef DEMO_PERSPECTIVE_CAT
        DX12Demo::PerspectiveCat::render(windowWidth, windowHeight);
#endif
#ifdef DEMO_CUBED_CAT
        DX12Demo::CubedCat::render(windowWidth, windowHeight);
#endif
    }

    return 0;
}