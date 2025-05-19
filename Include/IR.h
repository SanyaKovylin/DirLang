#ifndef IR_H_INCLUDED
#define IR_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "translator.h"
#include "parser.h"

typedef enum {
    IR_LABEL,
    IR_ASSIGN,
    IR_BINOP,
    IR_UNOP,
    IR_CALL,
    IR_JUMP,
    IR_CJUMP,
    IR_RETURN,
    IR_PRINT,
    IR_VAR,
    IR_CONST,
    IR_START,
    IR_RES_PUSH,
    IR_POP_REG,
    IR_MOV,
    IR_INPUT
} IRNodeType;

typedef enum {
        #define OPERATOR(name, ...) OP_##name,

        #include "operators.h"

        #undef OPERATOR
} IROperator;

typedef struct IRNode IRNode;
typedef struct IRList IRList;

struct IRNode {
    IRNodeType type;
    union {
        struct {
            char name[32];
        } start;

        struct {
            int label_num;
        } label;

        struct {
            int pos_dest;
        } assign;

        struct {
            IROperator op;
        } binop;

        struct {
            IROperator op;
            IRNode *operand;
            char dest[32];
        } unop;

        struct {
            char func_name[32];
            int nargs;
        } call;

        struct {
            int target_label;
        } jump;

        struct {
            int false_label;
        } cjump;

        struct {
            int var_pos;
            double num_value;
        } var_const;

        struct {

        } push_var;

        struct {
            int num;
        } pop_reg;

        struct {
            int from;
            int to;
        } mov;


    };
};

struct IRList {
    IRNode *node;
    IRList *prev;
    IRList *next;
};

IRNode* create_ir_node(IRNodeType type);
IRNode* create_binop(IROperator op);
IRNode* create_assign(int pos_dest);
IRNode* create_call(const char *func_name, int nargs);
IRNode* create_cjump(int false_label);
IRNode* create_jump(int target_label);
IRNode* create_label(int label_num);
IRNode* create_return();
IRNode* create_print();
IRNode* create_var(char name);
IRNode* create_const(double value);
IRNode* create_start(const char* name);
IRNode* create_res_push();
IRNode* create_pop_reg(int num);
IRNode* create_mov(int from, int to);
IRNode* create_input();

typedef enum {
    REG_RAX = 0,    // 000
    REG_RCX = 1,    // 001
    REG_RDX = 2,    // 010
    REG_RBX = 3,    // 011
    REG_RSP = 4,    // 100
    REG_RBP = 5,    // 101
    REG_RSI = 6,    // 110
    REG_RDI = 7,    // 111
} RegisterID;

typedef struct IRFunction {
    char* name;
    IRList* list;
    int nargs;
    int nlocals;
} IRFunction;

typedef struct IRFuncs {
    IRFunction* funcs;
    int nfuncs;
} IRFuncs;

IRList* IRParseFuncTree(e_tree *Tree, IRFuncs* curr_func, IRList* ir);
const char* get_type(IRNodeType type);
void DumpIR(IRFuncs* ir, int nfuncs);
const char* get_reg_name(int reg_num);
IRFuncs* TranslateToIR(Function* functions, int nfuncs);
void ProcessIR(IRFuncs* funcs);
void generate_elf_direct(IRFuncs* ir, const char* output_filename);
#endif
