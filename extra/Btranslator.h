#ifndef B_TRANSLATOR_H_INCLUDED
#define B_TRANSLATOR_H_INCLUDED

#include "parser.h"
#include "translator.h"

typedef struct VarReg {
    char name[4];
} VarReg;

const VarReg VarRegs[] = {
    "r12",
    "r13",
    "r14",
    "r15"
};

const int NVarRegs = sizeof(VarRegs)/sizeof(VarReg);

p_err BTranslator(Function *functions, const char* output, int nfuncs);
void SetBTransOutput(const char* name);

void BPreamble();
void FuncBPreamble(Function* func);
int Bfindmain();
p_err NBParseFuncTree(e_tree *Tree, Function* curr_func);

char* b_find_var(Function* curr_func, const char name);
void print_BFUNC(e_node* curr, Function* curr_func);


#endif
