#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <elf.h>
#include <stdint.h>
#include "IR.h"

// Buffer operations ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
typedef struct {
    uint8_t *code;
    size_t size;
    size_t capacity;
} CodeBuffer;

static void code_buffer_init(CodeBuffer *buf) {
    buf->capacity = 1024;
    buf->size = 0;
    buf->code = (uint8_t*) calloc(buf->capacity/sizeof(uint8_t), sizeof(uint8_t));
}

static void code_buffer_append(CodeBuffer *buf, const void *data, size_t len) {
    while (buf->size + len > buf->capacity) {
        buf->capacity *= 2;
        buf->code = (uint8_t *)realloc(buf->code, buf->capacity);
    }
    memcpy(buf->code + buf->size, data, len);
    buf->size += len;
}

static void code_buffer_append_byte(CodeBuffer *buf, uint8_t byte) {
    code_buffer_append(buf, &byte, 1);
}


static void code_buffer_append_u32(CodeBuffer *buf, uint32_t value) {
    code_buffer_append(buf, &value, 4);
}


static void code_buffer_append_u64(CodeBuffer *buf, uint64_t value) {
    code_buffer_append(buf, &value, 8);
}

//==============================================================================================

static void write_elf_header(FILE *out, uint64_t entry_point, uint64_t phdr_offset) {
    Elf64_Ehdr hdr;
    memset(&hdr, 0, sizeof(hdr));

    hdr.e_ident[EI_MAG0] = ELFMAG0;
    hdr.e_ident[EI_MAG1] = ELFMAG1;
    hdr.e_ident[EI_MAG2] = ELFMAG2;
    hdr.e_ident[EI_MAG3] = ELFMAG3;
    hdr.e_ident[EI_CLASS] = ELFCLASS64;
    hdr.e_ident[EI_DATA] = ELFDATA2LSB;
    hdr.e_ident[EI_VERSION] = EV_CURRENT;
    hdr.e_ident[EI_OSABI] = ELFOSABI_SYSV;

    hdr.e_type = ET_EXEC;
    hdr.e_machine = EM_X86_64;
    hdr.e_version = EV_CURRENT;
    hdr.e_entry = entry_point;
    hdr.e_phoff = phdr_offset;
    hdr.e_shoff = 0;
    hdr.e_flags = 0;
    hdr.e_ehsize = sizeof(Elf64_Ehdr);
    hdr.e_phentsize = sizeof(Elf64_Phdr);
    hdr.e_phnum = 1;
    hdr.e_shentsize = sizeof(Elf64_Shdr);
    hdr.e_shnum = 0;
    hdr.e_shstrndx = 0;

    fwrite(&hdr, 1, sizeof(hdr), out);
}

static void write_program_header(FILE *out, uint64_t offset, uint64_t vaddr,
                               uint64_t filesz, uint64_t memsz) {
    Elf64_Phdr phdr;
    memset(&phdr, 0, sizeof(phdr));

    phdr.p_type = PT_LOAD;
    phdr.p_offset = offset;
    phdr.p_vaddr = vaddr;
    phdr.p_paddr = vaddr;
    phdr.p_filesz = filesz;
    phdr.p_memsz = memsz;
    phdr.p_flags = PF_X | PF_R;
    phdr.p_align = 0x1000;

    fwrite(&phdr, 1, sizeof(phdr), out);
}

//===================================================================================================================

typedef struct {
    size_t code_offset;  // Where in the code the fixup is needed
    char *target;   // Label or function name we're jumping to
    size_t instr_size;    // Size of the instruction needing fixup (4 for rel32)
} Fixup;

uint8_t calculate_arg_shift(IRFunction* func, int position){
    uint8_t ans = 0;
    if (position < func->nargs) ans = (uint8_t) ((func->nargs - position + 1)* 8);
    else ans = (uint8_t) ((0 - position) * 8);
    return ans;
}

static void generate_machine_code(IRFuncs* ir, CodeBuffer *code_buf,
                                uint64_t *entry_point) {

    typedef struct {
        char *name;
        uint64_t address;
    } FunctionEntry;

    typedef struct {
        char *name;
        size_t address;
    } Label;

    Label *labels = NULL;
    size_t label_count = 0;
    Fixup *fixups = NULL;
    size_t fixup_count = 0;

    FunctionEntry *func_entries = (FunctionEntry *) calloc(ir->nfuncs , sizeof(FunctionEntry));
    size_t func_count = 0;


    for (int i = 0; i < ir->nfuncs; i++) {
        IRFunction *func = &ir->funcs[i];
        IRList *current = func->list;

        labels = (Label*) realloc (labels, (label_count + 1) * sizeof(Label));
        labels[label_count].name = func->name;
        puts(labels[0].name);
        labels[label_count].address = code_buf->size;
        label_count++;

        func_entries[func_count].name = func->name;
        func_entries[func_count].address = code_buf->size;
        func_count++;


        code_buffer_append_byte(code_buf, 0x55);       // push rbp

        code_buffer_append_byte(code_buf, 0x48);       // mov rbp, rsp
        code_buffer_append_byte(code_buf, 0x89);
        code_buffer_append_byte(code_buf, 0xE5);

        if (func->nlocals > 0) {
            code_buffer_append_byte(code_buf, 0x48);   // sub rsp, N
            code_buffer_append_byte(code_buf, 0x83);
            code_buffer_append_byte(code_buf, 0xEC);
            code_buffer_append_byte(code_buf, (uint8_t) func->nlocals * 8);
        }

        while (current != NULL) {
            IRNode *node = current->node;
            if (!node) {
                current = current->next;
                continue;
            }

            switch(node->type) {

                case IR_LABEL:
                    labels = (Label*) realloc (labels, (label_count + 1) * sizeof(Label));
                    char label_name[32];
                    snprintf(label_name, sizeof(label_name), "L%d", node->label.label_num);
                    labels[label_count].name = strdup(label_name);
                    labels[label_count].address = code_buf->size;
                    label_count++;
                    break;

                case IR_ASSIGN: //TODO
                    // mov [rbp - offset], rax
                    code_buffer_append_byte(code_buf, 0x48);
                    code_buffer_append_byte(code_buf, 0x89);
                    code_buffer_append_byte(code_buf, 0x45);
                    code_buffer_append_byte(code_buf, calculate_arg_shift(func, node->var_const.var_pos));
                    break;

                case IR_BINOP:
                    switch(node->binop.op) {
                        case OP_ADD:
                            // add rax, rbx
                            code_buffer_append_byte(code_buf, 0x48);
                            code_buffer_append_byte(code_buf, 0x01);
                            code_buffer_append_byte(code_buf, 0xD8);
                            break;
                        case OP_SUB:
                            // sub rax, rbx
                            code_buffer_append_byte(code_buf, 0x48);
                            code_buffer_append_byte(code_buf, 0x29);
                            code_buffer_append_byte(code_buf, 0xD8);
                            break;
                        case OP_MUL:
                            // imul rax, rbx
                            code_buffer_append_byte(code_buf, 0x48);
                            code_buffer_append_byte(code_buf, 0x0F);
                            code_buffer_append_byte(code_buf, 0xAF);
                            code_buffer_append_byte(code_buf, 0xC3);
                            break;
                        case OP_DIV:
                            // cqo
                            code_buffer_append_byte(code_buf, 0x48);
                            code_buffer_append_byte(code_buf, 0x99);
                            // idiv rbx
                            code_buffer_append_byte(code_buf, 0x48);
                            code_buffer_append_byte(code_buf, 0xF7);
                            code_buffer_append_byte(code_buf, 0xFB);
                            break;

                        case OP_SIN:
                        case OP_COS:
                        case OP_ARCSIN:
                        case OP_ARCCOS:
                        case OP_LOG:
                        case OP_LN:
                        case OP_E:
                        case OP_PI:
                        case OP_SQRT:
                        default: break;
                    }

                    break;

                case IR_CALL: {

                    code_buffer_append_byte(code_buf, 0xE8);
                    size_t call_pos = code_buf->size;
                    code_buffer_append_u32(code_buf, 0); // Placeholder

                    // Record fixup
                    fixups = (Fixup*) realloc(fixups, (fixup_count + 1) * sizeof(Fixup));
                    fixups[fixup_count].code_offset = call_pos;
                    fixups[fixup_count].target = node->call.func_name;
                    fixups[fixup_count].instr_size = 4;
                    fixup_count++;

                    if (node->call.nargs > 0) {
                        code_buffer_append_byte(code_buf, 0x48);   // add rsp, nargs*8
                        code_buffer_append_byte(code_buf, 0x83);
                        code_buffer_append_byte(code_buf, 0xC4);
                        code_buffer_append_byte(code_buf, node->call.nargs * 8);
                    }

                    break;
                }

                case IR_JUMP: {

                    // jmp rel32 (placeholder)
                    code_buffer_append_byte(code_buf, 0xE9);
                    size_t jump_pos = code_buf->size;
                    code_buffer_append_u32(code_buf, 0); // Placeholder

                    // Record fixup
                    fixups = (Fixup*) realloc(fixups, (fixup_count + 1) * sizeof(Fixup));
                    char target_label[32];
                    snprintf(target_label, sizeof(target_label), "L%d", node->jump.target_label);
                    fixups[fixup_count].code_offset = jump_pos;
                    fixups[fixup_count].target = strdup(target_label);
                    fixups[fixup_count].instr_size = 4;
                    fixup_count++;

                    break;}

                case IR_CJUMP: {

                    // test rax, rax
                    code_buffer_append_byte(code_buf, 0x48);
                    code_buffer_append_byte(code_buf, 0x85);
                    code_buffer_append_byte(code_buf, 0xC0);

                    code_buffer_append_byte(code_buf, 0x0F);
                    code_buffer_append_byte(code_buf, 0x84);
                    size_t cjump_pos = code_buf->size;
                    code_buffer_append_u32(code_buf, 0); // Placeholder

                    // Record fixup
                    fixups = (Fixup*) realloc (fixups, (fixup_count + 1) * sizeof(Fixup));
                    char false_label[32];
                    snprintf(false_label, sizeof(false_label), "L%d", node->cjump.false_label);
                    fixups[fixup_count].code_offset = cjump_pos;
                    fixups[fixup_count].target = strdup(false_label);
                    fixups[fixup_count].instr_size = 4;
                    fixup_count++;}

                    break;

                case IR_RETURN:

                    if (func->nlocals > 0) {
                        // Restore stack pointer
                        code_buffer_append_byte(code_buf, 0x48);       // lea rsp, [rbp]
                        code_buffer_append_byte(code_buf, 0x8D);
                        code_buffer_append_byte(code_buf, 0x65);
                        code_buffer_append_byte(code_buf, 0x00);
                    }
                    // mov rsp, rbp
                    code_buffer_append_byte(code_buf, 0x48);
                    code_buffer_append_byte(code_buf, 0x89);
                    code_buffer_append_byte(code_buf, 0xEC);
                    // pop rbp
                    code_buffer_append_byte(code_buf, 0x5D);

                    if (strcmp(func->name, "main") == 0) {
                        // Linux x86-64 _start expects main to use exit syscall
                        code_buffer_append_byte(code_buf, 0x48);     // mov rdi, rax (exit code)
                        code_buffer_append_byte(code_buf, 0x89);
                        code_buffer_append_byte(code_buf, 0xC7);

                        code_buffer_append_byte(code_buf, 0x48);     // mov rax, 60 (sys_exit)
                        code_buffer_append_byte(code_buf, 0xC7);
                        code_buffer_append_byte(code_buf, 0xC0);
                        code_buffer_append_byte(code_buf, 0x3C);
                        code_buffer_append_byte(code_buf, 0x00);
                        code_buffer_append_byte(code_buf, 0x00);
                        code_buffer_append_byte(code_buf, 0x00);

                        code_buffer_append_byte(code_buf, 0x0F);     // syscall
                        code_buffer_append_byte(code_buf, 0x05);
                    }
                    else {
                        // Normal function return
                        code_buffer_append_byte(code_buf, 0xC3);     // ret
                    }
                    break;

                case IR_VAR:
                    // mov rax, [rbp - offset]
                    code_buffer_append_byte(code_buf, 0x48);
                    code_buffer_append_byte(code_buf, 0x8B);
                    code_buffer_append_byte(code_buf, 0x45);
                    code_buffer_append_byte(code_buf, calculate_arg_shift(func, node->var_const.var_pos));
                    // // push rax
                    // code_buffer_append_byte(code_buf, 0x50);
                    break;

                case IR_CONST: {
                    // mov rax, imm64
                    code_buffer_append_byte(code_buf, 0x48);
                    code_buffer_append_byte(code_buf, 0xB8);
                    uint64_t value = (uint64_t)node->var_const.num_value;
                    code_buffer_append_u64(code_buf, value);
                    // // push rax
                    // code_buffer_append_byte(code_buf, 0x50);
                    break;}

                case IR_INPUT:
                    // System call for reading input (Linux x86_64)
                    // sys_read(fd=0, buf=stack, count=8)
                    code_buffer_append_byte(code_buf, 0x48);  // sub rsp, 8
                    code_buffer_append_byte(code_buf, 0x83);
                    code_buffer_append_byte(code_buf, 0xEC);
                    code_buffer_append_byte(code_buf, 0x08);

                    code_buffer_append_byte(code_buf, 0x48);  // mov rax, 0 (sys_read)
                    code_buffer_append_byte(code_buf, 0xC7);
                    code_buffer_append_byte(code_buf, 0xC0);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);

                    code_buffer_append_byte(code_buf, 0x48);  // mov rdi, 0 (stdin)
                    code_buffer_append_byte(code_buf, 0xC7);
                    code_buffer_append_byte(code_buf, 0xC7);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);

                    code_buffer_append_byte(code_buf, 0x48);  // mov rsi, rsp
                    code_buffer_append_byte(code_buf, 0x89);
                    code_buffer_append_byte(code_buf, 0xE6);

                    code_buffer_append_byte(code_buf, 0x48);  // mov rdx, 8
                    code_buffer_append_byte(code_buf, 0xC7);
                    code_buffer_append_byte(code_buf, 0xC2);
                    code_buffer_append_byte(code_buf, 0x08);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);

                    code_buffer_append_byte(code_buf, 0x0F);  // syscall
                    code_buffer_append_byte(code_buf, 0x05);

                    // Move input to destination
                    code_buffer_append_byte(code_buf, 0x48);  // mov rax, [rsp]
                    code_buffer_append_byte(code_buf, 0x8B);
                    code_buffer_append_byte(code_buf, 0x04);
                    code_buffer_append_byte(code_buf, 0x24);

                    code_buffer_append_byte(code_buf, 0x48);  // add rsp, 8
                    code_buffer_append_byte(code_buf, 0x83);
                    code_buffer_append_byte(code_buf, 0xC4);
                    code_buffer_append_byte(code_buf, 0x08);
                    break;

                case IR_PRINT:{

                    // Preserve registers we'll modify
                    code_buffer_append_byte(code_buf, 0x50);   // push rax
                    code_buffer_append_byte(code_buf, 0x51);   // push rcx
                    code_buffer_append_byte(code_buf, 0x52);   // push rdx
                    code_buffer_append_byte(code_buf, 0x56);   // push rsi
                    code_buffer_append_byte(code_buf, 0x57);   // push rdi

                    code_buffer_append_byte(code_buf, 0x48);  // sub rsp, 8
                    code_buffer_append_byte(code_buf, 0x83);
                    code_buffer_append_byte(code_buf, 0xEC);
                    code_buffer_append_byte(code_buf, 0x08);

                    code_buffer_append_byte(code_buf, 0x48);   // mov [rsp], rax
                    code_buffer_append_byte(code_buf, 0x89);
                    code_buffer_append_byte(code_buf, 0x04);
                    code_buffer_append_byte(code_buf, 0x24);


                    code_buffer_append_byte(code_buf, 0x48);   // mov rsi, reg
                    code_buffer_append_byte(code_buf, 0x89);
                    uint8_t modrm = 0xC0 | (REG_RSP << 3) | REG_RSI;
                    code_buffer_append_byte(code_buf, modrm);

                    // Prepare syscall arguments
                    code_buffer_append_byte(code_buf, 0x48);   // mov rax, 1 (sys_write)
                    code_buffer_append_byte(code_buf, 0xC7);
                    code_buffer_append_byte(code_buf, 0xC0);
                    code_buffer_append_byte(code_buf, 0x01);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);

                    code_buffer_append_byte(code_buf, 0x48);   // mov rdi, 1 (stdout)
                    code_buffer_append_byte(code_buf, 0xC7);
                    code_buffer_append_byte(code_buf, 0xC7);
                    code_buffer_append_byte(code_buf, 0x01);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);

                    code_buffer_append_byte(code_buf, 0x48);   // mov rdx, 8 (64-bit value)
                    code_buffer_append_byte(code_buf, 0xC7);
                    code_buffer_append_byte(code_buf, 0xC2);
                    code_buffer_append_byte(code_buf, 0x08);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);
                    code_buffer_append_byte(code_buf, 0x00);

                    // Make syscall
                    code_buffer_append_byte(code_buf, 0x0F);   // syscall
                    code_buffer_append_byte(code_buf, 0x05);

                    code_buffer_append_byte(code_buf, 0x48);  // add rsp, 8
                    code_buffer_append_byte(code_buf, 0x83);
                    code_buffer_append_byte(code_buf, 0xC4);
                    code_buffer_append_byte(code_buf, 0x08);

                    // Restore registers
                    code_buffer_append_byte(code_buf, 0x5F);   // pop rdi
                    code_buffer_append_byte(code_buf, 0x5E);   // pop rsi
                    code_buffer_append_byte(code_buf, 0x5A);   // pop rdx
                    code_buffer_append_byte(code_buf, 0x59);   // pop rcx
                    code_buffer_append_byte(code_buf, 0x58);   // pop rax
                    break;}

                case IR_RES_PUSH:
                    // push rax
                    code_buffer_append_byte(code_buf, 0x50);
                    break;

                case IR_POP_REG:
                    if (node->pop_reg.num == REG_RAX) {
                        // pop rax
                        code_buffer_append_byte(code_buf, 0x58);
                    } else if (node->pop_reg.num == REG_RBX) {
                        // pop rbx
                        code_buffer_append_byte(code_buf, 0x5B);
                    }
                    break;

                case IR_MOV:{

                    // mov reg_to, reg_from
                    code_buffer_append_byte(code_buf, 0x48); // REX.W prefix
                    code_buffer_append_byte(code_buf, 0x89); // MOV opcode

                    uint8_t modrm = 0xC0 | (node->mov.from << 3) | node->mov.to;
                    code_buffer_append_byte(code_buf, modrm);
                }

                default:
                    break;
            }

            current = current->next;
        }
    }

    for (size_t i = 0; i < fixup_count; i++) {
        Fixup *f = &fixups[i];
        size_t target_addr = 0;
        bool found = false;

        // Find the target label/function
        for (size_t j = 0; j < label_count; j++) {
            if (strcmp(labels[j].name, f->target) == 0) {
                target_addr = labels[j].address;
                found = true;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "Error: Could not find target %s\n", f->target);
            continue;
        }

        // Calculate relative offset properly
        // The actual code will be loaded at 0x400000 + ehdr_size + phdr_size

        uint64_t base_address = 0x400000 + sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
        uint64_t from_addr = base_address + f->code_offset + f->instr_size;
        uint64_t to_addr = base_address + target_addr;
        // printf("%lx %lx %lx %lx\n", base_address, from_addr, to_addr, to_addr - from_addr);

        // Calculate 32-bit relative offset
        int64_t offset = (int64_t)(to_addr - from_addr);

        if (offset < INT32_MIN || offset > INT32_MAX) {
            fprintf(stderr, "Error: Jump offset too large for 32-bit rel at %zx\n",
                   f->code_offset);
            continue;
        }

        // Patch the code
        uint8_t *code = code_buf->code + f->code_offset;
        printf("%lx\n", f->code_offset);
        memcpy(code, &offset, sizeof(int32_t));
    }

    // Find the entry point (main function)
    for (size_t i = 0; i < func_count; i++) {
        if (strcmp(func_entries[i].name, "main") == 0) {
            *entry_point = 0x400000 + sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + func_entries[i].address;
            break;
        }
    }

    // Clean up
    for (size_t i = 0; i < label_count; i++) {
        if (labels[i].name[0] == 'L') {  // Only free generated label names
            free(labels[i].name);
        }
    }
    free(labels);

    for (size_t i = 0; i < fixup_count; i++) {
        if (fixups[i].target[0] == 'L') {  // Only free generated label names
            free(fixups[i].target);
        }
    }
    free(fixups);

    free(func_entries);
}



// Generate ELF file directly
void generate_elf_direct(IRFuncs* ir, const char* output_filename) {
    CodeBuffer code_buf;
    CodeBuffer func_table_buf;

    code_buffer_init(&code_buf);
    code_buffer_init(&func_table_buf);

    uint64_t entry_point = 0;
    generate_machine_code(ir, &code_buf, &entry_point);

    // Open output file
    FILE *elf_file = fopen(output_filename, "wb");
    if (!elf_file) {
        perror("Failed to create output file");
        free(code_buf.code);
        free(func_table_buf.code);
        return;
    }

    // Calculate offsets
    uint64_t ehdr_size = sizeof(Elf64_Ehdr);
    uint64_t phdr_size = sizeof(Elf64_Phdr);
    uint64_t phdr_offset = ehdr_size;
    uint64_t code_offset = ehdr_size + phdr_size;
    uint64_t code_vaddr = 0x400000 + code_offset;

    // Write ELF header
    write_elf_header(elf_file, entry_point, phdr_offset);

    // Write program header
    write_program_header(elf_file, code_offset, code_vaddr, code_buf.size, code_buf.size);

    // Write actual code
    fwrite(code_buf.code, 1, code_buf.size, elf_file);

    // Clean up
    free(code_buf.code);
    free(func_table_buf.code);
    fclose(elf_file);
}
