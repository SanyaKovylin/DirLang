#ifndef LEXSINT_H_INCLUDED
#define LEXSINT_H_INCLUDED

#include "expr_tree.h"
#include "utils.h"

struct Tokens {
    e_node** buffer;
    int capacity;
    int curr;
};

struct ReservedNames {
    char** names;
    int num;
    int capacity;
};

void SetResNames(ReservedNames names);

const int lenres = COUNT_OPER;
#undef OPERATOR
e_err FParseInf(const char* srcfile, e_tree *Tree);

e_err LexParse(const char* srcfile, Tokens **nodes);
e_err ParseNext(Buffer *src, Tokens *Nodes);
e_node *GetNumber(Buffer *src);
e_node *GetWord(Buffer *src);
e_node *GetBrace(Buffer * src);
bool CheckReserve(char *word, e_node **node);
e_node *GetOperator(Buffer* src);

e_err SintParse(e_tree *Tree, Tokens* Nodes);

bool TryGetException(e_node** start, e_node*** end);
bool TryGetGram(e_node** node, e_node** StartNode, e_node*** EndNode);
bool TryGetExpr(e_node** node, e_node** StartNode, e_node*** EndNode);
bool TryGetTerm(e_node** node, e_node** StartNode, e_node*** EndNode);
bool TryGetFact(e_node** node, e_node** StartNode, e_node*** EndNode);
bool TryGetPrim(e_node** node, e_node** StartNode, e_node*** EndNode);
bool TryGetAdd(e_node **place, e_node** start, e_node*** end);
bool TryGetMul(e_node **place, e_node** start, e_node***end);
bool TryGetFunc(e_node **place, e_node** start, e_node*** end);
bool TryGetN(e_node **place, e_node** start, e_node*** end);
bool TryGetVar(e_node **place, e_node** start, e_node*** end);
bool TryGetOBrace(e_node** start, e_node*** end);
bool TryGetCBrace(e_node** start, e_node*** end);
// bool TryGetNum(e_node **node, Buffer *src);
bool TryGetKey(e_node **node, e_node** start, e_node*** end);
#endif
