#include <assert.h>

#include "IR.h"

extern Function* current_function;

IRNode* create_ir_node(IRNodeType type) {
    IRNode *node = (IRNode*) calloc (1, sizeof(IRNode));
    node->type = type;
    return node;
}

IRNode* create_assign(int pos_dest) {
    IRNode *node = create_ir_node(IR_ASSIGN);
    node->assign.pos_dest = pos_dest;
    return node;
}

IRNode* create_input() {
    IRNode *node = create_ir_node(IR_INPUT);
    return node;
}

IRNode* create_binop(IROperator op) {
    IRNode *node = create_ir_node(IR_BINOP);
    node->binop.op = op;
    return node;
}

IRNode* create_call(const char *func_name, int nargs) {
    IRNode *node = create_ir_node(IR_CALL);
    strncpy(node->call.func_name, func_name, sizeof(node->call.func_name));
    node->call.nargs = nargs;
    return node;
}

IRNode* create_cjump(int false_label) {
    IRNode *node = create_ir_node(IR_CJUMP);
    node->cjump.false_label = false_label;
    return node;
}

IRNode* create_jump(int target_label) {
    IRNode *node = create_ir_node(IR_JUMP);
    node->jump.target_label = target_label;
    return node;
}

IRNode* create_label(int label_num) {
    IRNode *node = create_ir_node(IR_LABEL);
    node->label.label_num = label_num;
    return node;
}

IRNode* create_pop_reg(int num) {
    IRNode *node = create_ir_node(IR_POP_REG);
    node->pop_reg.num = num;
    return node;
}

IRNode* create_mov(int from, int to){
    IRNode *node = create_ir_node(IR_MOV);
    node->mov.from = from;
    node->mov.to = to;
    return node;
}

IRNode* create_return() {
    IRNode *node = create_ir_node(IR_RETURN);
    return node;
}

IRNode* create_print() {
    IRNode *node = create_ir_node(IR_PRINT);
    return node;
}

IRNode* create_var(char name) {
    IRNode *node = create_ir_node(IR_VAR);
    node->var_const.var_pos = find_var(current_function, name);
    return node;
}

IRNode* create_start(const char *name) {
    IRNode *node = create_ir_node(IR_START);
    strncpy(node->start.name, name, sizeof(node->start.name));
    return node;
}

IRNode* create_const(double value) {
    IRNode *node = create_ir_node(IR_CONST);
    node->var_const.num_value = value;
    return node;
}

IRNode* create_res_push(){
    IRNode* node = create_ir_node(IR_RES_PUSH);
    return node;
}



