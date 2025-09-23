//Apparently you need header files to declare all the methods you will define for a class

#include <cstdint>
#include <fstream>
#include <chrono>
#include <random>

//The Chip8 computer and its specifications
class Chip8 {

    public:
        Chip8();
        void loadROM(char const* fileName);
        void Cycle();

        uint8_t keys[16]{}; //The key to input mappings
        
        uint32_t display[64 * 32]{}; //Current pixel display values

    private:
        void Table0();
        void Table8();
        void TableE();
        void TableF();

        void CLS_00E0();
        void RET_00EE();
        void JUMP_1nnn();
        void CALL_2nnn();
        void SE_3xkk();
        void SNE_4xkk();
        void SE_5xy0();
        void LD_6xkk();
        void ADD_7xkk();
        void LD_8xy0();
        void OR_8xy1();
        void AND_8xy2();
        void XOR_8xy3();
        void ADD_8xy4();
        void SUB_8xy5();
        void SHR_8xy6();
        void SUBN_8xy7();
        void SHL_8xyE();
        void SNE_9xy0();
        void LD_Annn();
        void JP_Bnnn();
        void RND_Cxkk();
        void DRW_Dxyn();
        void SKP_Ex9E();
        void SKNP_ExA1();
        void LD_Fx07();
        void LD_Fx0A();
        void LD_Fx15();
        void LD_Fx18();
        void ADD_Fx1E();
        void LD_Fx29();
        void LD_Fx33();
        void LD_Fx55();
        void LD_Fx65();
        void OP_NULL();

        uint8_t registers[16]{}; //The registers with which the CPU will perform its operations
        uint8_t memory[4096]{}; //4kb of memory for the computer
        uint16_t indexRegister{}; //The register for memory space addresses for operations performed by the CPU
        uint16_t programCounter{}; //The register for the ADDRESS of the next instruction

        uint16_t stack[16]{}; //The stack of memory instructions to return to upon RET calls
        uint8_t stackPointer{}; //The current index of the stack

        uint8_t delayTimer{}; //The timer (60 Hz)
        uint8_t soundTimer{}; //The timer for playing sound, which will turn off on next cycle unless kept on (60 Hz)

        uint16_t opcode; //The actual instruction we are currently looking at


        std::default_random_engine randGen; //Random number generator seeded by the clock
        std::uniform_int_distribution<unsigned int> randByte; //Uniform distribution of integer range


        typedef void (Chip8::*Chip8Func)(); //Easy to read way of making function pointers. Chip8Func is a function pointer, and we are making tables of this. This typedef command specifies that this itself is a pointer to a void function, which will be dereferenced upon conversion. Can use this command to create your own type names for readability.
        Chip8Func table[0xF + 1];
        Chip8Func table0[0xE + 1];
        Chip8Func table8[0xE + 1];
        Chip8Func tableE[0xE + 1];
        Chip8Func tableF[0x65 + 1];

};