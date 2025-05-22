#include <assert.h>

#include "IR.h"

void ProcessList(IRList* ir, int is_main);

void ProcessIR(IRFuncs* funcs){

    for (int i = 0; i < funcs->nfuncs; i++){
        ProcessList(funcs->funcs[i].list, !strcmp("main", funcs->funcs[i].name));
    }
}

#define type(curr) curr->node->type

void ProcessList(IRList* ir, int is_main){
    if (!ir) return;
    LList* curr = ir->start;
    while (curr != NULL && curr->next != NULL){

        LList* next = curr->next;
        // printf("%p\n", next);

        if (type(curr) == IR_RES_PUSH &&
            type(next) == IR_POP_REG){
            if (next->node->pop_reg.num == REG_RAX) {
                curr->prev->next = next->next;
                next->next->prev = curr->prev;
                curr = next->next;
                continue;
            }
            IRNode* n = create_mov(REG_RAX, next->node->pop_reg.num);
            LList* newnode = (LList*) calloc (1, sizeof(LList));

            newnode->node = n;
            newnode->prev = curr->prev;
            assert(curr->prev != NULL);
            curr->prev->next = newnode;
            newnode->next = next->next;
            next->next->prev = newnode;
            // free(curr->node); free(curr);
            // free(next->node); free(next);
            curr = newnode->next;
            continue;
        }

        if (type(curr) == IR_INPUT){

            IRNode* n = create_call("read", 0);
            LList* newnode = (LList*) calloc (1, sizeof(LList));

            newnode->node = n;
            newnode->prev = curr->prev;
            assert(curr->prev != NULL);
            curr->prev->next = newnode;
            newnode->next = next;
            next->prev = newnode;
            // free(curr->node); free(curr);
            curr = newnode->next;
            continue;
        }

        if (type(curr) == IR_PRINT){

            IRNode* n = create_call("print", 0);
            LList* newnode = (LList*) calloc (1, sizeof(LList));

            newnode->node = n;
            newnode->prev = curr->prev;
            assert(curr->prev != NULL);
            curr->prev->next = newnode;
            newnode->next = next;
            next->prev = newnode;
            // free(curr->node); free(curr);
            curr = newnode->next;
            continue;
        }

        curr = curr->next;
    }

    if (is_main && curr != NULL){

        IRNode* n = create_call("end", 0);
        LList* newnode = (LList*) calloc (1, sizeof(LList));

        newnode->node = n;
        newnode->prev = curr->prev;
        assert(curr->prev != NULL);
        curr->prev->next = newnode;
        newnode->next = NULL;
        // free(curr->node); free(curr);
    }
}
