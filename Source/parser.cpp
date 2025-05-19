#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "parser.h"
#include "utils.h"
#include "lexsint.h"
#include "translator.h"


ReservedNames ResNames = {};

int ParseFuncs(const char* namefld, Function** funcs){

    DIR *dp = opendir(namefld);
    if (dp == NULL) return EMPTY_FOLDER;

    Function* fnc = (Function*) calloc (startnum, sizeof(Function));
    int cnt = 0;

    ParseReservedNames(dp);
    dp = opendir(namefld);
    SetResNames(ResNames);

    for (dirent *ep = readdir(dp); ep != NULL; ep = readdir(dp)){

        if (strlen(ep->d_name) > 2){
            Function func = ParseFunc(namefld ,ep);
            puts(ep->d_name);

            fnc[cnt] = func;
            cnt++;
        }
    }

    *funcs = fnc;

    return cnt;
}

p_err ParseReservedNames(DIR* dp){

    ResNames = {
        .names = (char**) calloc (lenstr, sizeof(char*)),
        .num = 0,
        .capacity = lenstr,
    };

    for (dirent *ep = readdir(dp); ep != NULL; ep = readdir(dp)){

        if (strlen(ep->d_name) > 2){

            Function fnc = {.name = ep->d_name, .args = (char**) calloc (startnum, sizeof(char*))};
            ParseFuncName(&fnc);
            ResNames.names[ResNames.num] = (char*) calloc (strlen(fnc.name) + 1, sizeof(char));
            memcpy(ResNames.names[ResNames.num], fnc.name, strlen(fnc.name));
            ResNames.num++;
        }
    }
    return EX;
}

Function ParseFunc(const char* name, dirent *fldptr){

    Function new_func = {

        .name = (char*) calloc (lenstr, 1),
        .functree = (e_tree*) calloc (1, sizeof(e_tree)),
        .args = (char**) calloc (startnum, sizeof(char*)),
        .nargs = 0,
        .locals = (char**) calloc (startnum, sizeof(char*)),
        .nlocals = 0,
    };

    memcpy(new_func.name, fldptr->d_name, strlen(fldptr->d_name));
    ParseFuncName(&new_func);

    TreeCtor(new_func.functree, NULL, name);

    char path[lenstr] = {};
    sprintf(path, "%s/%s", name, fldptr->d_name);
    ParseFuncBody(new_func.functree, path, &new_func);
    new_func.functree->curr_node = &new_func.functree->head;
    // PrintTree(new_func.functree);

    new_func.functree->curr_node = &new_func.functree->head;
    // printptr(new_func.functree);
    return new_func;
}

p_err ParseFuncName(Function* func){

    int end = 0;

    while(func->name[end] != '(' && func->name[end] != '\0'){
        end++;
    }
    int nameend = end - 1;
    while (func->name[end] == '(' || func->name[end] == ' ') end++;



    while (func->name[end] != ')' && func->name[end] != '\0'){

        int start = end;
        char *arg = (char*) calloc (lenstr, sizeof(char));

        while (func->name[end] != ')' && func->name[end] != ',') end++;
        memcpy(arg, func->name + start, end - start);

        // printf("arg %s\n", arg);
        func->args[func->nargs++] = arg;
        // end++;
    }

    // printf("end\n");
    while (func->name[nameend] == ' '){
        func->name[nameend] = '\0';
        nameend--;
    }

    return EX;
}

#define skip1st while (name[start] != '$' && name[start] != '.') start++;\
        if (name[start] != '.') start++;

p_err ParseFuncBody(e_tree *Tree, const char* fld, Function* curr_func){

    // puts(fld);
    DIR *dp = opendir(fld);
    assert(dp != NULL);
    dirent *ep = NULL;

    // puts("started body\n");

    while ((ep = readdir(dp)) != NULL){
        if (strlen(ep->d_name)> 2){

            char path[lenstr] = {};
            sprintf(path, "%s/%s", fld, ep->d_name);

            e_node* new_node = NewNodeNEXT(0, NULL, NULL);

            *Tree->curr_node = new_node;
            Tree->curr_node = &new_node->left;

            // puts(ep->d_name);

            DIR* dp1 = NULL;

            if ((dp1 = opendir(path)) != NULL){
                ParseKeyword(Tree, ep, fld, curr_func);
            }
            else {
                // printf("expr found");
                ParseExpression(Tree, ep, fld, curr_func);
            }
            // puts("start expr");

            Tree->curr_node = &new_node->right;
        }
    }

    return EX;
}

p_err ParseKeyword(e_tree *Tree, dirent* ep, const char* fldpath, Function* curr_func){
    int start = 0;

    char path[lenstr] = {};
    sprintf(path,"%s/%s", fldpath, ep->d_name);
    char *name = ep->d_name;

    skip1st

    ParseAddExpr(Tree, name + start);

    e_node* new_node = *Tree->curr_node;

    new_node->left = new_node->right;
    new_node->right = NULL;

    if (new_node != NULL){
        // puts(path);
        Tree->curr_node = &new_node->right;
        ParseFuncBody(Tree, path, curr_func);
    }

    return EX;
}

p_err ParseAddExpr(e_tree* Tree, const char* expr){

    // FILE* add1 = fopen("add.txt", "w");
    // fclose(add1);
    FILE* add = fopen("add.txt", "w");
    fputs(expr, add);
    fclose(add);
    FParseInf("add.txt", Tree);

    return EX;
}

p_err ParseExpression(e_tree *Tree, dirent* ep, const char* fldpath, Function* curr_func){

    int start = 0;

    char path[lenstr] = {};
    sprintf(path,"%s/%s", fldpath, ep->d_name);
    char *name = ep->d_name;
    // puts(path);
    // puts(name);

    skip1st
    RemoveLastDot(name);
//     if (name[start] == '.' && !isalpha(name[start + 1])) return EX;
//
//     int end = 0;
//     while (name[start] == ' ') start++;
//     while (name[end] != '.') end++;
//     while (name[end] == '.' || name[end] == ' ') end--;
//     end++;
//     name[end] = '\0';
//     // printf("expr :%s\n", name + start);
//
    // e_node *new_node = NULL;
    // bool is_var = true;
//
//     #define KEYWORD(Name, num, str, getsbool, fld, ...)\
//         if (!stricmp(str, name + start) && !fld){\
//                 puts(#Name);\
//                 is_var = false;\
//                 new_node = NewNodeKEYWORD(num, NULL, NULL);\
//         }
//
//     #include "keywords.h"
//
//     #undef KEYWORD
    e_node* new_node = NULL;

    if (name[start] == '\0'){new_node = NewNodeKEYWORD(EMPTY, NULL, NULL);}
    else {
        ParseAddExpr(Tree, name + start);
        new_node = *Tree->curr_node;
    }
    // else {is_var = true; puts("here");}
    bool is_var = new_node->type == VAR;

    if (is_var){
        // printf("VAR, %d", is_var);
        bool was = false;
        // new_node = NewNodeKEYWORD(EQUAL, NewNodeVAR(name[start], NULL, NULL), NULL);
        new_node = NewNodeKEYWORD(EQUAL, new_node, NULL);

        for (int i = 0; i < curr_func->nlocals; i++){
            if (name[start] == curr_func->locals[i][0]){
                was = true;
                // printf("%c", name[start]);
            }
        }

        for (int i = 0; i < curr_func->nargs; i++){
            if (name[start] == curr_func->args[i][0]){
                was = true;
            }
        }
        // was = (find_var(curr_func, name[start]) == -1);
        // printf("find var: %d",find_var(curr_func, name[start]));
        if (!was){
            // puts("new variable");
            // printf("%s", &name[start]);
            char *var = (char*) calloc (1, 1);
            memcpy(var, name + start, 1);
            curr_func->locals[curr_func->nlocals] = var;
            curr_func->nlocals++;
        }
    }

    // puts("ea");

    *Tree->curr_node = new_node;
    Tree->curr_node = &new_node->right;

    // printf("started parse %s", path);
    FParseInf(path, Tree);
    // printf("%p", new_node->right);
    return EX;
}

//==========================================================================

void RemoveLastDot(char *name){

    int end = strlen(name);
    while(name[end] != '.') end--;
    name[end] = '\0';

}
