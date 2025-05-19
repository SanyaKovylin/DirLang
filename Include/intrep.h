#define IR(name, ...) IR_##name,

typedef enum IRNodeType {

    #include "IR.h"
} IRNodeType;

#undef IR
