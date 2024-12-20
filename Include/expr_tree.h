#ifndef EXPR_TREE_H_INCLUDED
#define EXPR_TREE_H_INCLUDED

#define _USE_MATH_DEFINES_ 10

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "utils.h"

#define OPERATOR(name, num, ...)  name = num,
typedef enum Operators {
    #include "operators.h"
    COUNT_OPER,
} e_oper;
#undef OPERATOR

typedef union NodeValue {
    uint64_t var;
    double number;
} e_value;

#define DEF_TYPE(name, num, ...) name = num,
typedef enum NodeType {
    #include "types.h"
} e_type;
#undef DEF_TYPE

typedef struct ExprTreeNode {
    e_value value;
    e_type type;
    ExprTreeNode *left;
    ExprTreeNode *right;
} e_node;

typedef struct ExprTreeStruct {
    const char* name;
    e_node *head;       //is actually a head? but to simplify the algorithm is right
    e_node **curr_node; // for recursion implementing
    int nnodes;
} e_tree;

typedef struct ExprTreePath {
    e_node **Nodes;
    int *truth;
    size_t capacity;
    size_t depth;
} e_path;

typedef enum ExprTreeError{
    TREE_OK = 0,
    WRONG_TYPE = 1,
    UNKNOWN_OPETOR = 2,
    FOUND_VARIABLE_IN_COMPUTE = 3,
    CALLOC_ERR = 4,
} e_err;



#define NODE (*Tree->curr_node)
#define NAME (char*) NODE->value

const int MaxWordSize = 20;

e_err TreeCtor(e_tree *Tree, e_node* head, const char* name);
// e_err TreeAddNode(e_tree *Tree, void* value, size_t valsize);

#define DEF_TYPE(name, num, type, ...) e_node* NewNode##name(type value, e_node *Left, e_node* Right);
#include "types.h"
#undef DEF_TYPE

e_err PrintTree(e_tree *Tree);
e_err RecPrintTree(e_tree *Tree, FILE *stream);
e_err PrintOperator(e_tree *Tree, FILE *stream);

e_err ParseExpressionFromFile(e_tree *Tree,const char *srcfile);
e_err TreeRecParse(Buffer* src, e_tree *Tree);
e_err TreeParseGetName(Buffer *src, char *name);

int priority(e_node* op);
const int lowest_prior = 0;

e_err ETreeDerivate(e_tree *Tree, e_tree *DerTree);
e_node* ETreeNodeDerivate(e_tree *DerTree, e_node *curr_node);

e_node* ETreeNodeCopy(e_node *node);

bool is_func(e_node *node);
double compute_node(e_node *node);
bool IsNodeEqual(e_node *node, double num);
bool IsZeroNode(e_node *node);
const int BIGINT = 100000000;

e_err ETreeSimplifier(e_tree* Tree);
int ETreeRecSimplifier(e_tree *Tree);

e_err NodeRecFree(e_node* node);

int printptr(e_tree *Tree);
e_err SetOutputFile(const char* file);
void PrintPreamble(FILE* stream);
void PrintExit();

#endif //EXPR_TREE_H_INCLUDED
