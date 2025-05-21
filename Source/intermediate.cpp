#include <assert.h>

#include "IR.h"

static int label_counter = 0;

static Function* current_function = NULL;

void dump_ir(IRList *ir_list, FILE *out);
int new_label();

int new_label() {
    return label_counter++;
}

IRNode* create_ir_node(IRNodeType type) {
    IRNode *node = (IRNode*) calloc (1, sizeof(IRNode));
    node->type = type;
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

IRNode* create_assign(int pos_dest) {
    IRNode *node = create_ir_node(IR_ASSIGN);
    node->assign.pos_dest = pos_dest;
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

static const char* op_to_str(IROperator op) {
    switch(op) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_SIN:
        case OP_COS:
        case OP_ARCSIN:
        case OP_ARCCOS:
        case OP_LOG:
        case OP_LN:
        // case OP_E:
        case OP_PI:
        case OP_SQRT:
        default: return "?";
    }
}

IRFuncs* TranslateToIR(Function* functions, int nfuncs){

    IRFuncs* allfuncs = (IRFuncs*) calloc (1, sizeof(IRFuncs));
    allfuncs->funcs = (IRFunction*) calloc (nfuncs + 3, sizeof(IRFunction));
    allfuncs->nfuncs = nfuncs;

    for (int cnt = 0; cnt < nfuncs; cnt++){

        IRList* ir = (IRList*) calloc (1, sizeof(IRList));
        ir->node = create_start(functions[cnt].name);

        allfuncs->funcs[cnt].nargs = functions[cnt].nargs;
        allfuncs->funcs[cnt].nlocals = functions[cnt].nlocals;
        allfuncs->funcs[cnt].name = functions[cnt].name;

        current_function = functions + cnt;

        functions[cnt].functree->curr_node = &functions[cnt].functree->head;
        IRParseFuncTree(functions[cnt].functree, allfuncs, ir);
        allfuncs->funcs[cnt].list = ir;
    }

    AddLibProt(allfuncs->funcs + nfuncs);
    allfuncs->nfuncs += 3;

    return allfuncs;
}



// list append?
#define ADD_NODE(n) \
    {\
    IRList* newnode = (IRList*) calloc (1, sizeof(IRList)); \
    newnode->node = n;\
    newnode->prev = ir;\
    ir->next = newnode;\
    ir = newnode;\
    }

#define REC_PUSH_CALL \
    Tree->curr_node = &curr->left;\
    ir = IRParseFuncTree(Tree, funcs, ir);\
    ADD_NODE(create_res_push());\
    Tree->curr_node = &curr->right;\
    ir = IRParseFuncTree(Tree, funcs, ir);\
    ADD_NODE(create_res_push());

#define REC_CALL \
    Tree->curr_node = &curr->left;\
    ir = IRParseFuncTree(Tree, funcs, ir);\
    Tree->curr_node = &curr->right;\
    ir = IRParseFuncTree(Tree, funcs, ir);\

void AddLibProt(IRFunction* funcs){

    // IRList* save = NULL;
    //READ

    funcs[0].nargs = 0;
    funcs[0].nlocals = 0;
    funcs[0].name = strdup("read");

    IRList* ir = (IRList*) calloc (1, sizeof(IRList));
    funcs[0].list = ir;
    ir->node = create_start(funcs[0].name);

    ADD_NODE(create_call(funcs[0].name, 0));
    ADD_NODE(create_return());



    //PRINT

    funcs[1].nargs = 0;
    funcs[1].nlocals = 0;
    funcs[1].name = strdup("print");

    ir = (IRList*) calloc (1, sizeof(IRList));
    funcs[1].list = ir;
    ir->node = create_start(funcs[1].name);

    ADD_NODE(create_call(funcs[1].name, 0));
    ADD_NODE(create_return());

    //TERMINATE

    funcs[2].nargs = 0;
    funcs[2].nlocals = 0;
    funcs[2].name = strdup("end");
}

IRList*  IRParseFuncTree(e_tree *Tree, IRFuncs* funcs, IRList* ir) {

    e_node* curr = *Tree->curr_node;
    if (curr == NULL) return ir;
    // dump_ir(ir, stdout);
    printf("TYPE: %d \n", curr->type);
    // На функции ->
    switch(curr->type) {
        case NUM:
            ADD_NODE(create_const(curr->value.number));
            ADD_NODE(create_res_push());

            return ir;
        case VAR:
            ADD_NODE(create_var((char) curr->value.var));
            ADD_NODE(create_res_push());
            return ir;

        case OPER: {
            puts("OPERATOR");
            REC_CALL;

            IROperator op = OP_ADD;

#define OPERATOR(name, ...) case name: op = OP_##name; puts(#name "\n"); break;

            switch (curr->value.var){

                #include "operators.h"

                default: fputs("unknown operator", stderr);
            }

#undef OPERATOR

            ADD_NODE(create_pop_reg(REG_RBX));
            ADD_NODE(create_pop_reg(REG_RAX));
            ADD_NODE(create_binop(op));
            ADD_NODE(create_res_push());
            return ir;
        }

        case KEYWORD: {
            printf("keyword type: %ld\n", curr->value.var);
            switch (curr->value.var) {

                case EMPTY:
                    REC_CALL;
                    return ir;;

                case PRINT:
                    REC_CALL;
                    ADD_NODE(create_pop_reg(REG_RAX));
                    ADD_NODE(create_print());
                    return ir;

                case EQUAL: {
                    if (curr->right != NULL){
                        Tree->curr_node = &curr->right;
                        ir = IRParseFuncTree(Tree, funcs, ir);
                    }
                    else {
                        ADD_NODE(create_input());
                    }
                    IRNode* var = create_var((char)curr->left->value.var);
                    ADD_NODE(create_assign(var->var_const.var_pos));
                    free(var);
                    return ir;
                }

                case IF: {


                    Tree->curr_node = &curr->left;
                    ir = IRParseFuncTree(Tree, funcs, ir);

                    ADD_NODE(create_pop_reg(REG_RAX));
                    int false_label = new_label();
                    int end_label = 0;

                    if (curr->right != NULL && curr->right->type != NEXT)
                         end_label = new_label();
                    else end_label = false_label;

                    ADD_NODE(create_cjump(false_label));

                    Tree->curr_node = &curr->right;
                    ir = IRParseFuncTree(Tree, funcs, ir);

                    if (curr->right != NULL && curr->right->type != NEXT){
                        ADD_NODE(create_jump(end_label));
                        ADD_NODE(create_label(false_label));
                    }
                    ADD_NODE(create_label(end_label));

                    return ir;
                }

                case BACK: {
                    REC_CALL;
                    if (curr->left != NULL || curr->right != NULL)
                        ADD_NODE(create_pop_reg(REG_RAX));
                    ADD_NODE(create_return());
                    return ir;
                }
            }
            return ir;
        }

        case NEXT:
            REC_CALL;
            return ir;

        case FUNC: {
            REC_CALL;

            ADD_NODE(create_call(funcs->funcs[curr->value.var].name, funcs->funcs[curr->value.var].nargs));
            ADD_NODE(create_res_push());
            puts("CALL");
            return ir;
        }

        default:
            return ir;
    }
    return ir;
}

int find_var(Function* curr_func, const char name){

    bool got_var = false;
    printf("var %c\n", name);
    for (int i = 0; i < curr_func->nargs && !got_var;i++){

        if (curr_func->args[i][0] == name){
            puts("found arg");

            return i;
        }
    }

    printf("nlocals: %d", curr_func->nlocals);

    for (int i = 0; i < curr_func->nlocals && !got_var; i++){
        if (*curr_func->locals[i] == name){
            puts("found local");

            return i + curr_func->nargs;

        }
        else{
            printf("local name: %c", curr_func->locals[i][0] );
        }
    }

    return -1;
}

//DUMP IR ========================================================================================================

FILE* logFile = fopen("logfile.log", "w");

const char* get_type(IRNodeType type){
    switch (type){
        case IR_LABEL:  return "Label" ;
        case IR_ASSIGN: return "Assign";
        case IR_BINOP:  return "Binop" ;
        case IR_UNOP:   return "Unop"  ;
        case IR_CALL:   return "Call"  ;
        case IR_JUMP:   return "Jump"  ;
        case IR_CJUMP:  return "Cjump" ;
        case IR_RETURN: return "Return";
        case IR_PRINT:  return "Print" ;
        case IR_VAR:    return "Var"   ;
        case IR_CONST:  return "Const" ;
        case IR_START:  return "Start" ;
        case IR_RES_PUSH: return "PushRes";
        default: return "Error";
    }
}

void DumpIR(IRFuncs* ir, int nfuncs)
{
    assert(ir);

    #define print(...) fprintf(logFile, __VA_ARGS__)
    print("===================|IR dump|===================\n");

    for (int i = 0; i < nfuncs; i++){
        print("---------------(New Func)---------------------\n");
        puts(ir->funcs[i].name);
        dump_ir(ir->funcs[i].list, logFile);
    }

    #undef print
}


void dump_ir(IRList *ir_list, FILE *out) {
    if (!ir_list || !out) return;

    fprintf(out, "=================== FUNC DUMP ===================\n");

    IRList *current = ir_list;
    printf("%p", current);
    puts("");
    while (current != NULL) {

        IRNode *node = current->node;
        if (!node) {
            current = current->next;
            continue;
        }

        switch(node->type) {
            case IR_START:
                fprintf(out, "FUNCTION %s:\n", node->start.name);
                break;

            case IR_LABEL:
                fprintf(out, "L%d:\n", node->label.label_num);
                break;

            case IR_ASSIGN:
                fprintf(out, "[var %d] =;\n", node->assign.pos_dest);
                break;

            case IR_BINOP:
                fprintf(out, "[left] %s [right];\n", op_to_str(node->binop.op));
                break;

            case IR_CALL:
                fprintf(out, "      = call %s(",
                       node->call.func_name);
                fprintf(out, ");\n");
                break;

            case IR_JUMP:
                fprintf(out, "    goto L%d;\n", node->jump.target_label);
                break;

            case IR_CJUMP:
                fprintf(out, "    if not ([condition] ) goto L%d;\n", node->cjump.false_label);
                break;

            case IR_RETURN:
                fprintf(out, "    return [value];\n");
                break;

            case IR_PRINT:
                fprintf(out, "    print [value];\n");
                break;

            case IR_VAR:
                fprintf(out, "    [var %d]\n", node->var_const.var_pos);
                break;

            case IR_CONST:
                fprintf(out, "    [const %g]\n", node->var_const.num_value);
                break;

            case IR_RES_PUSH:
                fprintf(out, "    [push result to stack]\n");
                break;

            case IR_POP_REG:
                fprintf(out, "    [pop to %s]\n", get_reg_name(node->pop_reg.num));
                break;

            case IR_INPUT:
                fprintf(out, "      = input();\n");
                break;

            case IR_MOV:
                fprintf(out, "      mov from %s to %s;\n", get_reg_name(node->mov.from), get_reg_name(node->mov.to));
                break;

            default:
                fprintf(out, "    [unknown IR node type %d]\n", node->type);
        }

        current = current->next;
    }

    fprintf(out, "================ END FUNC DUMP ================\n");
}

const char* get_reg_name(int reg_num) {
    static const char* reg_names[] = {
        "rax", "rcx", "rdx", "rbx",
        "rsp", "rbp", "rsi", "rdi",
        "r8",  "r9",  "r10", "r11",
        "r12", "r13", "r14", "r15"
    };

    return reg_names[reg_num];
}
