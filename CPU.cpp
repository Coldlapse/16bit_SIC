#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <bitset>
#include <stdexcept>

// ALU 클래스 (Arithmetic Logic Unit)
class ALU {
public:
    uint16_t add(uint16_t a, uint16_t b) { return a + b; }
    uint16_t mul(uint16_t a, uint16_t b) { return a * b; }
    uint16_t div(uint16_t a, uint16_t b) {
        if (b == 0) throw std::runtime_error("0으로 나눌 수 없습니다.");
        return a / b;
    }
    uint16_t mod(uint16_t a, uint16_t b) {
        if (b == 0) throw std::runtime_error("0으로 나눌 수 없습니다.");
        return a % b;
    }
};

// 메모리 클래스
class Memory {
private:
    static const int MEM_SIZE = 4096; // 메모리 크기: 0~4095
    unsigned int mpt = 0;            // 메모리 포인터 (유효범위: 0~4095)
    std::vector<uint8_t> mem;        // 8비트 단위 메모리 공간

public:
    Memory() : mem(MEM_SIZE, 0) {}

    uint16_t readWord(int address) {
        if (address < 0 || address >= MEM_SIZE - 1) throw std::out_of_range("주소 범위 초과");
        return (mem[address] << 8) | mem[address + 1];
    }

    void writeWord(int address, uint16_t value) {
        if (address < 0 || address >= MEM_SIZE - 1) throw std::out_of_range("주소 범위 초과");
        mem[address] = (value >> 8) & 0xFF;     // 상위 8비트
        mem[address + 1] = value & 0xFF;        // 하위 8비트
    }

    void dump(int start, int end, int format = 16) {
        if (start < 0 || end >= MEM_SIZE || start > end) throw std::out_of_range("주소 범위 초과");
        std::cout << "메모리 덤프 (" << (format == 16 ? "16진수" : "2진수") << "):\n";

        if (format == 16) {
            for (int i = start; i <= end; ++i) {
                std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)mem[i];
                if ((i - start + 1) % 16 == 0 || i == end) {
                    std::cout << "\n";
                } else {
                    std::cout << " ";
                }
            }
        } else {
            for (int i = start; i <= end; ++i) {
                std::cout << std::bitset<8>(mem[i]);
                if ((i - start + 1) % 8 == 0 || i == end) {
                    std::cout << "\n";
                } else {
                    std::cout << " ";
                }
            }
        }
    }
};

// 레지스터 클래스
class Register {
protected:
    uint16_t value;

public:
    Register() : value(0) {}
    uint16_t read() const { return value; }
    void write(uint16_t val) { value = val; }
};

// 디버깅 출력 함수
void debug(const Register& PC, const Register& IR, const Register& AC) {
    std::cout << "==================== DEBUG ====================\n";
    std::cout << "PC: " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << PC.read() << "\n";
    std::cout << "IR: " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << IR.read() << "\n";
    std::cout << "AC: " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << AC.read() << "\n";
    std::cout << "===============================================\n";
}

// CPU 클래스
class CPU {
private:
    Register PC; // Program Counter
    Register IR; // Instruction Register
    Register AC; // Accumulator
    ALU alu;     // Arithmetic Logic Unit
    Memory& memory;

public:
    CPU(Memory& mem) : memory(mem) {}

    void fetch() {
        uint16_t address = PC.read();
        uint16_t instruction = memory.readWord(address);
        IR.write(instruction);
        PC.write(address + 2); // Increment PC by 2 (16-bit instruction)
    }

    void execute() {
    uint16_t instruction = IR.read();
    uint16_t opcode = (instruction >> 12) & 0xF; // Extract 4-bit opcode
    uint16_t operand = instruction & 0xFFF;      // Extract 12-bit operand

    std::cout << "\nExecuting Opcode: " << std::hex << opcode 
              << ", Operand: " << std::hex << operand 
              << ", AC: " << std::hex << AC.read() << "\n";

    switch (opcode) {
    case 0xF: // SEA
        AC.write(operand);
        break;
    case 0x2: // ADD
        AC.write(alu.add(AC.read(), operand));
        break;
    case 0x3: // MUL
        AC.write(alu.mul(AC.read(), operand));
        break;
    case 0x4: // DIV
        AC.write(alu.div(AC.read(), operand));
        break;
    case 0x5: // MOD
        AC.write(alu.mod(AC.read(), operand));
        break;
    case 0x1: // STA
        std::cout << "STA writing AC: " << std::hex << AC.read() 
                  << " to address: " << operand << "\n";
        memory.writeWord(operand, AC.read());
        break;
    case 0x0: // LDA
        AC.write(memory.readWord(operand));
        break;
    default:
        throw std::runtime_error("알 수 없는 명령어");
    }
}


    void run() {
        while (true) {
            try {
                fetch();
                execute();

                // 디버깅 정보 출력
                debug(PC, IR, AC);

                // 사용자에게 dump 여부를 묻기
                std::cout << "dump? (y/n): ";
                char choice;
                std::cin >> choice;

                if (choice == 'y' || choice == 'Y') {
                    int start, end, format;
                    std::cout << "시작 주소, 끝 주소, 형식(2: 이진수, 16: 16진수)을 입력하세요: ";
                    std::cin >> start >> end >> format;

                    try {
                        memory.dump(start, end, format);
                    } catch (const std::exception& e) {
                        std::cerr << "오류: " << e.what() << "\n";
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "오류: " << e.what() << "\n";
                break;
            }
        }
    }
};

// 프로그램 로드 함수
void loadProgram(Memory& memory, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "파일을 열 수 없습니다.\n";
        return;
    }

    std::string line;
    int address = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // 명령어를 파싱
        std::string opcode = line.substr(0, line.find(' '));
        std::string operand = line.substr(line.find(' ') + 1);

        uint16_t opcodeBinary = 0;
        if (opcode == "SEA") opcodeBinary = 0xF;
        else if (opcode == "ADD") opcodeBinary = 0x2;
        else if (opcode == "MUL") opcodeBinary = 0x3;
        else if (opcode == "DIV") opcodeBinary = 0x4; // DIV 명령어 추가
        else if (opcode == "MOD") opcodeBinary = 0x5; // MOD 명령어 추가
        else if (opcode == "STA") opcodeBinary = 0x1;
        else if (opcode == "LDA") opcodeBinary = 0x0; // Load value from memory into AC

        uint16_t operandBinary = std::stoi(operand, nullptr, 16);

        uint16_t instruction = (opcodeBinary << 12) | operandBinary;
        memory.writeWord(address, instruction);
        address += 2; // Increment by 2 for 16-bit instruction
    }
    file.close();
}

int main() {
    Memory memory;
    CPU cpu(memory);

    // 프로그램 로드
    std::string filename = "prog.txt";
    loadProgram(memory, filename);

    // CPU 실행
    cpu.run();

    return 0;
}
