#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "lexsint.h"
#include "parser.h"

#define readval src->buffer[src->readptr]
#define rptr src->buffer + src->readptr
#define STRIP while (isspace(readval)) src->readptr++;
#define SKIP(chr) STRIP; if(readval == chr) src->readptr++; STRIP;

const int startalloc = 16;

ReservedNames Resnames = {};

struct Oper {
    e_oper num;
    char* name;
    char* altname;
};

#define OPERATOR(name, number, str, nargs, ispref, isfunc, prior, diff_action, action, simplifier, alterstr)\
    char st##name[] = str;\
    char alt##name[] = alterstr;

   #include "operators.h"

#undef OPERATOR

#define OPERATOR(name, number, str, nargs, ispref, isfunc, prior, diff_action, action, simplifier, alterstr)\
    {name, st##name, alt##name},

Oper Reserve[] = {
    #include "operators.h"
};

#undef OPERATOR

struct Key {
    Keywords num;
    char* name;
};

#define KEYWORD(name, num, str, ...) char st##name[] = str;

#include "keywords.h"

#undef KEYWORD

#define KEYWORD(name, num, ...) {name, st##name},

Key Keys[] = {
    #include "keywords.h"
};

#undef KEYWORD

e_err FParseInf(const char* srcfile, e_tree *Tree){

    Tokens *n = NULL;
    // printf("\nParsing .... %s\n", srcfile);
    e_node** save = Tree->curr_node;

    LexParse(srcfile, &n);
    // printf("Tokens %p\n", n);
    if (n != NULL)
        SintParse(Tree, n);

    Tree->curr_node = save;
    return TREE_OK;
}

e_err LexParse(const char* srcfile, Tokens **nodes){

    Buffer buffer = {
        .buffer = NULL,
        .readptr = 0,
        .len = 0,
    };

    // Tokens NNodes = {
    //     .buffer = (e_node**) calloc (startalloc, sizeof(e_node*)),
    //     .capacity = startalloc,
    //     .curr = 0,
    // };

    Tokens *Nodes= (Tokens*) calloc (1, sizeof(Tokens));
    Nodes->buffer = (e_node**) calloc (startalloc, sizeof(e_node*));
    Nodes->capacity = startalloc;


    if (Nodes->buffer == NULL) return CALLOC_ERR;

    buffer.len = Read(srcfile, &buffer.buffer);
    if (buffer.buffer == NULL) return CALLOC_ERR;
    // if (buffer.len == 0) return TREE_OK;

    // printf("buffer: %s", buffer.buffer);

    while (buffer.len > buffer.readptr){
        ParseNext(&buffer, Nodes);
    }

    Nodes->buffer[Nodes->curr] = NewNodeVAR('\0', NULL, NULL);
    *nodes = Nodes;

    return TREE_OK;
}

e_err ParseNext(Buffer *src, Tokens *Nodes){

    STRIP;
    e_node* new_node = NULL;

// char char char -> lexem lexem lexem -> node node node

//
//             type: ++
//             operation
// +-----+     +-----+     +-----++-----++-----++-----++-----++-----+
// | 123 |     | ++  |     |
// +-----+     +-----+     +-----++-----++-----++-----++-----++-----+
//  number
// integer
// value: 123

    // TryGetNumber -s

    // if (!tryGetNumber() &&
    //     !tryGetWord() &&
    //     !tryGetBrace() &&
    //     !tryOperator() ) {
    //     printf("ОШИБКА"); // chain of responsibility
    // }

    if (readval == '\0'){
        return TREE_OK;
    }
    else if (isdigit(readval) || (readval == '-' && isdigit(*(rptr + 1)))){
        new_node = GetNumber(src);
        // printf("NUM\n");
    }
    else if (isalpha(readval) || readval == '\\'){
        new_node = GetWord(src);
        // printf("WORD\n");
    }
    else if (readval == '(' || readval == ')'){
        new_node = GetBrace(src);
        // printf("Brace\n");
    }
    else {
        new_node = GetOperator(src);
        // printf("Oper\n");
    }

    if (new_node == NULL) return UNKNOWN_OPETOR;

    Nodes->buffer[Nodes->curr++] = new_node;

    // printf("Node type: %d\n", new_node->type);

    if (Nodes->capacity == Nodes->curr){
        Nodes->buffer = (e_node**) Resize (Nodes->buffer, sizeof(e_node*), Nodes->capacity, true);
        Nodes->capacity *= ResizeScale;
    }
    STRIP;

    return TREE_OK;
}

e_node *GetNumber(Buffer *src){

    char *end = NULL;
    e_node* node = NewNodeNUM(strtod(rptr, &end), NULL, NULL);
    src->readptr = end - src->buffer;

    return node;
}

e_node *GetWord(Buffer *src){

    size_t len = 0;
    while (isalpha(*(rptr + len)) || *(rptr + len) == '\\') len++;

    char word[100] = {};

    memcpy(word, rptr, len);
    word[len] = '\0';

    src->readptr += len;

    e_node* node = NULL;

    bool was_res = CheckReserve(word, &node);

    if (!was_res) node = NewNodeVAR(word[0], NULL, NULL);
    // printf("%s", word);

    return node;
}

e_node *GetBrace(Buffer * src){

    char brace = readval;
    src->readptr++;

    return NewNodeVAR(brace, NULL, NULL);
}

bool CheckReserve(char *word, e_node **node){

    bool was_res = false;
    for (int i = 0; i < lenres && !was_res; i++){
        if (!stricmp(word, Reserve[i].name) || !stricmp(word, Reserve[i].altname)){

            was_res = true;
            *node = NewNodeOPER(Reserve[i].num, NULL, NULL);
        }
    }

    for (int i = 0; i < COUNT_KEYWORDS && !was_res; i++){
        if (!stricmp(word, Keys[i].name)){
            was_res = true;
            *node = NewNodeKEYWORD(Keys[i].num, NULL, NULL);
        }
    }

    for (int i = 0; i < Resnames.num && !was_res; i++){
        if (!stricmp(Resnames.names[i], word)){
            was_res = true;
            *node = NewNodeFUNC(i, NULL, NULL);
        }
    }

    return was_res;
}


e_node *GetOperator(Buffer* src){

    char oper[2] = {readval, '\0'};
    src->readptr++;

    e_node *node = NULL;
    CheckReserve(oper, &node);

    return node;
}


// Sintacsis =========================================================================================

e_node** debugpoint = NULL;

e_err SintParse(e_tree* Tree, Tokens* Nodes){

    debugpoint = Nodes->buffer;

    e_node **e = NULL;
    TryGetExpr(Tree->curr_node, Nodes->buffer, &e);
    // Tree->curr_node = &Tree->head;

    return TREE_OK;
}

#define IS_OBRACE (Nodes->buffer[i]->type == VAR && Nodes->buffer[i]->value.var == '(')
#define IS_CBRACE (Nodes->buffer[i]->type == VAR && Nodes->buffer[i]->value.var == ')')
#define INIT_NODE \
    e_node* curr_node = NULL;\
    e_node* left =  NULL;\
    e_node* right = NULL;

#define AGAIN else *EndNode = StartNode;

#define BUILD_NODE \
    curr_node->left = left;\
    curr_node->right = right;\
    *node = curr_node;

#define POS(x) x - debugpoint

bool TryGetGram(e_node** node, e_node** StartNode, e_node*** EndNode){

    if (TryGetExpr(node, StartNode, EndNode) &&
        TryGetException(*EndNode, EndNode)){

        return true;
    }
    AGAIN;

    return false;
}

bool TryGetExpr(e_node** node, e_node** StartNode, e_node*** EndNode){

    assert(node != NULL);
    INIT_NODE;

    if (TryGetKey(&curr_node, StartNode, EndNode) &&
        TryGetOBrace(*EndNode, EndNode) &&
        TryGetExpr(&right, *EndNode, EndNode) &&
        TryGetCBrace(*EndNode, EndNode)){

        BUILD_NODE;
    }

    if (TryGetTerm(&left, StartNode, EndNode) &&
        TryGetAdd(&curr_node,   *EndNode, EndNode) &&
        TryGetExpr(&right,      *EndNode, EndNode)){

        BUILD_NODE;

        return true;
    }
    AGAIN

    if(TryGetTerm(node, StartNode, EndNode)) {

        return true;
    }

    if(TryGetKey(&curr_node, StartNode, EndNode)) {

        BUILD_NODE;
        return true;
    }

    return false;
}

bool TryGetTerm(e_node** node, e_node** StartNode, e_node*** EndNode){

    INIT_NODE;

    if (TryGetFact(&left, StartNode, EndNode) &&
        TryGetMul(&curr_node,   *EndNode, EndNode) &&
        TryGetTerm(&right,      *EndNode, EndNode)){

        BUILD_NODE;

        return true;
    }
    AGAIN

    if(TryGetFact(node, StartNode, EndNode)) {

        return true;
    }
    AGAIN

    return false;
}

bool TryGetFact(e_node** node, e_node** StartNode, e_node*** EndNode){

    INIT_NODE;
    if ((TryGetPrim(&left, StartNode, EndNode) || true) &&
        TryGetFunc(&curr_node,  *EndNode, EndNode) &&
        TryGetPrim(&right,      *EndNode, EndNode)){

        BUILD_NODE;

        return true;
    }
    AGAIN

    if (TryGetFunc(&curr_node, StartNode, EndNode) &&
        TryGetPrim(&left,       *EndNode, EndNode) &&
        TryGetPrim(&right,      *EndNode, EndNode)){

        BUILD_NODE;

        return true;
    }
    AGAIN

    if (TryGetPrim(node, StartNode, EndNode)) {

        return true;
    }
    AGAIN

    return false;
}

bool TryGetPrim(e_node** node, e_node** StartNode, e_node*** EndNode){

    if (TryGetN(node, StartNode, EndNode)){

        return true;
    }
    AGAIN

    if (TryGetVar(node, StartNode, EndNode)){

        return true;
    }
    AGAIN

    if (TryGetOBrace(StartNode, EndNode) &&
        TryGetExpr(node, *EndNode, EndNode)&&
        TryGetCBrace(*EndNode, EndNode)) {

        return true;
    }
    AGAIN

    return false;
}

#define IS_OPER(x) ((*start)->type == OPER && (*start)->value.var == x)
#define TRUE_CASE \
        *node = *start;\
        *end = start + 1;

bool TryGetAdd(e_node** node, e_node** start, e_node*** end){

    if (IS_OPER(ADD) || IS_OPER(SUB)){

        TRUE_CASE;
        // printf("got add\n");
        return true;
    }


    return false;
}

bool TryGetMul(e_node **node, e_node** start, e_node*** end){

    if (IS_OPER(MUL) || IS_OPER(DIV)){

        TRUE_CASE;
        return true;
    }

    return false;
}

bool TryGetFunc(e_node **node, e_node** start, e_node*** end){

    if (IS_OPER(SIN) ||
        IS_OPER(COS) ||
        IS_OPER(LOG) ||
        IS_OPER(LN) ||
        IS_OPER(ARCCOS) ||
        IS_OPER(ARCSIN) ||
        IS_OPER(POW) ||
        IS_OPER(SQRT)||
        (*start)->type > OPER
       ){

        TRUE_CASE;
        return true;
    }

    return false;
}

bool TryGetKey(e_node **node, e_node** start, e_node*** end){

    if ((*start)->type == KEYWORD){

        TRUE_CASE;
        return true;
    }

    return false;
}

bool TryGetN(e_node **node, e_node** start, e_node*** end){

    if ((*start)->type == NUM){

        TRUE_CASE;
        return true;
    }

    return false;
}

bool TryGetVar(e_node **node, e_node** start, e_node*** end){

    e_node **e = NULL;

    if ((*start)->type == VAR &&
    !TryGetOBrace(start, &e) &&
    !TryGetCBrace(start, &e) &&
    (*start)->value.var > 0){

        TRUE_CASE;
        return true;
    }

    return false;
}

bool TryGetOBrace(e_node** start, e_node*** end){

    if ((*start)->type == VAR && (*start)->value.var == '('){

        *end = start + 1;
        return true;
    }

    return false;
}

bool TryGetCBrace(e_node** start, e_node*** end){

    if ((*start)->type == VAR && (*start)->value.var == ')'){

        *end = start + 1;
        return true;
    }

    return false;
}

bool TryGetException(e_node** start, e_node*** end){

    if ((*start)->type == VAR && (*start)->value.var == '\0'){

        *end = start + 1;
        return true;
    }

    return false;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void SetResNames(ReservedNames names){
    Resnames = names;
}
