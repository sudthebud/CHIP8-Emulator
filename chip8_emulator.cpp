#include <cstdint>
//The Chip8 computer and its specifications
class Chip8 {

    public:
        uint8_t registers[16]{}; //The registers with which the CPU will perform its operations
        uint8_t memory[4096]{}; //4kb of memory for the computer
        uint16_t indexRegister{}; //The register for memory spaces for operations performed by the CPU
        uint16_t programCounter{}; //The register for the address of the next instruction

        uint16_t stack[16]{}; //The stack of memory instructions to return to upon RET calls
        uint8_t stackPointer{}; //The current index of the stack

        uint8_t delayTimer{}; //The timer (60 Hz)
        uint8_t soundTimer{}; //The timer for playing sound, which will turn off on next cycle unless kept on (60 Hz)

        uint8_t keys{}; //The key to input mappings
        
        uint32_t display[64 * 32]{}; //Current pixel display values

        uint16_t opcode;
};




#include <fstream>

const unsigned int START_ADDRESS = 0x200; //Starting address of the memory where the ROM file is stored

//CHIP8 method which reads contents of a ROM file and loads it into memory
void Chip8::loadROM(char const* fileName) {

    //Creates file object from file name
    //Created as a stream of binary and moving pointer at the end of file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file.is_open()) {

        //Finds file size by getting current position of the file
        //Then creates an array of that size which will hold the ROM contents
        std::streampos fileSize = file.tellg();
        char* buffer = new char[fileSize];

        //Moves pointer to beginning of file
        //Saves file contents to buffer array
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        //Loads file contents into CHIP8 memory
        for (int i = 0; i < size; ++i) {
            memory[START_ADDRESS + i] = buffer[i];
        }

    }

}




//CHIP8 constructor which sets the program counter register to the first instruction address,
//which is where the memory where the program is stored starts
Chip8::Chip8() {

    programCounter = START_ADDRESS;

    for (int i = 0; i < sizeof(FONTSET); ++i) {
        memory[FONT_ADDRESS + i] = FONTSET[i];
    }

}




//Font characters for the CHIP8 emulator
//CHIP8 uses built in font sprites, and font sprites are represented as 1s and 0s in a 5x5
//grid, where 1 is a pixel on the screen to display
//We condense these 5x5 1s and 0s into 5 bytes (byte = 8 bits), one for each row of 1s and 0s
const unsigned int NUM_FONT_CHARACTERS = 16;

const unsigned int FONT_ADDRESS = 0x50;

uint8_t FONTSET[NUM_FONT_CHARACTERS * 5] = 
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
}