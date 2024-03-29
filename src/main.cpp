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

    uint32_t windowWidth = 1280;
    uint32_t windowHeight = 720;

    uint32_t sdlWindowFlags = 0;
#ifdef DEMO_NAME_RESIZING
    sdlWindowFlags |= SDL_WINDOW_RESIZABLE;
#endif

    SDL_Window* sdlWindow = SDL_CreateWindow(
        "Window",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        windowWidth,
        windowHeight,
        sdlWindowFlags);

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

        // This is only acted on in certain demos, but I don't want to clutter the code with ifdefs
        std::optional<std::pair<uint32_t, uint32_t>> newDimensions;

        SDL_Event event;
        while(SDL_PollEvent(&event) && running)
        {
            switch(event.type)
            {
                case SDL_QUIT: running = false; break;
                case SDL_KEYDOWN: {
                    if(event.key.keysym.sym == SDLK_ESCAPE)
                        running = false;
                    break;
                }
                case SDL_WINDOWEVENT: {
                    if(event.window.event == SDL_WINDOWEVENT_RESIZED && event.window.data1 > 0
                       && event.window.data2 > 0)
                    {
                        newDimensions = {event.window.data1, event.window.data2};
                    }
                    break;
                }
                default: break;
            }
        }

        if(!running)
            break;

#ifdef DEMO_NAME_RESIZING
        if(newDimensions)
        {
            std::tie(windowWidth, windowHeight) = newDimensions.value();
            dx12_demo::DEMO_NAME::resize(wmInfo.info.win.window, windowWidth, windowHeight);
            continue;
        }
#endif

        dx12_demo::DEMO_NAME::render(windowWidth, windowHeight);
#ifdef DEMO_NAME_TIMING
        accumulatedGpuTime += dx12_demo::timing::getLastFrameTimeMS();
#endif
    }

    return 0;
}