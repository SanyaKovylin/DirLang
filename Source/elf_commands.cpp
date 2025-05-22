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


extern FILE* nasm_file;

void handle_ir_assign(IRFunction* func, CodeBuffer* code_buf, IRNode* node) {
    COMMAND("\x48\x89\x45", "mov [rbp - %d], rax", calculate_arg_shift(func, node->var_const.var_pos));
    code_buffer_append_byte(code_buf, calculate_arg_shift(func, node->var_const.var_pos));
}

void handle_ir_binop(CodeBuffer* code_buf, IRNode* node) {
    switch(node->binop.op) {
        case OP_ADD:
            COMMAND("\x48\x01\xD8", "add rax, rbx");
            break;
        case OP_SUB:
            COMMAND("\x48\x29\xD8", "sub rax, rbx");
            break;
        case OP_MUL:
            COMMAND("\x48\x0F\xAF\xC3", "imul rax, rbx");
            break;
        case OP_DIV:
            COMMAND("\x48\x99", "cqo");
            COMMAND("\x48\xF7\xFB", "idiv rbx");
            break;
        // Handle other binary operations as needed
    }
}

void handle_ir_call(Fixups* fixups, CodeBuffer* code_buf, IRNode* node) {
    COMMAND("\xE8", "call %s", node->call.func_name);
    code_buffer_append_u32(code_buf, 0); // Placeholder
    add_fixup(fixups, node->call.func_name, code_buf->size - 4, 0, 4);

    if (node->call.nargs > 0) {
        COMMAND("\x48\x83\xC4", "add rsp, %d", node->call.nargs * 8);
        code_buffer_append_byte(code_buf, node->call.nargs * 8);
    }
}

void handle_ir_jump(Fixups* fixups, CodeBuffer* code_buf, IRNode* node) {
    COMMAND("\xE9", "jmp L%d", node->jump.target_label);
    code_buffer_append_u32(code_buf, 0); // Placeholder
    add_fixup(fixups, NULL, code_buf->size - 4, node->jump.target_label, 4);
}

void handle_ir_cjump(Fixups* fixups, CodeBuffer* code_buf, IRNode* node) {
    COMMAND("\x48\x85\xC0", "test rax, rax");
    COMMAND("\x0F\x84", "je L%d", node->cjump.false_label);
    size_t cjump_pos = code_buf->size;
    code_buffer_append_u32(code_buf, 0); // Placeholder
    add_fixup(fixups, NULL, code_buf->size - 4, node->cjump.false_label, 4);
}

void handle_ir_return(IRFunction* func, CodeBuffer* code_buf) {
    if (func->nlocals > 0) {
        COMMAND("\x48\x8D\x65\x00", "lea rsp, [rbp]");
    }
    COMMAND("\x48\x89\xEC", "mov rsp, rbp");
    COMMAND("\x5D", "pop rbp");
    COMMAND("\xC3", "ret");
}

void handle_ir_var(IRFunction* func, CodeBuffer* code_buf, IRNode* node) {
    COMMAND("\x48\x8B\x45", "mov rax, [rbp - %d]", calculate_arg_shift(func, node->var_const.var_pos));
    code_buffer_append_byte(code_buf, calculate_arg_shift(func, node->var_const.var_pos));
}

void handle_ir_const(CodeBuffer* code_buf, IRNode* node) {
    COMMAND("\x48\xB8", "mov rax, %g", node->var_const.num_value);
    code_buffer_append_u64(code_buf, (uint64_t)node->var_const.num_value);
}

void handle_ir_res_push(CodeBuffer* code_buf) {
    COMMAND("\x50", "push rax");
}

void handle_ir_pop_reg(CodeBuffer* code_buf, IRNode* node) {
    if (node->pop_reg.num == REG_RAX) {
        COMMAND("\x58", "pop rax");
    } else if (node->pop_reg.num == REG_RBX) {
        COMMAND("\x5B", "pop rbx");
    }
}

void handle_ir_mov(CodeBuffer* code_buf, IRNode* node) {
    COMMAND("\x48\x89", "mov %s, %s",
           get_reg_name(node->mov.to),
           get_reg_name(node->mov.from));
    uint8_t modrm = 0xC0 | (node->mov.from << 3) | node->mov.to;
    code_buffer_append_byte(code_buf, modrm);
}
