#include <cstdint>
//The Chip8 computer and its specifications
class Chip8 {

    public:
        uint8_t registers[16]{}; //The registers with which the CPU will perform its operations
        uint8_t memory[4096]{}; //4kb of memory for the computer
        uint16_t indexRegister{}; //The register for memory space addresses for operations performed by the CPU
        uint16_t programCounter{}; //The register for the ADDRESS of the next instruction

        uint16_t stack[16]{}; //The stack of memory instructions to return to upon RET calls
        uint8_t stackPointer{}; //The current index of the stack

        uint8_t delayTimer{}; //The timer (60 Hz)
        uint8_t soundTimer{}; //The timer for playing sound, which will turn off on next cycle unless kept on (60 Hz)

        uint8_t keys{}; //The key to input mappings
        
        uint32_t display[64 * 32]{}; //Current pixel display values

        uint16_t opcode; //The actual instruction we are currently looking at


        std::default_random_engine randGen; //Random number generator seeded by the clock
        std::uniform_int_distribution<uint8_t> randByte; //Uniform distribution of integer range
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




#include <chrono>
#include <random>

//CHIP8 constructor which sets the program counter register to the first instruction address,
//which is where the memory where the program is stored starts
Chip8::Chip8()
    : randGen(std::chrono::system_clock::now().time_since_epoch().count()),
      randByte(std::uniform_int_distribution<uint8_t>(0, 255U))
{

    programCounter = START_ADDRESS;

    for (unsigned int i = 0; i < sizeof(FONTSET); ++i) {
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









//Opcode instructions


//Clears display
void Chip8::CLS_00E0() {
    memset(display, 0, sizeof(display));
}

//Returns from a subroutine
void Chip8::RET_00EE() {
    --stackPointer;
    programCounter = stack[stackPointer];
}

//Jumps to new address without adding to stack
void Chip8::JUMP_1nnn() {
    uint16_t newAddress = opcode & 0x0FFFu;
    programCounter = newAddress;
}

//Jumps to new address, adding to stack to be able to refer back to original line
void Chip8::CALL_2nnn() {
    stack[stackPointer] = programCounter;
    ++stackPointer;

    uint16_t newAddress = opcode & 0x0FFFu;
    programCounter = newAddress;
}

//Skips next instruction if value in xth register is equal to value of "kk"
void Chip8::SE_3xkk() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t kk = opcode & 0x00FF;

    if (registers[x] == kk) {
        programCounter += 2;
    }
}

//Skips next instruction if value in xth register is NOT equal to value of "kk"
void Chip8::SNE_4xkk() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t kk = opcode & 0x00FF;

    if (registers[x] != kk) {
        programCounter += 2;
    }
}

//Skips next instruction if value in xth register is equal to value in yth register
void Chip8::SE_5xy0() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;

    if (registers[x] == registers[y]) {
        programCounter += 2;
    }
}

//Sets value at xth register to kk
void Chip8::LD_6xkk() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t kk = opcode & 0x00FF;

    registers[x] = kk;
}

//Adds kk to value at xth register
void Chip8::ADD_7xkk() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t kk = opcode & 0x00FF;

    registers[x] += kk;
}

//Sets value at xth register to value at yth register
void Chip8::LD_8xy0() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;

    registers[x] = registers[y];
}

//Sets value at xth register to bitwise OR between xth register and yth register values
void Chip8::OR_8xy1() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;

    registers[x] |= registers[y];
}

//Sets value at xth register to bitwise AND between xth register and yth register values
void Chip8::AND_8xy2() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;

    registers[x] &= registers[y];
}

//Sets value at xth register to bitwise XOR between xth register and yth register values
void Chip8::XOR_8xy3() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;

    registers[x] &= registers[y];
}

//Adds value at yth register to xth register, marking overflow register as 1 if the value 
//overflows (is greater than 8 bits)
void Chip8::ADD_8xy4() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;

    uint8_t valX = registers[x];
    uint8_t valY = registers[y];

    uint8_t sum = valX + valY;
    registers[x] = sum & 0x00FFu;

    if (sum > 0x00FFu) {
        registers[sizeof(registers) - 1] = 1;
    }
    else {
        registers[sizeof(registers) - 1] = 0;
    }
}

//Subtracts value at yth register from xth register, marking overflow register as 1 if the value
//does NOT borrow (is NOT negative or 0)
void Chip8::SUB_8xy5() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;

    uint8_t valX = registers[x];
    uint8_t valY = registers[y];

    uint8_t sum = valX - valY;
    registers[x] = sum;

    if (valX > valY) {
        registers[sizeof(registers) - 1] = 1;
    }
    else {
        registers[sizeof(registers) - 1] = 0;
    }
}

//Divides value at xth register by 2 and sets overflow register to 1 if there is a decimal
//at the end (last digit before division is 1). Shifting bits to the right is equivalent to
//division by 2
void Chip8::SHR_8xy6() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t valX = registers[x];

    registers[sizeof(registers) - 1] = valX & 0x01u;
    
    registers[x] >>= 1;
}

//Subtracts value at xth register from yth register and sets it in xth register,
//marking overflow register as 1 if the value does NOT borrow (is NOT negative or 0)
void Chip8::SUB_8xy7() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;

    uint8_t valX = registers[x];
    uint8_t valY = registers[y];

    uint8_t sum = valX=Y - valX;
    registers[x] = sum;

    if (valY > valX) {
        registers[sizeof(registers) - 1] = 1;
    }
    else {
        registers[sizeof(registers) - 1] = 0;
    }
}

//Multiplies value at xth register by 2 and sets overflow register to 1 if there is an
//overflow (first digit before multiplication is 1). Shifting bits to the left is equivalent to
//multiplication by 2
void Chip8::SHL_8xy7() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t valX = registers[x];

    registers[sizeof(registers) - 1] = (valX & 0x80u) >> 7;
    
    registers[x] <<= 1;
}

//Skips next instruction if value at xth register and yth registers are NOT equal
void Chip8::SNE_9xy0() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;

    uint8_t valX = registers[x];
    uint8_t valY = registers[y];

    if (valX != valY) {
        programCounter += 2;
    }
}

//Sets value of index register equal to given address nnn
void Chip8::LD_Annn() {
    uint16_t address = opcode & 0x0FFFu;
    indexRegister = address;
}

//Jumps to address nnn + value at 1st register
void Chip8::JP_Bnnn() {
    uint16_t address = opcode & 0x0FFFu;
    programCounter = address + registers[0];
}

//Sets xth register to bitwise AND of random number and kk
void Chip8::RND_Cxkk() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t kk = opcode & 0x00FFu;

    registers[x] = randByte(randGen) & kk;
}

//Draws sprite given by memory location saved in index register
//Draws this sprite at position (x, y) in display
//Then sets overflow register to 1 if sprite has collided with another sprite
//We know the sprite's width will be 8 pixels, but not height, which is what n stands for
void Chip8::DRW_Dxyn() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;
    uint8_t height = (opcode & 0x000Fu);

    for (unsigned int i = 0; i < height; i++) {

        uint8_t spriteRow = memory[indexRegister + i];

        for (unsigned int j = 0; j < )

    }
}