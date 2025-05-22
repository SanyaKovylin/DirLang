#ifndef ELFGEN_H_INCLUDED
#define ELFGEN_H_INCLUDED

typedef struct {
    uint8_t *code;
    size_t size;
    size_t capacity;
} CodeBuffer;

typedef struct {
    size_t code_offset;  // Where in the code the fixup is needed
    char *target;   // Label or function name we're jumping to
    size_t instr_size;    // Size of the instruction needing fixup (4 for rel32)
} Fixup;

typedef struct {
    char *name;
    size_t address;
} Label;

typedef struct {
    Fixup* fixups;
    size_t count;
    size_t capacity;
} Fixups;

typedef struct {
    Label* labels;
    size_t count;
    size_t capacity;
} Labels;

typedef enum ElfError {
    OK = 0,
    ALLOCATION_ERROR,
    STRUCT_EMPTY,
    EMPTY_NAME
} ElfError;

typedef struct {
    char *name;
    uint64_t address;
} FunctionEntry;

const size_t init_size = 4;

#define COMMAND(byte_seq, string_representation, ...)\
    fprintf(nasm_file, string_representation "\n" __VA_OPT__(,)  __VA_ARGS__);\
    code_buffer_append(code_buf, byte_seq, sizeof(byte_seq) - 1);

#define SEC_COMMAND

// code buffer
void code_buffer_init(CodeBuffer *buf);
void code_buffer_append(CodeBuffer *buf, const void *data, size_t len);
void code_buffer_append_byte(CodeBuffer *buf, uint8_t byte);
void code_buffer_append_u32(CodeBuffer *buf, uint32_t value);
void code_buffer_append_u64(CodeBuffer *buf, uint64_t value);

// Headers
void write_elf_header(FILE *out, uint64_t entry_point, uint64_t phdr_offset);
void write_program_header(FILE *out, uint64_t offset, uint64_t vaddr, uint64_t filesz, uint64_t memsize);

// Labels
Labels* init_labels();
ElfError free_labels(Labels* labels);
ElfError add_label(Labels* labels, char* name, size_t address, int number);

//Fixups
Fixups* init_fixups();
ElfError free_fixups(Fixups* fixups);
ElfError add_fixup(Fixups* fixups, char* name, size_t address, int number, size_t instr_size);

// Parsing IR
void FirstPass(IRFuncs* ir, CodeBuffer* code_buf, uint64_t* entry_point, int shift, Labels* labels, Fixups* fixups);
void SecondPass(IRFuncs* ir, CodeBuffer* code_buf, int shift, Labels* labels, Fixups* fixups);
void generate_machine_code(IRFuncs* ir, CodeBuffer *code_buf, uint64_t *entry_point, int shift);

void handle_ir_assign(IRFunction* func, CodeBuffer* code_buf, IRNode* node);
void handle_ir_binop(CodeBuffer* code_buf, IRNode* node);
void handle_ir_call(Fixups* fixups, CodeBuffer* code_buf, IRNode* node);
void handle_ir_jump(Fixups* fixups, CodeBuffer* code_buf, IRNode* node);
void handle_ir_cjump(Fixups* fixups, CodeBuffer* code_buf, IRNode* node);
void handle_ir_return(IRFunction* func, CodeBuffer* code_buf);
void handle_ir_var(IRFunction* func, CodeBuffer* code_buf, IRNode* node);
void handle_ir_const(CodeBuffer* code_buf, IRNode* node);
void handle_ir_res_push(CodeBuffer* code_buf);
void handle_ir_pop_reg(CodeBuffer* code_buf, IRNode* node);
void handle_ir_mov(CodeBuffer* code_buf, IRNode* node);

uint8_t calculate_arg_shift(IRFunction* func, int position);
#endif
