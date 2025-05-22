#include <assert.h>

#include "IR.h"

static int label_counter = 0;
Function* current_function = NULL;

// LList ========================================================================================

IRList* InitIR(const char* name){
    IRList* ir = (IRList*) calloc (1, sizeof(IRList));
    ir->start = (LList*) calloc (8, sizeof(LList));
    ir->start->node = create_start(name);
    ir->capacity = 8;
    ir->count = 1;
    return ir;
}

void ListAppend(IRList* ir, IRNode* node){

    if (ir->count == ir->capacity) {
        ir->start = (LList*) realloc (ir->start, ir->capacity*2 * sizeof(LList));
        ir->capacity *= 2;
    }

    LList* curr = ir->start + ir->count;
    curr->prev = ir->start + ir->count - 1;
    curr->node = node;
    curr->prev->next = curr;
    printf("%d %p %p\n", ir->count, curr->prev, curr);
    ir->count++;
}

void FreeIR(IRList* ir){

    for (size_t i = 0; i < ir->count; i++){
        free(ir->start[i].node);
    }

    free(ir->start);
    free(ir);
}

void Link(IRFuncs* funcs){

    for (int i = 0; i < funcs->nfuncs; i++){
        LinkList(funcs->funcs[i].list);
    }
}

#define type(curr) curr->node->type

void LinkList(IRList* ir){
    if (!ir) return;
    for (int i = 0; i < ir->count; i++){

        LList* curr = ir->start + i;
        if (!i) {curr->next = ir->start + 1; curr->prev = NULL; continue;}
        if (i == ir->count - 1) {curr->prev = ir->start + i - 1; curr->next = NULL; continue;}
        curr->prev = ir->start + i - 1;
        curr->next = ir->start + i + 1;
    }
}


// ==========================================================================================

IRFuncs* TranslateToIR(Function* functions, int nfuncs){

    IRFuncs* allfuncs = (IRFuncs*) calloc (1, sizeof(IRFuncs));
    allfuncs->funcs = (IRFunction*) calloc (nfuncs + 3, sizeof(IRFunction));
    allfuncs->nfuncs = nfuncs;

    for (int cnt = 0; cnt < nfuncs; cnt++){

        IRList* ir = InitIR(functions[cnt].name);

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

#define ADD_NODE(node) \
    ListAppend(ir, node);

#define REC_CALL \
    Tree->curr_node = &curr->left;\
    IRParseFuncTree(Tree, funcs, ir);\
    Tree->curr_node = &curr->right;\
    IRParseFuncTree(Tree, funcs, ir);\

// Parse Tree=======================================================================================

void IRParseFuncTree(e_tree *Tree, IRFuncs* funcs, IRList* ir) {

    e_node* curr = *Tree->curr_node;
    if (curr == NULL) return;

    printf("TYPE: %d \n", curr->type);

    switch(curr->type) {
        case NUM:       handle_num_case(curr, ir);            break;
        case VAR:       handle_var_case(curr, ir);            break;
        case OPER:      handle_oper_case(Tree, funcs, ir);    break;
        case KEYWORD:   handle_keyword_case(Tree, funcs, ir); break;
        case NEXT:      handle_next_case(Tree, funcs, ir);    break;
        case FUNC:      handle_func_case(Tree, funcs, ir);    break;
        default:        break;
    }
}

void handle_num_case(e_node *curr, IRList *ir) {
    ADD_NODE(create_const(curr->value.number));
    ADD_NODE(create_res_push());
}

void handle_var_case(e_node *curr, IRList *ir) {
    ADD_NODE(create_var((char)curr->value.var));
    ADD_NODE(create_res_push());
}

void handle_oper_case(e_tree *Tree, IRFuncs *funcs, IRList *ir) {

    e_node* curr = *Tree->curr_node;
    REC_CALL;

    IROperator op = OP_ADD;
#define OPERATOR(name, ...) case name: op = OP_##name; puts(#name "\n"); break;

    switch (curr->value.var) {
        #include "operators.h"
        default: fputs("unknown operator", stderr);
    }

#undef OPERATOR

    ADD_NODE(create_pop_reg(REG_RBX));
    ADD_NODE(create_pop_reg(REG_RAX));
    ADD_NODE(create_binop(op));
    ADD_NODE(create_res_push());
}

void handle_keyword_case(e_tree *Tree, IRFuncs *funcs, IRList *ir) {
    e_node* curr = *Tree->curr_node;
    printf("keyword type: %ld\n", curr->value.var);

    switch (curr->value.var) {
        case EMPTY:     REC_CALL; break;
        case PRINT:     handle_print_case(Tree, funcs, ir); break;
        case EQUAL:     handle_equal_case(Tree, funcs, ir); break;
        case IF:        handle_if_case(Tree, funcs, ir);    break;
        case BACK:      handle_back_case(Tree, funcs, ir);  break;
        default:        break;
    }
}

void handle_print_case(e_tree *Tree, IRFuncs *funcs, IRList *ir) {
    e_node* curr = *Tree->curr_node;
    REC_CALL;
    ADD_NODE(create_pop_reg(REG_RAX));
    ADD_NODE(create_print());
}

void handle_equal_case(e_tree *Tree, IRFuncs *funcs, IRList *ir) {
    e_node* curr = *Tree->curr_node;

    if (curr->right) {
        Tree->curr_node = &curr->right;
        IRParseFuncTree(Tree, funcs, ir);
    } else {
        ADD_NODE(create_input());
    }

    IRNode* var = create_var((char)curr->left->value.var);
    ADD_NODE(create_assign(var->var_const.var_pos));
    free(var);
}

void handle_if_case(e_tree *Tree, IRFuncs *funcs, IRList *ir) {

    e_node* curr = *Tree->curr_node;
    Tree->curr_node = &curr->left;
    IRParseFuncTree(Tree, funcs, ir);

    ADD_NODE(create_pop_reg(REG_RAX));
    int false_label = new_label();
    int end_label = (curr->right && curr->right->type != NEXT) ? new_label() : false_label;

    ADD_NODE(create_cjump(false_label));

    Tree->curr_node = &curr->right;
    IRParseFuncTree(Tree, funcs, ir);

    if (curr->right && curr->right->type != NEXT) {
        ADD_NODE(create_jump(end_label));
        ADD_NODE(create_label(false_label));
    }
    ADD_NODE(create_label(end_label));
}

void handle_back_case(e_tree *Tree, IRFuncs *funcs, IRList *ir) {
    e_node* curr = *Tree->curr_node;
    REC_CALL;
    if (curr->left || curr->right) {
        ADD_NODE(create_pop_reg(REG_RAX));
    }
    ADD_NODE(create_return());

}

void handle_next_case(e_tree *Tree, IRFuncs *funcs, IRList *ir) {
    e_node* curr = *Tree->curr_node;
    REC_CALL;
}

void handle_func_case(e_tree *Tree, IRFuncs *funcs, IRList *ir) {
    e_node* curr = *Tree->curr_node;
    REC_CALL;
    ADD_NODE(create_call(funcs->funcs[curr->value.var].name,
                       funcs->funcs[curr->value.var].nargs));
    ADD_NODE(create_res_push());
    puts("CALL");
}

// Library =============================================================================================================

void AddLibProt(IRFunction* funcs){

    //READ

    funcs[0].nargs = 0;
    funcs[0].nlocals = 0;
    funcs[0].name = strdup("read");

    IRList* ir = InitIR(funcs[0].name);
    funcs[0].list = ir;

    ADD_NODE(create_call(funcs[0].name, 0));
    ADD_NODE(create_return());

    //PRINT

    funcs[1].nargs = 0;
    funcs[1].nlocals = 0;
    funcs[1].name = strdup("print");

    ir = InitIR(funcs[1].name);
    funcs[1].list = ir;

    ADD_NODE(create_call(funcs[1].name, 0));
    ADD_NODE(create_return());

    //TERMINATE

    funcs[2].nargs = 0;
    funcs[2].nlocals = 0;
    funcs[2].name = strdup("end");
}

#undef ADD_NODE
#undef REC_CALL


// UTILS ================================================================================================================

int new_label() {
    return label_counter++;
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
static const char* op_to_str(IROperator op);

void DumpIR(IRFuncs* ir, int nfuncs)
{
    assert(ir);

    #define print(...) fprintf(logFile, __VA_ARGS__)
    print("===================|IR dump|===================\n");

    for (int i = 0; i < nfuncs; i++){
        print("---------------(New Func)---------------------\n");
        puts(ir->funcs[i].name);
        dump_ir(ir->funcs[i].list->start, logFile);
    }

    #undef print
}


void dump_ir(LList *ir_list, FILE *out) {
    if (!ir_list ) return;

    fprintf(out, "=================== FUNC DUMP ===================\n");

    LList *current = ir_list;
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
        case OP_PI:
        case OP_SQRT:
        default: return "?";
    }
}
