
//OPERATOR(name, number, str, nargs, ispref, isfunc, prior, diff_action, action, simplifier, alterstr)

OPERATOR(ADD,    0, "+",        2, false, false, 3 , {
    //e_node *new_node = NULL;
    //curr_node = *Tree->curr_node;
    new_node = NewNodeOPER(ADD, ETreeNodeDerivate(DerTree, curr_node->left), ETreeNodeDerivate(DerTree, curr_node->right));
}, {
    // x, y are given ealier
    // get double z
    z = x + y;
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

    if (IsZeroNode(curr_node->left)) {*Tree->curr_node = curr_node->right; actions++;}
    else if (IsZeroNode(curr_node->right)) {*Tree->curr_node = curr_node->left; actions++;}
}, "")
OPERATOR(SUB,    1, "-",        2, false, false, 3, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    new_node = NewNodeOPER(SUB, ETreeNodeDerivate(DerTree, curr_node->left), ETreeNodeDerivate(DerTree, curr_node->right));
}, {
    // x, y are given ealier
    // get double z
    z = x - y;
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

    if (IsZeroNode(curr_node->left)) {*Tree->curr_node = curr_node->right; actions++;}
    else if (IsZeroNode(curr_node->right)) {*Tree->curr_node = NewNodeOPER(MUL,
                                                                       NewNodeNUM(-1,NULL,NULL),
                                                                       curr_node->left); actions++;}
}, "")
OPERATOR(MUL,    2, "\\cdot",   2, false, false, 2, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    new_node = NewNodeOPER(ADD,
                    NewNodeOPER(MUL,ETreeNodeDerivate(DerTree, curr_node->left), ETreeNodeCopy(curr_node->right)),
                    NewNodeOPER(MUL,ETreeNodeDerivate(DerTree, curr_node->right), ETreeNodeCopy(curr_node->left)));
}, {
    // x, y are given ealier
    // get double z
    z = x * y;
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

    if (IsZeroNode(curr_node->left))      {*Tree->curr_node = NewNodeNUM(0, NULL,NULL); actions++;}
    else if (IsNodeEqual(curr_node->left, 1))  {*Tree->curr_node = curr_node->right; actions++;}
    else if (IsZeroNode(curr_node->right))     {*Tree->curr_node = NewNodeNUM(0, NULL, NULL); actions++;}
    else if (IsNodeEqual(curr_node->right, 1)) {*Tree->curr_node = curr_node->left; actions++;}
}, "*")
OPERATOR(DIV,    3, "\\frac",   2, true , true , 2, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    new_node =  NewNodeOPER(DIV,
                    NewNodeOPER(SUB,
                        NewNodeOPER(MUL,ETreeNodeDerivate(DerTree, curr_node->left), ETreeNodeCopy(curr_node->right)),
                        NewNodeOPER(MUL,ETreeNodeDerivate(DerTree, curr_node->right), ETreeNodeCopy(curr_node->left))),
                    NewNodeOPER(POW,
                        ETreeNodeCopy(curr_node->right),
                        NewNodeNUM(2, NULL, NULL)));
}, {
    // x, y are given ealier
    // get double z
    z = x / y;
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

    if (IsZeroNode(curr_node->left))      {*Tree->curr_node = NewNodeNUM(0, NULL,NULL); actions++;}
    else if (IsZeroNode(curr_node->right))     {printf("Division by zero!!!\n"); actions++;}
    else if (IsNodeEqual(curr_node->right, 1)) {*Tree->curr_node = curr_node->left; actions++;}
}, "/")

OPERATOR(POW,    4, "^",        2, false, true , 0, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    if (is_func(curr_node->right)){
        if (is_func(curr_node->left)){

            new_node = NewNodeOPER(MUL,
                        ETreeNodeCopy(curr_node),
                        ETreeNodeDerivate(DerTree,
                            NewNodeOPER(MUL,
                                NewNodeOPER(LN,NULL,
                                    ETreeNodeCopy(curr_node->left)),
                                ETreeNodeCopy(curr_node->right))));
        }
        else{
            new_node = NewNodeOPER(MUL,
            NewNodeOPER(LN, NULL,
                NewNodeNUM(compute_node(curr_node->left), NULL, NULL)),
            NewNodeOPER(MUL,
                ETreeNodeDerivate(DerTree, curr_node->right),
                ETreeNodeCopy(curr_node)));
        }
    }
    else{
        new_node = NewNodeOPER(MUL,
                        NewNodeOPER(MUL,
                            NewNodeNUM(compute_node(curr_node->right), NULL, NULL),
                            ETreeNodeDerivate(DerTree, curr_node->left)),
                        NewNodeOPER(POW,
                            ETreeNodeCopy(curr_node->left),
                            NewNodeOPER(SUB,
                                NewNodeNUM(compute_node(curr_node->right), NULL, NULL),
                                NewNodeNUM(1, NULL, NULL))));
    }
}, {
    // x, y are given ealier
    // get double z
    z = pow(x, y);
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

    if (IsZeroNode(curr_node->left))      {*Tree->curr_node = NewNodeNUM(0, NULL, NULL); actions++;}
    if (IsNodeEqual(curr_node->right, 1)) {*Tree->curr_node = curr_node->left; actions++;}
    if (IsNodeEqual(curr_node->left, 1))  {*Tree->curr_node = NewNodeNUM(1, NULL, NULL); actions++;}
    if (IsZeroNode(curr_node->right))     {*Tree->curr_node = NewNodeNUM(1, NULL, NULL); actions++;}
}, "")

OPERATOR(SIN,    5, "\\sin",    1, false, true , 1, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    new_node = NewNodeOPER(MUL,
                    ETreeNodeDerivate(DerTree, curr_node->right),
                    NewNodeOPER(COS, NULL, ETreeNodeCopy(curr_node->right)));
}, {
    // x, y are given ealier
    // get double z
    z = sin(y);
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

    if (IsZeroNode(curr_node->right)) {*Tree->curr_node = NewNodeNUM(0, NULL, NULL); actions++;}
}, "sin")
OPERATOR(COS,    6, "\\cos",    1, false, true , 1, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    new_node = NewNodeOPER(MUL,
                    ETreeNodeDerivate(DerTree, curr_node->right),
                    NewNodeOPER(MUL,
                        NewNodeNUM(-1, NULL, NULL),
                        NewNodeOPER(SIN, NULL, ETreeNodeCopy(curr_node->right))));
}, {
    // x, y are given ealier
    // get double z
    z = cos(y);
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

    if (IsZeroNode(curr_node->right)) {*Tree->curr_node = NewNodeNUM(1, NULL, NULL); actions++;}
}, "cos")
OPERATOR(ARCSIN, 7, "\\arcsin", 1, false, true , 1, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    // new_node = NewNodeOPER(DIV,
    //                 ETreeNodeDerivate(DerTree, curr_node->right),
    //                 NewNodeOPER(POW,
    //                     NewNodeOPER(SUB,
    //                         NewNodeNUM(1, NULL, NULL),
    //                         NewNodeOPER(POW,
    //                             ETreeNodeCopy(curr_node->right),
    //                             NewNodeNUM(2, NULL, NULL))), NewNodeNUM(0.5, NULL, NULL)));

    new_node = NewNodeOPER(DIV,
                    ETreeNodeDerivate(DerTree, curr_node->right),
                    NewNodeOPER(SQRT, NULL,
                        NewNodeOPER(SUB,
                            NewNodeNUM(1, NULL, NULL),
                            NewNodeOPER(POW,
                                ETreeNodeCopy(curr_node->right),
                                NewNodeNUM(2, NULL, NULL)))));
}, {
    // x, y are given ealier
    // get double z
    z = asin(y);
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

    if (IsZeroNode(curr_node->right)) {*Tree->curr_node = NewNodeNUM(0, NULL, NULL); actions++;}
}, "arcsin")
OPERATOR(ARCCOS, 8, "\\arccos", 1, false, true , 1, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    new_node = NewNodeOPER(MUL,
                    NewNodeNUM(-1, NULL, NULL),
                    NewNodeOPER(DIV,
                        ETreeNodeDerivate(DerTree, curr_node->right),
                        NewNodeOPER(POW,
                            NewNodeOPER(SUB,
                                NewNodeNUM(1, NULL, NULL),
                                NewNodeOPER(POW,
                                    ETreeNodeCopy(curr_node->right),
                                    NewNodeNUM(2, NULL, NULL))), NewNodeNUM(0.5,NULL,NULL))));
}, {
    // x, y are given ealier
    // get double z
    z = acos(y);
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

    if (IsZeroNode(curr_node->right)) {*Tree->curr_node = NewNodeOPER(PI, NULL, NULL); actions++;}
}, "arccos")
OPERATOR(LOG,    9, "\\log_",   2, true , true , 1, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    new_node = NewNodeOPER(DIV,
                   ETreeNodeDerivate(DerTree, NewNodeOPER(LN, NULL, curr_node->right)),
                    ETreeNodeDerivate(DerTree, NewNodeOPER(LN, NULL, curr_node->left)));
}, {
    // x, y are given ealier
    // get double z
    z = log(y)/log(x);
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

    if (IsZeroNode(curr_node->left)) {printf("Base is nothing!!!\n"); actions++;}
    if (IsNodeEqual(curr_node->right, 1)) {*Tree->curr_node = NewNodeNUM(0, NULL, NULL); actions++;}

}, "log")

OPERATOR(LN,    10, "\\ln",     1, false, true , 1, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    new_node = NewNodeOPER(DIV, ETreeNodeDerivate(DerTree, curr_node->right), ETreeNodeCopy(curr_node->right));
}, {
    z = log(y);
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

    if (IsZeroNode(curr_node->right)) {*Tree->curr_node = NewNodeNUM(0, NULL, NULL); actions++;}
}, "ln")

OPERATOR(E,    11, "\\e",     0, false, false , 0, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    new_node = NewNodeNUM(1, NULL, NULL);
}, {
    // x, y are given ealier
    // get double z
    z = M_E;
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;
}, "e")

OPERATOR(PI,    12, "\\pi",     0, false, false , 0, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    new_node = NewNodeNUM(1, NULL, NULL);
}, {
    // x, y are given ealier
    // get double z
    z = M_PI;
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;

}, "pi")

OPERATOR(SQRT,    13, "\\sqrt",     1, true, false , 0, {
    //e_node *new_node = NULL;
    // curr_node = *Tree->curr_node;
    new_node = NewNodeOPER(DIV,
                 NewNodeNUM(1,NULL,NULL),
                 NewNodeOPER(MUL,
                    NewNodeNUM(2, NULL,NULL),
                    ETreeNodeDerivate(DerTree, curr_node->right)));
}, {
    // x, y are given ealier
    // get double z
    z = sqrt(y);
}, {
    // int actions = 0
    //curr_node = *Tree->curr_node;
}, "sqrt")
