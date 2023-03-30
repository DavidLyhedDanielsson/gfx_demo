#include <SDL_video.h>
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

    auto lastTime = std::chrono::high_resolution_clock::now();

    float accumulatedCpuTime = 0.0f;
    int accumulatedIterations = 0;
#ifdef DEMO_NAME_TIMING
    float accumulatedGpuTime = 0.0f;
#endif

    bool running = true;
    while(running)
    {
        // This time will be very small in the first frame since `lastTime` is initialized just above this
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> floatTime = currentTime - lastTime;
        std::chrono::milliseconds frameTimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(floatTime);
        lastTime = currentTime;

        if(accumulatedIterations == 60)
        {
            char buffer[64]{0};

            float cpuTimeMS = accumulatedCpuTime / 60.0f;
#ifdef DEMO_NAME_TIMING
            float gpuTimeMS = accumulatedGpuTime / 60.0f;
            sprintf(buffer, "CPU: %f, GPU: %f", cpuTimeMS, gpuTimeMS);
#else
            sprintf(buffer, "CPU: %f", cpuTimeMS);
#endif

            SDL_SetWindowTitle(sdlWindow, buffer);

            accumulatedCpuTime = 0.0f;
            accumulatedIterations = 0;
#ifdef DEMO_NAME_TIMING
            accumulatedGpuTime = 0.0f;
#endif
        }

        accumulatedCpuTime += frameTimeMS.count();
        ++accumulatedIterations;

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
#ifdef DEMO_NAME_TIMING
        accumulatedGpuTime += dx12_demo::timing::getLastFrameTimeMS();
#endif
    }

    return 0;
}