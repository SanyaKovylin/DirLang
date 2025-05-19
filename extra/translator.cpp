#include <stdio.h>
#include <stdlib.h>

#include "translator.h"


static FILE* mainstream = stdout;

void SetTransOutput(const char* name){
    mainstream = fopen(name, "w");
}

Funcs* funcs = NULL;

p_err Translator(Function *functions, const char* output, int nfuncs){

    SetTransOutput(output);
    puts("\n\n");
    Preamble();

    static Funcs allfuncs = {
        .funcs = functions,
        .nfuncs = nfuncs,
    };

    funcs = &allfuncs;

    for (int cnt = 0; cnt < nfuncs; cnt++){

        FuncPreamble(functions + cnt);
        // puts(functions[cnt].name);
        functions[cnt].functree->curr_node = &functions[cnt].functree->head;
        ParseFuncTree(functions[cnt].functree, functions + cnt);
    }
    fclose(mainstream);
    return EX;
}

int findmain(){

    for (int i = 0; i < funcs->nfuncs;i++){
        if (!strcmp(funcs->funcs[i].name, "main")) return i;
    }

    return -1;
}

void FuncPreamble(Function* func){

    fputs("\n\n", mainstream);
    fputs(func->name, mainstream);
    fputs(":\n", mainstream);
}

void Preamble(){

    fputs("push 0\n", mainstream);
    fputs("pop bx\n", mainstream);
    fputs("push 0\n", mainstream);
    fputs("pop ax\n", mainstream);
    // fputs("push 0\n", mainstream);

    fputs("call main\n"
    //  "out\n"
     , mainstream);

    fputs("hlt\n", mainstream);
}

#define REC_CALL \
    Tree->curr_node = &curr->left;\
    ParseFuncTree(Tree, curr_func);\
    Tree->curr_node = &curr->right;\
    ParseFuncTree(Tree, curr_func);

#define REC_RIGHT \
    Tree->curr_node = &curr->right;\
    ParseFuncTree(Tree, curr_func);

#define REC_LEFT \
    Tree->curr_node = &curr->left;\
    ParseFuncTree(Tree, curr_func);

p_err ParseFuncTree(e_tree *Tree, Function* curr_func){

    e_node* curr  = *Tree->curr_node;

    if (curr == NULL) return EX;

    printf("func: %s\n", curr_func->name);

    switch(curr->type){

        case NUM:
            fprintf(mainstream,"push %g\n", curr->value.number);
            break;

        case VAR:
            {

            char name = curr->value.var;


            fprintf(mainstream,
            // "push %d\n"
            //  "push bx\n"
            //  "add\n"
            //  "pop ax\n"
            //  "push [ax]\n"
            "push[bx+%d]\n", find_var(curr_func, name));

            break;
            }

        case OPER:

            REC_CALL;

            #define OPERATOR(name, ...) case name: fputs(#name "\n", mainstream); break;

            switch (curr->value.var){
                #include "operators.h"

                default: fputs("unknown operator", stderr);
            }

            break;

        case KEYWORD:{

            static int cou = 0;
            cou++;
            puts("kew");

            switch (curr->value.var){

                case EMPTY:
                    REC_CALL;
                    break;

                case PRINT:
                    REC_CALL;
                    fputs("out\n", mainstream);
                    break;

                case EQUAL:

                    Tree->curr_node = &curr->right;
                    if (curr->right != NULL)
                        ParseFuncTree(Tree, curr_func);
                    else
                        fputs("in\n", mainstream);

                    REC_LEFT;
                    for(int i = 0;i < 2; i++)
                        fprintf(mainstream, "pop [bx+%d]\n", find_var(curr_func, curr->left->value.var));
                    break;

                case IF:{

                    REC_LEFT;
                    fprintf(mainstream, "push 0\n" "jae skip%d\n", cou);
                    int save = cou;
                    REC_RIGHT;

                    fprintf(mainstream, "\n\nskip%d:\n", save);
                    break;
                }

                case BACK:
                    REC_CALL;
                    fputs("ret\n", mainstream);
                    break;

                default:
                    fputs("unknown keyword", stderr);
            }}
            break;

        case NEXT:
            REC_CALL;
            break;

        case FUNC: {
            REC_CALL
            print_FUNC(curr, curr_func);

            break;
        }

        default:
            puts("unknown type\n");
    }

    return EX;
}

// int find_var(Function* curr_func, const char name){
//
//     bool got_var = false;
//     printf("var %c\n", name);
//     for (int i = 0; i < curr_func->nargs && !got_var;i++){
//
//         if (curr_func->args[i][0] == name){
//             puts("found arg");
//
//             return i;
//         }
//     }
//
//     printf("nlocals: %d", curr_func->nlocals);
//
//     for (int i = 0; i < curr_func->nlocals && !got_var; i++){
//         if (*curr_func->locals[i] == name){
//             puts("found local");
//
//             return i + curr_func->nargs;
//
//         }
//         else{
//             printf("local name: %c", curr_func->locals[i][0] );
//         }
//     }
//
//     return -1;
// }

void print_FUNC(e_node* curr, Function* curr_func){

    int now = curr->value.var;
    int nvars = curr_func->nargs + curr_func->nlocals;

    for (int i = 0; i < funcs->funcs[now].nargs; i++){
        fprintf(mainstream, "push bx\n" "push %d\n" "add\n" "pop ax\n" "pop [ax]\n", i + nvars);
    }
    fprintf(mainstream, "push bx \n" "push %d\n" "add\n" "pop bx\n", nvars);
    fprintf(mainstream, "call %s\n", funcs->funcs[now].name);
    fprintf(mainstream, "push bx \n" "push %d\n" "sub\n" "pop bx\n", nvars);
}
