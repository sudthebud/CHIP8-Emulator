#include <chrono>
#include <iostream>

int main(int argc, char__ argv) { //Main method which all C++ programs start from, with argc being num args and argv being the list of args passed through

    int VIDEO_WIDTH = 32;
    int VIDEO_HEIGHT = 16;

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
        std::exit(EXIT_FAILURE);
    }

    int videoScale = std::stoi(argv[1]); //Interpret signed integer from string
    int cycleDelay = std::stoi(argv[2]);
    char* const romFilename = argv[3];

    Platform platform("CHIP-8 Emulator", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);

    Chip8 chip8;
    chip8.loadROM(romFilename);

    int videoPitch = sizeof(chip8.video[0]) * VIDEO_WIDTH;

    auto lastCycleTime = std::chrono::high_resolution_clock::now();
    bool quit = false;

    //Keeps running to update the program
    while (!quit) {

        quit = platform.ProcessInput(chip8.keys);

        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();

        if (dt > cycleDelay) {
            lastCycleTime = currentTime;
            chip8.Cycle();
            platform.Update(chip8.display, videoPitch);
        }

    }

    return 0;

}