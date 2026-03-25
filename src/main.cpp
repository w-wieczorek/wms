#include <replxx.hxx>

#include <cctype>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace replxx;

namespace {

constexpr std::size_t kDataStackSize = 1'048'576;
constexpr std::size_t kReturnStackSize = 1'048'576;
constexpr std::size_t kDataMemorySize = 32'768;
constexpr std::size_t kProgramMemorySize = 32'768;

enum class Mode {
    Compile,
    Execute
};

enum class Opcode : int16_t {
    LIT,
    DUP,
    DROP,
    SWAP,
    OVER,
    POP,
    LOAD,
    STORE,
    MEM,
    ADD,
    SUB,
    MUL,
    DIV,
    AND,
    OR,
    XOR,
    NOT,
    EQ,
    LT,
    GT,
    LTE,
    GTE,
    JMP,
    JZ,
    JNZ,
    CALL,
    RET
};

std::string trim(const std::string& input) {
    std::size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    std::size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

std::string to_upper(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool parse_number(const std::string& token, int16_t& out_value) {
    try {
        std::size_t pos = 0;
        int value = std::stoi(token, &pos, 10);
        if (pos != token.size()) {
            return false;
        }
        out_value = static_cast<int16_t>(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool requires_inline_arg(Opcode opcode) {
    switch (opcode) {
        case Opcode::LIT:
        case Opcode::JMP:
        case Opcode::JZ:
        case Opcode::JNZ:
        case Opcode::CALL:
        case Opcode::MEM:
            return true;
        default:
            return false;
    }
}

std::string format_prompt(std::uint16_t pc, Mode mode) {
    std::ostringstream out;
    out << std::setw(5) << std::setfill('0') << pc << (mode == Mode::Compile ? ": " : "> ");
    return out.str();
}

} // namespace

int main() {
    Replxx rx;
    Mode mode = Mode::Execute;

    std::vector<int16_t> data_stack;
    data_stack.reserve(kDataStackSize);
    std::vector<int16_t> return_stack;
    return_stack.reserve(kReturnStackSize);
    std::vector<int16_t> data_memory(kDataMemorySize, 0);
    std::vector<int16_t> program_memory(kProgramMemorySize, 0);

    std::uint16_t program_counter = 0;
    std::optional<Opcode> pending_opcode;
    std::uint16_t pending_opcode_address = 0;

    auto opcode_map = std::unordered_map<std::string, Opcode>{
        {"LIT", Opcode::LIT},
        {"DUP", Opcode::DUP},
        {"DROP", Opcode::DROP},
        {"SWAP", Opcode::SWAP},
        {"OVER", Opcode::OVER},
        {"POP", Opcode::POP},
        {"LOAD", Opcode::LOAD},
        {"STORE", Opcode::STORE},
        {"MEM", Opcode::MEM},
        {"ADD", Opcode::ADD},
        {"SUB", Opcode::SUB},
        {"MUL", Opcode::MUL},
        {"DIV", Opcode::DIV},
        {"AND", Opcode::AND},
        {"OR", Opcode::OR},
        {"XOR", Opcode::XOR},
        {"NOT", Opcode::NOT},
        {"EQ", Opcode::EQ},
        {"LT", Opcode::LT},
        {"GT", Opcode::GT},
        {"LTE", Opcode::LTE},
        {"GTE", Opcode::GTE},
        {"JMP", Opcode::JMP},
        {"JZ", Opcode::JZ},
        {"JNZ", Opcode::JNZ},
        {"CALL", Opcode::CALL},
        {"RET", Opcode::RET}
    };

    auto pop_stack = [&](std::vector<int16_t>& stack, const char* name) -> std::optional<int16_t> {
        if (stack.empty()) {
            std::cout << "Stack underflow on " << name << ".\n";
            return std::nullopt;
        }
        int16_t value = stack.back();
        stack.pop_back();
        return value;
    };

    auto push_stack = [&](std::vector<int16_t>& stack, int16_t value, const char* name) -> bool {
        if (stack.size() >= kDataStackSize) {
            std::cout << "Stack overflow on " << name << ".\n";
            return false;
        }
        stack.push_back(value);
        return true;
    };

    auto execute = [&](std::uint16_t start_pc, std::uint16_t stop_pc) {
        std::uint16_t exec_pc = start_pc;
        while (exec_pc < program_memory.size() && exec_pc != stop_pc) {
            Opcode opcode = static_cast<Opcode>(program_memory[exec_pc]);
            auto fetch_inline = [&]() -> std::optional<int16_t> {
                if (exec_pc + 1 >= program_memory.size()) {
                    std::cout << "Program memory out of bounds.\n";
                    return std::nullopt;
                }
                return program_memory[exec_pc + 1];
            };

            switch (opcode) {
                case Opcode::LIT: {
                    auto arg = fetch_inline();
                    if (!arg.has_value()) {
                        return;
                    }
                    if (!push_stack(data_stack, arg.value(), "data")) {
                        return;
                    }
                    exec_pc = static_cast<std::uint16_t>(exec_pc + 2);
                    break;
                }
                case Opcode::DUP: {
                    if (data_stack.empty()) {
                        std::cout << "Stack underflow on DUP.\n";
                        return;
                    }
                    if (!push_stack(data_stack, data_stack.back(), "data")) {
                        return;
                    }
                    exec_pc = static_cast<std::uint16_t>(exec_pc + 1);
                    break;
                }
                case Opcode::DROP: {
                    if (!pop_stack(data_stack, "DROP").has_value()) {
                        return;
                    }
                    exec_pc = static_cast<std::uint16_t>(exec_pc + 1);
                    break;
                }
                case Opcode::SWAP: {
                    if (data_stack.size() < 2) {
                        std::cout << "Stack underflow on SWAP.\n";
                        return;
                    }
                    std::swap(data_stack[data_stack.size() - 1], data_stack[data_stack.size() - 2]);
                    exec_pc = static_cast<std::uint16_t>(exec_pc + 1);
                    break;
                }
                case Opcode::OVER: {
                    if (data_stack.size() < 2) {
                        std::cout << "Stack underflow on OVER.\n";
                        return;
                    }
                    if (!push_stack(data_stack, data_stack[data_stack.size() - 2], "data")) {
                        return;
                    }
                    exec_pc = static_cast<std::uint16_t>(exec_pc + 1);
                    break;
                }
                case Opcode::POP: {
                    auto value = pop_stack(data_stack, "POP");
                    if (!value.has_value()) {
                        return;
                    }
                    std::cout << value.value() << "\n";
                    exec_pc = static_cast<std::uint16_t>(exec_pc + 1);
                    break;
                }
                case Opcode::LOAD: {
                    auto addr_value = pop_stack(data_stack, "LOAD");
                    if (!addr_value.has_value()) {
                        return;
                    }
                    std::uint16_t addr = static_cast<std::uint16_t>(addr_value.value());
                    if (addr >= data_memory.size()) {
                        std::cout << "Data memory out of bounds.\n";
                        return;
                    }
                    if (!push_stack(data_stack, data_memory[addr], "data")) {
                        return;
                    }
                    exec_pc = static_cast<std::uint16_t>(exec_pc + 1);
                    break;
                }
                case Opcode::STORE: {
                    auto addr_value = pop_stack(data_stack, "STORE");
                    auto val_value = pop_stack(data_stack, "STORE");
                    if (!addr_value.has_value() || !val_value.has_value()) {
                        return;
                    }
                    std::uint16_t addr = static_cast<std::uint16_t>(addr_value.value());
                    if (addr >= data_memory.size()) {
                        std::cout << "Data memory out of bounds.\n";
                        return;
                    }
                    data_memory[addr] = val_value.value();
                    exec_pc = static_cast<std::uint16_t>(exec_pc + 1);
                    break;
                }
                case Opcode::MEM: {
                    auto arg = fetch_inline();
                    if (!arg.has_value()) {
                        return;
                    }
                    int size = arg.value();
                    if (size < 0) {
                        size = 0;
                    }
                    std::size_t limit = std::min<std::size_t>(static_cast<std::size_t>(size), data_memory.size());
                    for (std::size_t i = 0; i < limit; ++i) {
                        std::cout << std::setw(5) << std::setfill('0') << i << ": " << data_memory[i] << "\n";
                    }
                    exec_pc = static_cast<std::uint16_t>(exec_pc + 2);
                    break;
                }
                case Opcode::ADD:
                case Opcode::SUB:
                case Opcode::MUL:
                case Opcode::DIV:
                case Opcode::AND:
                case Opcode::OR:
                case Opcode::XOR:
                case Opcode::EQ:
                case Opcode::LT:
                case Opcode::GT:
                case Opcode::LTE:
                case Opcode::GTE: {
                    auto rhs_value = pop_stack(data_stack, "op");
                    auto lhs_value = pop_stack(data_stack, "op");
                    if (!rhs_value.has_value() || !lhs_value.has_value()) {
                        return;
                    }
                    int16_t lhs = lhs_value.value();
                    int16_t rhs = rhs_value.value();
                    int16_t result = 0;
                    switch (opcode) {
                        case Opcode::ADD:
                            result = static_cast<int16_t>(lhs + rhs);
                            break;
                        case Opcode::SUB:
                            result = static_cast<int16_t>(lhs - rhs);
                            break;
                        case Opcode::MUL:
                            result = static_cast<int16_t>(lhs * rhs);
                            break;
                        case Opcode::DIV:
                            if (rhs == 0) {
                                std::cout << "Division by zero.\n";
                                return;
                            }
                            result = static_cast<int16_t>(lhs / rhs);
                            break;
                        case Opcode::AND:
                            result = static_cast<int16_t>(lhs & rhs);
                            break;
                        case Opcode::OR:
                            result = static_cast<int16_t>(lhs | rhs);
                            break;
                        case Opcode::XOR:
                            result = static_cast<int16_t>(lhs ^ rhs);
                            break;
                        case Opcode::EQ:
                            result = static_cast<int16_t>(lhs == rhs ? 1 : 0);
                            break;
                        case Opcode::LT:
                            result = static_cast<int16_t>(lhs < rhs ? 1 : 0);
                            break;
                        case Opcode::GT:
                            result = static_cast<int16_t>(lhs > rhs ? 1 : 0);
                            break;
                        case Opcode::LTE:
                            result = static_cast<int16_t>(lhs <= rhs ? 1 : 0);
                            break;
                        case Opcode::GTE:
                            result = static_cast<int16_t>(lhs >= rhs ? 1 : 0);
                            break;
                        default:
                            break;
                    }
                    if (!push_stack(data_stack, result, "data")) {
                        return;
                    }
                    exec_pc = static_cast<std::uint16_t>(exec_pc + 1);
                    break;
                }
                case Opcode::NOT: {
                    auto value = pop_stack(data_stack, "NOT");
                    if (!value.has_value()) {
                        return;
                    }
                    if (!push_stack(data_stack, static_cast<int16_t>(~value.value()), "data")) {
                        return;
                    }
                    exec_pc = static_cast<std::uint16_t>(exec_pc + 1);
                    break;
                }
                case Opcode::JMP: {
                    auto arg = fetch_inline();
                    if (!arg.has_value()) {
                        return;
                    }
                    exec_pc = static_cast<std::uint16_t>(arg.value());
                    break;
                }
                case Opcode::JZ: {
                    auto arg = fetch_inline();
                    if (!arg.has_value()) {
                        return;
                    }
                    auto flag_value = pop_stack(data_stack, "JZ");
                    if (!flag_value.has_value()) {
                        return;
                    }
                    if (flag_value.value() == 0) {
                        exec_pc = static_cast<std::uint16_t>(arg.value());
                    } else {
                        exec_pc = static_cast<std::uint16_t>(exec_pc + 2);
                    }
                    break;
                }
                case Opcode::JNZ: {
                    auto arg = fetch_inline();
                    if (!arg.has_value()) {
                        return;
                    }
                    auto flag_value = pop_stack(data_stack, "JNZ");
                    if (!flag_value.has_value()) {
                        return;
                    }
                    if (flag_value.value() != 0) {
                        exec_pc = static_cast<std::uint16_t>(arg.value());
                    } else {
                        exec_pc = static_cast<std::uint16_t>(exec_pc + 2);
                    }
                    break;
                }
                case Opcode::CALL: {
                    auto arg = fetch_inline();
                    if (!arg.has_value()) {
                        return;
                    }
                    if (return_stack.size() >= kReturnStackSize) {
                        std::cout << "Return stack overflow.\n";
                        return;
                    }
                    return_stack.push_back(static_cast<int16_t>(exec_pc + 2));
                    exec_pc = static_cast<std::uint16_t>(arg.value());
                    break;
                }
                case Opcode::RET: {
                    if (return_stack.empty()) {
                        std::cout << "Return stack underflow.\n";
                        return;
                    }
                    int16_t addr = return_stack.back();
                    return_stack.pop_back();
                    exec_pc = static_cast<std::uint16_t>(addr);
                    break;
                }
                default:
                    std::cout << "Unknown opcode " << static_cast<int>(program_memory[exec_pc]) << ".\n";
                    return;
            }
        }
    };

    auto colon_handler = [&](char32_t) -> Replxx::ACTION_RESULT {
        if (pending_opcode.has_value()) {
            return Replxx::ACTION_RESULT::CONTINUE;
        }
        mode = Mode::Compile;
        std::string prompt = format_prompt(program_counter, mode);
        rx.set_prompt(prompt);
        return Replxx::ACTION_RESULT::CONTINUE;
    };

    auto gt_handler = [&](char32_t) -> Replxx::ACTION_RESULT {
        if (pending_opcode.has_value()) {
            return Replxx::ACTION_RESULT::CONTINUE;
        }
        mode = Mode::Execute;
        std::string prompt = format_prompt(program_counter, mode);
        rx.set_prompt(prompt);
        return Replxx::ACTION_RESULT::CONTINUE;
    };

    rx.bind_key(':', colon_handler);
    rx.bind_key('>', gt_handler);

    while (true) {
        std::string prompt = format_prompt(program_counter, mode);
        const char* line = rx.input(prompt.c_str());
        if (!line) {
            break;
        }
        std::string input = trim(line);
        if (input.empty()) {
            continue;
        }

        if (input == ":") {
            if (!pending_opcode.has_value()) {
                mode = Mode::Compile;
            }
            continue;
        }
        if (input == ">") {
            if (!pending_opcode.has_value()) {
                mode = Mode::Execute;
            }
            continue;
        }

        if (pending_opcode.has_value()) {
            int16_t arg = 0;
            if (!parse_number(input, arg)) {
                std::cout << "Expected numeric argument.\n";
                continue;
            }
            if (program_counter >= program_memory.size()) {
                std::cout << "Program memory full.\n";
                pending_opcode.reset();
                continue;
            }
            program_memory[program_counter] = arg;
            ++program_counter;

            if (mode == Mode::Execute) {
                execute(pending_opcode_address, program_counter);
            }
            pending_opcode.reset();
            continue;
        }

        std::string token = to_upper(input);
        auto opcode_it = opcode_map.find(token);
        if (opcode_it == opcode_map.end()) {
            std::cout << "Unknown token: " << input << "\n";
            continue;
        }

        if (program_counter >= program_memory.size()) {
            std::cout << "Program memory full.\n";
            continue;
        }

        Opcode opcode = opcode_it->second;
        program_memory[program_counter] = static_cast<int16_t>(opcode);
        std::uint16_t opcode_address = program_counter;
        ++program_counter;

        if (requires_inline_arg(opcode)) {
            pending_opcode = opcode;
            pending_opcode_address = opcode_address;
            continue;
        }

        if (mode == Mode::Execute) {
            execute(opcode_address, program_counter);
        }
    }

    return 0;
}
