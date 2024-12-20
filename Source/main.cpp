#include <stdlib.h>
#include <stdio.h>

#include "parser.h"
#include "translator.h"
#include "lexsint.h"

int main(void){
    Function * e = NULL;
    int nfuncs = ParseFuncs("./Progs/1st", &e);
    Translator(e, "arab.txt", nfuncs);

//     e_tree T = {};
//     TreeCtor(&T, NULL, "eee");
//
//     FParseInf("ea.txt", &T);
//     T.curr_node = &T.head;
//
//     PrintTree(&T);
}

