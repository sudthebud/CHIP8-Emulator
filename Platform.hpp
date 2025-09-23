#include <cstdint>
#include <SDL.h>

//For some reason, importing the SDL library would not work with this file. Found this solution thanks to Akash Sharma (akash5852)
class SDL_Window;
class SDL_Renderer;
class SDL_Texture;

//Graphics and input class for the emulator
class Platform {

    private:
        SDL_Window* window{};
        SDL_Renderer* renderer{};
        SDL_Texture* texture{};

    public:
        
        Platform(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight);

        ~Platform();

        void Update(void const* buffer, int pitch);

        bool ProcessInput(uint8_t* keys);

};