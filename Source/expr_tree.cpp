#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>



#include "expr_tree.h"

static const char *output = NULL;
FILE* outptr = stdout;
e_err SetOutputFile(const char* file){

    output = file;
    outptr = fopen(file, "w");
    PrintPreamble(outptr);
    return TREE_OK;
}

void PrintPreamble(FILE* stream){

    fputs(
"\\input{preamble}\n"
"\\title{Math}"
"\\author{Sanya Kovylin}\n"
"\\date{November 2024}\n"
"\n"
"\\begin{document}\n"
"\n"
"\\maketitle\n"
"\n"
"\\section{Finding the derivative}\n"
, stream);

}

void PrintExit(){

    fputs("\\end{document}", outptr);
}

e_err TreeCtor(e_tree *Tree, e_node *head, const char* name){

    assert(Tree != NULL);
    *Tree = {
        .name = name,
        .head = head,
        .curr_node = &Tree->head,
        .nnodes = 0,
    };

    return TREE_OK;
}

#define DEF_TYPE(name, num, ctype, place, ...) e_node* NewNode##name(ctype value, e_node *Left, e_node* Right){\
\
    e_node *node_val = (e_node*) calloc(1, sizeof(e_node));\
\
    assert(node_val != NULL);\
\
    e_value val = {};\
    val.place = value;\
\
    *node_val = {\
        .value = val,\
        .type = name,\
        .left = Left,\
        .right = Right,\
    };\
\
    return node_val;\
}\

#include "types.h"

#undef DEF_TYPE

//TODO: Add Rec
e_err PrintTree(e_tree *Tree){
    printf("\n");

    FILE *stream = outptr;
    setvbuf(stream, NULL, 0, 0);
    // Tree->curr_node = &Tree->head;

    fprintf(outptr, "\\begin{equation} \n");
    e_err ret = RecPrintTree(Tree, stream);
    fprintf(outptr, "\n\\end{equation} \n");

    printf("\n");

    return ret;
}
e_err RecPrintTree(e_tree *Tree, FILE *stream){

    assert(Tree != NULL);
    assert(Tree->curr_node != NULL);

    if (Tree->curr_node == &Tree->head && stream == stdout) printf("\n");

#pragma GCC diagnostic ignored "-Wnonnull"             //format exception for operattor type
#pragma GCC diagnostic ignored "-Wformat-extra-args"   //contains format == NULL

    switch (NODE->type){

    #define DEF_TYPE(name, num, type, place, format, ...) \
    case name: {if (format != NULL) fprintf(stream, format, (type) NODE->value.place); break;}

        #include "types.h"

    #undef DEF_TYPE

        default: {
            fputs("Wrong type got!", stderr);
            return WRONG_TYPE;
        }
    }

#pragma GCC diagnostic error "-Wnonnull"
#pragma GCC diagnostic error "-Wformat-extra-args"


    if (NODE->type == OPER){
        return PrintOperator(Tree, stream);
    }
    if (NODE->type > OPER){
        printf("%d(" , NODE->type);
        if (NODE->left != NULL){

            Tree->curr_node = &NODE->left;
            RecPrintTree(Tree, stream);
            printf(")(");
        }
        if (NODE->right != NULL){
            Tree->curr_node = &NODE->right;
            RecPrintTree(Tree, stream);
            // printf(")");
        }
        printf(")");
    }

    return TREE_OK;
}

#define PRINTLEFT(need, prior) \
        if (need && (priority(curr_node->left) > prior || (!prior && curr_node->left->type == OPER))) fputc('(', stream);\
        Tree->curr_node = &curr_node->left;\
        RecPrintTree(Tree, stream); \
        if (need && (priority(curr_node->left) > prior || (!prior && curr_node->left->type == OPER))) fputc(')', stream);\

#define PRINTRIGHT(need, prior) \
        if (need && priority(curr_node->right) > prior) fputc('(', stream);\
        Tree->curr_node = &curr_node->right;\
        RecPrintTree(Tree, stream); \
        if (need && priority(curr_node->right) > prior) fputc(')', stream);\

e_err PrintOperator(e_tree *Tree, FILE *stream){

    e_node *curr_node = *Tree->curr_node;

    assert(curr_node->right != NULL);

    // printf("value:%d", curr_node->value.var);

    switch (curr_node->value.var){

#define OPERATOR(name, num, str, nargs, ispref, isfunc, prior, ...)\
\
case (name):{\
    if (ispref){\
\
        fprintf(stream, "%s{", str);\
        if (nargs > 1){\
            PRINTLEFT(false, prior);\
            fputs("}{", stream);\
        }\
        PRINTRIGHT(false, prior);\
        fputc('}', stream);\
    }\
    else\
    {\
        if (nargs == 2) {\
        \
            PRINTLEFT(true, prior);\
        }\
\
        fprintf(stream, "%s", str);\
        \
        if (nargs > 0){\
            if (isfunc) {\
                fputc('{', stream);\
            }\
            else fputc(' ', stream);\
            PRINTRIGHT(true, prior);\
            if (isfunc) {\
                fputc('}', stream);\
            }\
        }\
    }\
    break;\
}

#include "operators.h"

#undef OPERATOR

        default: {
            fputs("Unknown operator", stderr);
            return UNKNOWN_OPERATOR;
        }
    }

    return TREE_OK;
}


e_err ParseExpressionFromFile(e_tree *Tree, const char *srcfile){

    assert(Tree != NULL);

    Buffer Source = {
        .buffer = NULL,
        .readptr = 0,
        .len = 0,
    };

    Source.len = Read(srcfile, &Source.buffer);

    TreeRecParse(&Source, Tree);

    Tree->curr_node = &Tree->head;

    free(Source.buffer);

    return TREE_OK;
}

#define OPERATOR(name, number, str, nargs, ispref, isfunc, prior,...) case name: return prior;

int priority(e_node *node){
    if (node->type == OPER){
        switch (node->value.var){
            #include "operators.h"

            default: printf("WRONG OPERATOR IN prior");
        }
    }

    return lowest_prior;
}

#undef OPERATOR

#define readval src->buffer[src->readptr]

e_err TreeRecParse(Buffer* src, e_tree *Tree){

    char cmd[MaxWordSize] = {};

    TreeParseGetName(src, cmd);

    // printf("cmd:%s\n", cmd);
    // printf("%s", &readval);

    e_node *new_node = NULL;

    if (isdigit(cmd[0])){
       new_node = NewNodeNUM(strtod(cmd, NULL), NULL, NULL);
       *Tree->curr_node = new_node;
    //    printf("NUMBER: %g\n", strtod(cmd, NULL));
    }
    else
        #define OPERATOR(name, number, str, nargs, ispref, isfunc, ...) \
            if (!stricmp(str, cmd)) {\
                new_node = NewNodeOPER(name, NULL, NULL);\
                *Tree->curr_node = new_node;\
                \
                if (nargs > 1){\
                    Tree->curr_node = &new_node->left;\
                    TreeRecParse(src, Tree);\
                }\
\
                if (nargs > 0){\
                    Tree->curr_node = &new_node->right;\
                    TreeRecParse(src, Tree);\
                }\
                } else

        #include "operators.h"

    #undef OPERATOR

    ///else
    {
        new_node = NewNodeVAR(cmd[0], NULL, NULL);
        *Tree->curr_node = new_node;
        // printf("VAR");
    }

    return TREE_OK;
}



e_err TreeParseGetName(Buffer *src, char *name){

    // for (;readval != '('; src->readptr++);

    for (;readval == '('; src->readptr++);

    sscanf(src->buffer + src->readptr, "%[^()]", name);
    src->readptr += 1 + strlen(name);
    // printf("name:%s , readval: %c\n", name, readval);

    for (;readval == ')'; src->readptr++);

    return TREE_OK;
}



e_err ETreeDerivate(e_tree *Tree, e_tree *DerTree){

    assert(Tree != NULL);
    assert(Tree->head != NULL);
    assert(DerTree != NULL);
    // assert(DerTree->head != NULL);

    DerTree->head = ETreeNodeDerivate(DerTree, Tree->head);
    Tree->curr_node = &Tree->head;
    DerTree->curr_node = &DerTree->head;

    return TREE_OK;
}

e_node* ETreeNodeDerivate(e_tree *DerTree, e_node *curr_node){

    assert(curr_node != NULL);

    e_node * new_node = NULL;

    switch (curr_node->type){

        case NUM:
            new_node = NewNodeNUM(0, NULL, NULL);
            break;

        case VAR:
            new_node = NewNodeNUM(1, NULL, NULL);
            break;

        case OPER:

        #define OPERATOR(name, number, str, nargs, ispref, isfunc, prior, diff_action, ...) case name: diff_action; break;

            switch (curr_node->value.var) {

                #include "operators.h"

                default:
                    printf("Found Unknown Operator!!!\n");
            }

            break;

        #undef OPERATOR

        default: printf("Unknown TYPE!, %d", curr_node->type);

    }

    DerTree->curr_node = &new_node;

    if (new_node->left && new_node->right){

        fprintf(outptr, "Итак, следующий шаг: \n");
        PrintTree(DerTree);
        // fprintf(outptr, "\n\\end{equation} \n");
    }

    DerTree->curr_node = NULL;

    return new_node;
}

e_node* ETreeNodeCopy(e_node *node){

    if (node == NULL) return NULL;

    e_node *new_node = NULL;

    switch (node->type){

        #define DEF_TYPE(name,num, type, place, ...) \
        \
        case name:\
            new_node = NewNode##name((type) node->value.place, ETreeNodeCopy(node->left), ETreeNodeCopy(node->right));\
            break;

        #include "types.h"

        #undef DEF_TYPE

        default: printf("Unknown Type: %d", node->type);
    }

    return new_node;
}

bool is_func(e_node *node){

    bool was_var = false;

    if (node == NULL) return was_var;
    if (node->type == VAR) return true;

    was_var = is_func(node->left) || is_func(node->right);

    return was_var;
}

double compute_node(e_node *node){

    if (node == NULL) return NAN;

    if (node->type == NUM) return node->value.number;

    if (node->type == VAR) return NAN;

    double x = compute_node(node->left);
    double y = compute_node(node->right);
    double z = NAN;

    switch (node->value.var) {

#define OPERATOR(name, number, str, nargs, ispref, isfunc, prior, diff_action, action, ...) case name: action; break;
    #include "operators.h"
#undef OPERATOR

    default: printf("Unknown Operator in Compute");
    }

    return z;
}

e_err ETreeSimplifier(e_tree* Tree){

    Tree->curr_node = &Tree->head;

    while (ETreeRecSimplifier(Tree) > 0);

    return TREE_OK;
}


int ETreeRecSimplifier(e_tree *Tree){

    assert(Tree->curr_node != NULL);

    e_node *curr_node = *Tree->curr_node;
    e_node **save = Tree->curr_node;

    if (curr_node == NULL || curr_node->type != OPER) return TREE_OK;

    if (!is_func(curr_node)) {
        *Tree->curr_node = NewNodeNUM(compute_node(curr_node), NULL, NULL);
        NodeRecFree(curr_node);
        return BIGINT;

    }
    int actions = 0;

    Tree->curr_node = &curr_node->left;
    actions += ETreeRecSimplifier(Tree);

    Tree->curr_node = &curr_node->right;
    actions += ETreeRecSimplifier(Tree);

    Tree->curr_node = save;

    switch (curr_node->value.var){

#define OPERATOR(name, number, str, nargs, ispref, isfunc, prior, diff_action, action, simplifier, ...) case name: simplifier; break;
    #include "operators.h"
#undef OPERATOR

    default: printf("Unknown operator in simplifier");
    }
    return actions;
}



bool IsZeroNode(e_node *node){
    return (node->type == NUM && IsZero(node->value.number));
}

bool IsNodeEqual(e_node *node, double num){
    return !is_func(node) && IsZero(compute_node(node) - num);
}

e_err NodeRecFree(e_node* node){

    if (node == NULL) return TREE_OK;

    NodeRecFree(node->left);
    NodeRecFree(node->right);

    free(node);
    return TREE_OK;
}

int printptr(e_tree *Tree){

    if (*Tree->curr_node == NULL) return 0;

    printf("(%p", *Tree->curr_node);

    e_node *sv = *Tree->curr_node;

    Tree->curr_node = &sv->left;
    printptr(Tree);

    Tree->curr_node = &sv->right;
    printptr(Tree);

    printf(")");

    return 1;
}
