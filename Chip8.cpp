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

//CHIP8 constructor which:
//-Sets the program counter register to the first instruction address, which is where the memory where the program is stored starts
//-Sets the random number generator and distribution
//-Creates the table of opcode to function mappings
Chip8::Chip8()
    : randGen(std::chrono::system_clock::now().time_since_epoch().count()),
      randByte(std::uniform_int_distribution<uint8_t>(0, 255U)) //Apparently this is better for declaring vars in the constructor because it handles errors better and does default constructor
{

    programCounter = START_ADDRESS;

    for (unsigned int i = 0; i < sizeof(FONTSET); ++i) { //In for loops, ++i or i++ work, but conventionally ++i is used because if you set a variable as i++, it sets the variable first THEN increments i, so good practice to do ++i
        memory[FONT_ADDRESS + i] = FONTSET[i];
    }


    //Why is this the best strategy for mapping opcodes to functions? We could have a
    //set of if statements or a switch statement but that would be unweildy for a program
    //with many opcodes. Instead, we have a table that we can easily input the opcode
    //into and have it spit out a pointer reference to the function, which we will use to
    //call the function. Why not have one giant table that goes from 0x0000 to 0xFF55?
    //Because that would be an array that has thousands of spaces, most of which would be
    //empty. This strategy allows us to input an opcode into a table that is a manageable size,
    //with the downside that we reference another set of tables to get more specific for certain
    //opcodes which require more bits to check which distinct opcode it is.

    //Based on first digit
    table[0x0] = &Chip8::Table0;
    table[0x1] = &Chip8::JUMP_1nnn;
    table[0x2] = &Chip8::CALL_2nnn;
    table[0x3] = &Chip8::SE_3xkk;
    table[0x4] = &Chip8::SNE_4xkk;
    table[0x5] = &Chip8::SE_5xy0;
    table[0x6] = &Chip8::LD_6xkk;
    table[0x7] = &Chip8::ADD_7xkk;
    table[0x8] = &Chip8::Table8;
    table[0x9] = &Chip8::SNE_9xy0;
    table[0xA] = &Chip8::LD_Annn;
    table[0xB] = &Chip8::JP_Bnnn;
    table[0xC] = &Chip8::RND_Cxkk;
    table[0xD] = &Chip8::DRW_Dxyn;
    table[0xE] = &Chip8::TableE;
    table[0xF] = &Chip8::TableF;

    std::fill(table0, table0 + sizeof(table0), &Chip8::OP_NULL);
    std::fill(table8, table8 + sizeof(table8), &Chip8::OP_NULL);
    std::fill(tableE, tableE + sizeof(tableE), &Chip8::OP_NULL);
    std::fill(tableF, tableF + sizeof(tableF), &Chip8::OP_NULL);

    //Based on fourth digit
    table0[0x0] = &Chip8::CLS_00E0;
    table0[0xE] = &Chip8::RET_00EE;

    //Based on fourth digit
    table8[0x0] = &Chip8::LD_8xy0;
    table8[0x1] = &Chip8::OR_8xy1;
    table8[0x2] = &Chip8::AND_8xy2;
    table8[0x3] = &Chip8::XOR_8xy3;
    table8[0x4] = &Chip8::ADD_8xy4;
    table8[0x5] = &Chip8::SUB_8xy5;
    table8[0x6] = &Chip8::SHR_8xy6;
    table8[0x7] = &Chip8::SUBN_8xy7;
    table8[0xE] = &Chip8::SHL_8xyE;

    //Based on fourth digit
    tableE[0x1] = &Chip8::SKP_Ex9E;
	tableE[0xE] = &Chip8::SKNP_ExA1;

    //Based on third and fourth digit
    tableF[0x07] = &Chip8::LD_Fx07;
    tableF[0x0A] = &Chip8::LD_Fx0A;
    tableF[0x15] = &Chip8::LD_Fx15;
    tableF[0x18] = &Chip8::LD_Fx18;
    tableF[0x1E] = &Chip8::ADD_Fx1E;
    tableF[0x29] = &Chip8::LD_Fx29;
    tableF[0x33] = &Chip8::LD_Fx33;
    tableF[0x55] = &Chip8::LD_Fx55;
    tableF[0x65] = &Chip8::LD_Fx65;


    typedef void (Chip8::*Chip8Func)(); //Easy to read way of making function pointers. Chip8Func is a function pointer, and we are making tables of this. This typedef command specifies that this itself is a pointer to a void function, which will be dereferenced upon conversion. Can use this command to create your own type names for readability.
    Chip8Func table[0xF + 1];
    Chip8Func table0[0xE + 1];
	Chip8Func table8[0xE + 1];
	Chip8Func tableE[0xE + 1];
	Chip8Func tableF[0x65 + 1];

}

//Calls specific opcode if starting with 0x0
void Chip8::Table0() {
    ((*this).*(table0[opcode & 0x000Fu]))(); //Dereferencing both the class instance itself and its function to call it. Guess it may not be necessary but good practice? If there are free functions with the same name
}

//Calls specific opcode if starting with 0x0
void Chip8::Table8() {
    ((*this).*(table8[opcode & 0x000Fu]))();
}

//Calls specific opcode if starting with 0x0
void Chip8::TableE() {
    ((*this).*(tableE[opcode & 0x000Fu]))();
}

//Calls specific opcode if starting with 0x0
void Chip8::TableF() {
    ((*this).*(tableF[opcode & 0x00FFu]))();
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
    uint16_t newAddress = opcode & 0x0FFFu; //Smart way of getting specific bit substrings you want to check out of an integer
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
void Chip8::SUBN_8xy7() {
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
void Chip8::SHL_8xyE() {
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
//Draws this sprite at position (x, y) in display, using register x and y
//Then sets overflow register to 1 if sprite has collided with another sprite
//We know the sprite's width will be 8 pixels, but not height, which is what n stands for
void Chip8::DRW_Dxyn() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t y = (opcode & 0x00F0u) >> 4u;
    uint8_t height = (opcode & 0x000Fu);

    uint8_t xDisplay = registers[x] % VIDEO_WIDTH; //Can overflow beyond screen size so we wrap around
    uint8_t yDisplay = registers[y] % VIDEO_HEIGHT;

    for (unsigned int i = 0; i < height; ++i) {

        uint8_t spriteRow = memory[indexRegister + i];

        for (unsigned int j = 0; j < 8; ++j) {

            uint8_t spritePixel = (spriteRow >> j) & 0x01u;
            uint32_t* screenPixel = &display[(xDisplay + i) * VIDEO_WIDTH + (yDisplay + j)] //I was confused by this line at first. This method is probably marginally more efficient than just referencing the value in the array again and again?

            if (spritePixel) {

                if (*screenPixel == 0xFFFFFFFF) {
                    registers[sizeof(registers) - 1] = 1;
                }

                *screenPixel ^= 0xFFFFFFFF;

            }

        }

    }
}

//Skip next instruction if key with value at xth register is pressed
void Chip8::SKP_Ex9E() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;

    uint8_t key = registers[x];

    if (keys[key]) {
        programCounter += 2;
    }
}

//Skip next instruction if key with value at xth register is NOT pressed
void Chip8::SKNP_ExA1() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;

    uint8_t key = registers[x];

    if (!keys[key]) {
        programCounter += 2;
    }
}

//Set xth register to the value of the delay timer
void Chip8::LD_Fx07() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;

    registers[x] = delayTimer;
}

//Wait until a key is pressed, then store the key value in the xth register
//We do a "wait" by repeatedly moving the program counter back 2, which keeps it on the
//same instruction
void Chip8::LD_Fx0A() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;

    for (int i = 0; i < sizeof(keys); ++i) { //Not sure if more efficient than simple if else statements but was a lot easier to write
        if (keys[i]) {
            register[x] = i;
            return;
        }
    }

    programCounter -= 2;
}

//Set delay timer to the value of the xth register
void Chip8::LD_Fx15() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;

    delayTimer = registers[x];
}

//Set sound timer to the value of the xth register
void Chip8::LD_Fx18() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;

    soundTimer = registers[x];
}

//Add xth register value to index register
void Chip8::ADD_Fx1E() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;

    indexRegister += registers[x];
}

//Set index register to location of ith font sprite, i being value from xth register
void Chip8::LD_Fx29() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;

    indexRegister = FONTSET[FONT_ADDRESS + 5 * registers[x]];
}

//Stores BCD (binary coded decimal) value of xth register in index register, index + 1, index + 2
//index register = hundreds digit, index + 1 = tens digit, index + 2 = ones digit
void Chip8::LD_Fx33() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;
    uint8_t value = registers[x];

    memory[indexRegister + 2] = value % 10;
    value /= 10;

    memory[indexRegister + 1] = value % 10;
    value /= 10;

    memory[indexRegister] = value % 10;
}

//Store values from 0th to xth registers in memory starting from index register
void Chip8::LD_Fx55() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i <= x; ++i) {
        memory[indexRegister + i] = registers[i];
    }
}

//Read values from 0th to xth registers in memory starting from index register, and store in registers
void Chip8::LD_Fx65() {
    uint8_t x = (opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i <= x; ++i) {
        registers[i] = memory[indexRegister + i];
    }
}

//Null function used for invalid opcodes
void Chip8::OP_NULL() {}









//Function that accomplishes everything that occurs within one cycle of the CHIP8 CPU
void Chip8::Cycle() {

    opcode = (memory[programCounter] << 8u) | memory[programCounter + 1]; //Bitwise OR when the first byte is moved 8 spaces left just adds that byte to the next 8 spaces

    programCounter += 2;

    ((*this).*(table[(opcode & 0xF000u) >> 12u]))();

    if (delayTimer > 0) {
        --delayTimer;
    }
    if (soundTimer > 0) {
        --soundTimer;
    }

}