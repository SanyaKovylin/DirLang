#ifndef TRANSLATOR_H_INCLUDED
#define TRANSLATOR_H_INCLUDED

#include "parser.h"

struct Funcs {
    Function* funcs;
    int nfuncs;
};

p_err Translator(Function *functions, const char* output, int nfuncs);
void SetTransOutput(const char* name);

void Preamble();
void FuncPreamble(Function* func);
int findmain();
p_err ParseFuncTree(e_tree *Tree, Function* curr_func);

int find_var(Function* curr_func, const char name);
void print_FUNC(e_node* curr, Function* curr_func);


#endif
