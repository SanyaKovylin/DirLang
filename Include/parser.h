#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include "expr_tree.h"


typedef enum ParseError {
    EX = 0,
    AX = 1,
    EMPTY_FOLDER = 2,
} p_err;

struct Function {
    char* name;
    e_tree *functree;
    char** args;
    int nargs;
    char **locals;
    int nlocals;
};


#define KEYWORD(name, num, ...) name = num,

enum Keywords {
    #include "keywords.h"
    COUNT_KEYWORDS
};

#undef KEYWORD
const int startnum = 4;
const int lenstr = 50;

int ParseFuncs(const char* namefld, Function** funcs);
Function ParseFunc(const char* name, dirent* fldptr);
p_err ParseFuncName(Function* func);

p_err ParseFuncBody(e_tree *Tree, const char* fld, Function* curr);
p_err ParseReservedNames(DIR* dp);
p_err ParseKeyword(e_tree *Tree, dirent* ep, const char* path, Function* curr);
p_err ParseAddExpr(e_tree* Tree, const char* expr);
p_err ParseExpression(e_tree *Tree, dirent* ep, const char* path, Function* curr);
void RemoveLastDot(char *name);
#endif //PARSER_H_INCLUDED
