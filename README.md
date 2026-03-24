# WMS – Stack Virtual Machine

WMS (in Polish wirtualna maszyna stosowa) is a minimal, 
stack-based virtual machine (MISC architecture) implemented as an
interactive command interpreter. The machine operates on four independent
16-bit memory segments and supports two execution modes distinguished by the
prompt character.

## Architecture

### Memory Segments

| Segment        | Size         | Cell width | Description                                      |
|----------------|--------------|------------|--------------------------------------------------|
| Data Stack     | 1 048 576    | 16 bits    | Operand stack used for all arithmetic and logic  |
| Return Stack   | 1 048 576    | 16 bits    | Stores return addresses during CALL/RET          |
| Data Memory    | 65 536 cells | 16 bits    | Random-access heap, addressed by LOAD/STORE      |
| Program Memory | 65 536 cells | 16 bits    | Stores interpreted opcodes and their inline values  |

All memory cells are 16-bit (`short`), giving a natural word size of 16 bits.

### Execution Modes

The machine is always in one of two modes, indicated by the prompt character:

| Prompt | Mode    | Behaviour                                                             |
|--------|---------|-----------------------------------------------------------------------|
| `:`    | Compile | Each entered token is encoded and written to Program Memory.         |
|        |         | No instruction is executed yet.                                      |
| `>`    | Execute | Each entered token is encoded, written to Program Memory, and        |
|        |         | immediately executed.                                                |

Switching between modes can be triggered by specific key bindings in the REPL.

### Instruction Encoding

Each instruction occupies exactly one 16-bit cell in Program Memory.  
Instructions that carry an inline argument (e.g. `LIT`, `JMP`) occupy **two
consecutive cells**: the first cell holds the opcode, the second holds the
argument value. The interpreter prompts for the argument on the next input
line.

Example – entering `LIT 512` in compile mode writes two cells:

```
00000: LIT
00001: 512
```


## Instruction Set Reference

### Stack Manipulation

| Instruction | Stack Effect       | Description                                          |
|-------------|--------------------|------------------------------------------------------|
| `LIT <val>` | `( -- val )`       | Push a 16-bit literal value onto the data stack.     |
| `DUP`       | `( a -- a a )`     | Duplicate the top of the data stack.                 |
| `DROP`      | `( a -- )`         | Discard the top of the data stack.                   |
| `SWAP`      | `( a b -- b a )`   | Swap the two topmost values.                         |
| `OVER`      | `( a b -- a b a )` | Copy the second value to the top.                    |
| `POP`       | `( a -- )`         | Pop the top value and print it to the console.       |

### Memory

| Instruction    | Stack Effect      | Description                                               |
|----------------|-------------------|-----------------------------------------------------------|
| `LOAD`         | `( addr -- val )` | Read a 16-bit value from Data Memory at `addr`.           |
| `STORE`        | `( val addr -- )` | Write `val` to Data Memory at `addr`.                     |
| `MEM <size>`   | `( -- )`          | Dump the first `size` cells of Data Memory to the console.|

### Arithmetic

| Instruction | Stack Effect           | Description              |
|-------------|------------------------|--------------------------|
| `ADD`       | `( a b -- a+b )`       | Integer addition.        |
| `SUB`       | `( a b -- a-b )`       | Integer subtraction.     |
| `MUL`       | `( a b -- a*b )`       | Integer multiplication.  |
| `DIV`       | `( a b -- a/b )`       | Integer division.        |

### Bitwise / Logic

| Instruction | Stack Effect       | Description              |
|-------------|--------------------|--------------------------|
| `AND`       | `( a b -- a&b )`   | Bitwise AND.             |
| `OR`        | `( a b -- a\|b )`  | Bitwise OR.              |
| `XOR`       | `( a b -- a^b )`   | Bitwise XOR.             |
| `NOT`       | `( a -- ~a )`      | Bitwise NOT (complement).|

### Comparison

All comparison instructions pop two values and push `1` (true) or `0` (false).

| Instruction | Condition       |
|-------------|-----------------|
| `EQ`        | `a == b`        |
| `LT`        | `a < b`         |
| `GT`        | `a > b`         |
| `LTE`       | `a <= b`        |
| `GTE`       | `a >= b`        |

### Control Flow

| Instruction   | Stack Effect | Description                                                       |
|---------------|--------------|-------------------------------------------------------------------|
| `JMP <addr>`  | `( -- )`     | Unconditional jump to `addr` in Program Memory.                  |
| `JZ <addr>`   | `( flag -- )`| Jump to `addr` if top of stack equals zero.                      |
| `JNZ <addr>`  | `( flag -- )`| Jump to `addr` if top of stack is non-zero.                      |
| `CALL <addr>` | `( -- )`     | Push current PC to Return Stack, then jump to `addr`.            |
| `RET`         | `( -- )`     | Pop Return Stack and resume execution from that address.         |

## Usage Example

Below is a session that compiles a small subroutine (`double`: multiplies
top-of-stack by 2) and then calls it in execute mode.

```
00002: LIT ; push opcode of LIT
00003: 2 ; push literal value 2
00004: MUL ; multiply
00005: RET ; end of subroutine — subroutine "double" is now at address 2
00006> LIT ; push opcode of LIT (executed immediately)
00007> 21 ; push literal 21 → data stack
00008> CALL ; push opcode of CALL
00009> 2 ; call subroutine at address 2
00010> POP ; print and discard top → prints: 42
```


## Building

```bash
cmake -S . -B build
cmake --build build
```

Requirements: C++17 or later, [replxx](https://github.com/AmokHuginnsson/replxx)
(available via `vcpkg install replxx`).

## License

MIT