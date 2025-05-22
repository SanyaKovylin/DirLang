#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <elf.h>
#include <stdint.h>

#include "IR.h"
#include "elfgen.h"

FILE* nasm_file = fopen("nasm_file", "w");

// Buffer operations ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void code_buffer_init(CodeBuffer *buf) {
    buf->capacity = 1024;
    buf->size = 0;
    buf->code = (uint8_t*) calloc(buf->capacity/sizeof(uint8_t), sizeof(uint8_t));
}

void code_buffer_append(CodeBuffer *buf, const void *data, size_t len){
    while (buf->size + len > buf->capacity) {
        buf->capacity *= 2;
        buf->code = (uint8_t *)realloc(buf->code, buf->capacity);
    }
    memcpy(buf->code + buf->size, data, len);
    buf->size += len;
}

void code_buffer_append_byte(CodeBuffer *buf, uint8_t byte) {
    code_buffer_append(buf, &byte, 1);
}


void code_buffer_append_u32(CodeBuffer *buf, uint32_t value) {
    code_buffer_append(buf, &value, 4);
}


void code_buffer_append_u64(CodeBuffer *buf, uint64_t value) {
    code_buffer_append(buf, &value, 8);
}

//==============================================================================================

void write_elf_header(FILE *out, uint64_t entry_point, uint64_t phdr_offset) {
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

void write_program_header(FILE *out, uint64_t offset, uint64_t vaddr,
                               uint64_t filesz, uint64_t memsize) {
    Elf64_Phdr phdr;
    memset(&phdr, 0, sizeof(phdr));

    phdr.p_type = PT_LOAD;
    phdr.p_offset = offset;
    phdr.p_vaddr = vaddr;
    phdr.p_paddr = vaddr;
    phdr.p_filesz = filesz;
    phdr.p_memsz = memsize;
    phdr.p_flags = PF_X | PF_R;
    phdr.p_align = 0x1000;

    fwrite(&phdr, 1, sizeof(phdr), out);
}

//===================================================================================================================

uint8_t calculate_arg_shift(IRFunction* func, int position){
    uint8_t ans = 0;
    if (position < func->nargs) ans = (uint8_t) ((func->nargs - position + 1)* 8);
    else ans = (uint8_t) ((0 - position) * 8);
    return ans;
}

//=================================================================================================================
Labels* init_labels(){

    Labels* node = (Labels*) calloc (1, sizeof(Labels));
    node->capacity = init_size;
    node->count = 0;
    node->labels = (Label*) calloc (init_size, sizeof(Label));
    return node;
}

ElfError free_labels(Labels* labels){
    if (!labels || !labels->labels) return STRUCT_EMPTY;

    for (size_t i = 0; i < labels->count; i++){
        if (labels->labels[i].name) free(labels->labels[i].name);
        else return EMPTY_NAME;
    }

    free(labels->labels);
    free(labels);
    return OK;
}

ElfError free_fixups(Fixups* fixups){
    if (!fixups || !fixups->fixups) return STRUCT_EMPTY;

    for (size_t i = 0; i < fixups->count; i++){
        if (fixups->fixups[i].target) free(fixups->fixups[i].target);
        else return EMPTY_NAME;
    }

    free(fixups->fixups);
    free(fixups);
    return OK;
}

Fixups* init_fixups(){

    Fixups* node = (Fixups*) calloc (1, sizeof(Fixups));
    node->capacity = init_size;
    node->count = 0;
    node->fixups = (Fixup*) calloc (init_size, sizeof(Fixup));
    return node;
}

ElfError add_label(Labels* labels, char* name, size_t address, int number){

    if (!labels) return STRUCT_EMPTY;

    if (labels->count == labels->capacity){

        labels->labels = (Label*) realloc (labels->labels, labels->capacity * 2 * sizeof(Label));
        labels->capacity *= 2;
    }

    if (!labels->labels) return ALLOCATION_ERROR;
    if (name){
        labels->labels[labels->count].name = strdup(name);
    } else {
        char label_name[8];
        snprintf(label_name, sizeof(label_name), "L%d", number);
        labels->labels[labels->count].name = strdup(label_name);
    }

    fprintf(nasm_file, "%s:\n", labels->labels[labels->count].name);

    labels->labels[labels->count].address = address;
    labels->count++;
    return OK;
}

ElfError add_fixup(Fixups* fixups, char* name, size_t address, int number, size_t instr_size){

    if (!fixups) return STRUCT_EMPTY;

    if (fixups->count == fixups->capacity){

        fixups->fixups = (Fixup*) realloc (fixups->fixups, fixups->capacity * 2 * sizeof(Fixup));
        fixups->capacity *= 2;
    }

    if (!fixups->fixups) return ALLOCATION_ERROR;

    if (name){
        fixups->fixups[fixups->count].target = strdup(name);
    } else {
        char target_name[8];
        snprintf(target_name, sizeof(target_name), "L%d", number);
        fixups->fixups[fixups->count].target = strdup(target_name);
    }

    fixups->fixups[fixups->count].code_offset = address;
    fixups->fixups[fixups->count].instr_size = instr_size;

    fixups->count++;
    return OK;
}

// ================================================================================================================

void generate_machine_code(IRFuncs* ir, CodeBuffer *code_buf,
                                uint64_t *entry_point, int shift) {

    Labels* labels = init_labels();
    Fixups* fixups = init_fixups();

    FirstPass(ir, code_buf, entry_point, shift, labels, fixups);
    SecondPass(ir, code_buf, shift, labels, fixups);

    free_labels(labels);
    free_fixups(fixups);

}

void FirstPass(IRFuncs* ir, CodeBuffer* code_buf, uint64_t* entry_point, int shift, Labels* labels, Fixups* fixups){

    FunctionEntry *func_entries = (FunctionEntry *) calloc(ir->nfuncs , sizeof(FunctionEntry));
    size_t func_count = 0;

    for (int i = 0; i < ir->nfuncs; i++) {
        IRFunction *func = &ir->funcs[i];

        add_label(labels, func->name, code_buf->size, 0);

        func_entries[func_count].name = func->name;
        func_entries[func_count].address = code_buf->size;
        func_count++;

        COMMAND("\x55", "push rbp");
        COMMAND("\x48\x89\xE5", "mov rbp, rsp");

        if (!func->list) continue;
        LList *current = func->list->start;

        if (func->nlocals > 0) {
            COMMAND("\x48\x83\xEC", "sub rsp, %d", func->nlocals * 8);
            code_buffer_append_byte(code_buf, (uint8_t)func->nlocals * 8);
        }

        while (current != NULL) {

            IRNode *node = current->node;

            if (!node) {
                current = current->next;
                continue;
            }

            switch(node->type) {
                case IR_LABEL:
                    add_label(labels, 0, code_buf->size, node->label.label_num); break;

                case IR_ASSIGN:     handle_ir_assign(func, code_buf, node);  break;
                case IR_BINOP:      handle_ir_binop(code_buf, node);         break;
                case IR_CALL:       handle_ir_call(fixups, code_buf, node);  break;
                case IR_JUMP:       handle_ir_jump(fixups, code_buf, node);  break;
                case IR_CJUMP:      handle_ir_cjump(fixups, code_buf, node); break;
                case IR_RETURN:     handle_ir_return(func, code_buf);        break;
                case IR_VAR:        handle_ir_var(func, code_buf, node);     break;
                case IR_CONST:      handle_ir_const(code_buf, node);         break;
                case IR_RES_PUSH:   handle_ir_res_push(code_buf);            break;
                case IR_POP_REG:    handle_ir_pop_reg(code_buf, node);       break;
                case IR_MOV:        handle_ir_mov(code_buf, node);           break;

                default:
                    break;
            }

            current = current->next;
        }

    }

    for (size_t i = 0; i < func_count; i++) {
        if (strcmp(func_entries[i].name, "main") == 0) {
            *entry_point = shift + sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + func_entries[i].address;
            break;
        }
    }

    free(func_entries);
}

void SecondPass(IRFuncs* ir, CodeBuffer* code_buf, int shift, Labels* labels, Fixups* fixups){

    for (size_t i = 0; i < fixups->count; i++) {
        Fixup *f = &fixups->fixups[i];
        size_t target_addr = 0;
        bool found = false;

        for (size_t j = 0; j < labels->count; j++) {
            if (strcmp(labels->labels[j].name, f->target) == 0) {
                target_addr = labels->labels[j].address;
                found = true;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "Error: Could not find target %s\n", f->target);
            continue;
        }

        uint64_t base_address = shift + sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
        uint64_t from_addr = base_address + f->code_offset + f->instr_size;
        uint64_t to_addr = base_address + target_addr;

        int64_t offset = (int64_t)(to_addr - from_addr);

        uint8_t *code = code_buf->code + f->code_offset;
        printf("%lx\n", f->code_offset);
        memcpy(code, &offset, sizeof(int32_t));
    }
}

// ==================================================================================================================
// Generate ELF file directly
void generate_elf_direct(IRFuncs* ir, const char* output_filename) {

    CodeBuffer code_buf;
    CodeBuffer func_table_buf;

    code_buffer_init(&code_buf);
    code_buffer_init(&func_table_buf);

    int shift = 0x400000;

    char* lib = NULL;
    int lenlib = GetLibrary("lib/IO.o", &lib);

    uint64_t entry_point = 0;
    generate_machine_code(ir, &code_buf, &entry_point, shift);

    FILE *elf_file = fopen(output_filename, "wb");

    uint64_t ehdr_size = sizeof(Elf64_Ehdr);
    uint64_t phdr_size = sizeof(Elf64_Phdr);
    uint64_t phdr_offset = ehdr_size;
    uint64_t code_offset = ehdr_size + phdr_size;
    uint64_t code_vaddr = shift + code_offset;

    write_elf_header(elf_file, entry_point, phdr_offset);
    write_program_header(elf_file, code_offset, code_vaddr, code_buf.size, code_buf.size);

    // Write actual code
    fwrite(code_buf.code, 1, code_buf.size  - 32*sizeof(char), elf_file);
    write_lib(elf_file, lib, lenlib);

    free(code_buf.code);
    free(func_table_buf.code);
    fclose(elf_file);
}

int GetLibrary(const char* libfile, char** buffer){

    int lenlib = Read(libfile, buffer);
    printf("LIB: %d\n", lenlib);
    *buffer += 0x180;
    lenlib -= 0x180;
    return lenlib;
}

void write_lib(FILE* elf_file, char* buffer, int lenlib){

    fwrite(buffer, sizeof(char), lenlib, elf_file);
    free(buffer - 0x180);
}
