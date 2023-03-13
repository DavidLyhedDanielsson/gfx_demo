#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <thread>

#include <asset/asset.hpp>
#include <asset/asset_loader.hpp>
#include <asset/io_job.hpp>

#include <graphics/dx12/demos/ndc_triangle.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#define STB_IMAGE_IMPLEMENTATION
#ifdef _WIN32
    #define STBI_WINDOWS_UTF8
#endif
#include <stb_image.h>

struct Texture
{
    static constexpr int ID = __COUNTER__;
};

int main(int argc, char** argv)
{
    AssetLoader loader;
    loader.registerLoader<Texture>(+[](AssetLoader& loader, const std::filesystem::path& path) -> Loader {
        int width;
        int height;
        int channels;

#ifdef _WIN32
        char stbiBuf[512];
        stbi_convert_wchar_to_utf8(stbiBuf, sizeof(stbiBuf), path.c_str());

        auto stbiData = std::unique_ptr<stbi_uc, void (*)(void*)>(
            stbi_load(stbiBuf, &width, &height, &channels, STBI_rgb_alpha),
            stbi_image_free);
#else
        auto stbiData = std::unique_ptr<stbi_uc, void (*)(void*)>(
            stbi_load(this->path.c_str(), &width, &height, &channels, STBI_rgb_alpha),
            stbi_image_free);
#endif

        // TODO
    });

    // loader.load<Texture>("Something");

    if(SDL_Init(SDL_INIT_VIDEO) < 0)
        return 1;

    const uint32_t windowWidth = 1280;
    const uint32_t windowHeight = 720;

    SDL_Window* sdlWindow =
        SDL_CreateWindow("Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, 0);

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version)
    SDL_GetWindowWMInfo(sdlWindow, &wmInfo);

    DX12::init(wmInfo.info.win.window, 1280, 720);

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

        DX12::render(windowWidth, windowHeight);
    }

    loader.stopRunning();
    loader.wait();

    return 0;
}