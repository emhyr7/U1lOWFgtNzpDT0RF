/* (type, identifier, body, syntax) */

X(0, undefined, {}, "")

/* expressions */
#define UNARY_NODE_BODY   { node *node; }
#define BINARY_NODE_BODY  { node *left; node *right; }
#define TERNARY_NODE_BODY { struct BINARY_NODE_BODY; struct UNARY_NODE_BODY; }

X(0, unary,   UNARY_NODE_BODY,   "operator node")
X(0, binary,  BINARY_NODE_BODY,  "node operator node")
X(0, ternary, TERNARY_NODE_BODY, "node operator node [operator node]")

/* scoped expressions */
X(1, subexpression, UNARY_NODE_BODY, "`(` node `)`")
X(1, indexation,    UNARY_NODE_BODY, "`[` node `]`")

/* logical expressions */
X(1, logical_negation,           UNARY_NODE_BODY,  "`!` node")
X(2, logical_conjunction,        BINARY_NODE_BODY, "node `&&` node")
X(2, logical_disjunction,        BINARY_NODE_BODY, "node `||` node")
X(2, logical_equality,           BINARY_NODE_BODY, "node `==` node")
X(2, logical_inequality,         BINARY_NODE_BODY, "node `!=` node")
X(2, logical_majority,           BINARY_NODE_BODY, "node `>` node")
X(2, logical_minority,           BINARY_NODE_BODY, "node `<` node")
X(2, logical_inclusive_majority, BINARY_NODE_BODY, "node `>=` node")
X(2, logical_inclusive_minority, BINARY_NODE_BODY, "node `<=` node")

/* arithmetic expressions */
X(1, negation,       UNARY_NODE_BODY,  "`-` node")  
X(2, addition,       BINARY_NODE_BODY, "node `+` node") 
X(2, subtraction,    BINARY_NODE_BODY, "node `-` node") 
X(2, multiplication, BINARY_NODE_BODY, "node `*` node") 
X(2, division,       BINARY_NODE_BODY, "node `/` node") 
X(2, modulus,        BINARY_NODE_BODY, "node `%` node") 

/* bitwise expressions */
X(1, bitwise_negation,              UNARY_NODE_BODY,  "`~` node")
X(2, bitwise_conjunction,           BINARY_NODE_BODY, "node `&` node")
X(2, bitwise_disjunction,           BINARY_NODE_BODY, "node `|` node")
X(2, bitwise_exclusive_disjunction, BINARY_NODE_BODY, "node `^` node")
X(2, bitwise_left_shift,            BINARY_NODE_BODY, "node `<<` node")
X(2, bitwise_right_shift,           BINARY_NODE_BODY, "node `>>` node")

/* assignment expressions */
X(2, assignment,                               BINARY_NODE_BODY, "node `=` node")
X(2, addition_assignment,                      BINARY_NODE_BODY, "node `+=` node")
X(2, subtraction_assignment,                   BINARY_NODE_BODY, "node `-=` node")
X(2, multiplication_assignment,                BINARY_NODE_BODY, "node `*=` node")
X(2, division_assignment,                      BINARY_NODE_BODY, "node `/=` node")
X(2, modulus_assignment,                       BINARY_NODE_BODY, "node `%=` node")
X(2, bitwise_conjunction_assignment,           BINARY_NODE_BODY, "node `&=` node")
X(2, bitwise_disjunction_assignment,           BINARY_NODE_BODY, "node `|=` node")
X(2, bitwise_exclusive_disjunction_assignment, BINARY_NODE_BODY, "node `^=` node")
X(2, bitwise_left_shift_assignment,            BINARY_NODE_BODY, "node `<<=` node")
X(2, bitwise_right_shift_assignment,           BINARY_NODE_BODY, "node `>>=` node")

/* other expressions */
X(1, address,     UNARY_NODE_BODY,   "`@` node")
X(2, invocation,  BINARY_NODE_BODY,  "node node")
X(2, resolution,  BINARY_NODE_BODY,  "node `.` node")
X(2, list,        BINARY_NODE_BODY,  "node `,` node")
X(2, declaration, BINARY_NODE_BODY,  "node `:` node")
X(3, procedure,   TERNARY_NODE_BODY, "node `->` node [scope]")
X(3, condition,   TERNARY_NODE_BODY, "node `?` node [`:` node]")

#undef TERNARY_NODE_BODY
#undef BINARY_NODE_BODY
#undef UNARY_NODE_BODY

/* literals */
X(0, scope,      { node **nodes; uint nodes_count; }, "`{` {node `;`} `}`")
X(0, identifier, { utf8 *data; uint size; },          "identifier")
X(0, text,       { utf8 *data; uint size; },          "text")
X(0, digital,    { uint64 value; },                   "digital|hexadecimal|binary")

/* to use a single precision floating-point literal, a pragma is used */
X(0, decimal,    { real64 value; },                   "decimal|scientific")

/* pragmas can modify the syntax tree, and are omitted in the final syntax tree */
X(0, pragma,     { pragma value; },                   "`#` identifier ...")
