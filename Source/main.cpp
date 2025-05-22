#include <stdlib.h>
#include <stdio.h>

#include "parser.h"
#include "IR.h"
#include "translator.h"
#include "lexsint.h"

int main(void){
    Function * e = NULL;
    int nfuncs = ParseFuncs("./Progs/1st", &e);
    // Translator(e, "arab.txt", nfuncs);
    IRFuncs* a = TranslateToIR(e, nfuncs);
    Link(a);
    ProcessIR(a);
    puts("EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
    DumpIR(a, nfuncs);
    generate_elf_direct(a, "Build/Prog");
}

