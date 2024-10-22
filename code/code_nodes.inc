/* (identifier, body, syntax) */

X(undefined, {}, "")

/* expressions */
#define UNARY_NODE_BODY   { node *node; }
#define BINARY_NODE_BODY  { node *left; node *right; }
#define TERNARY_NODE_BODY { struct BINARY_NODE_BODY; struct UNARY_NODE_BODY; }

X(unary,   UNARY_NODE_BODY,   "operator node")
X(binary,  BINARY_NODE_BODY,  "node operator node")
X(ternary, TERNARY_NODE_BODY, "node operator node [operator node]")

/* scoped expressions */
X(subexpression, UNARY_NODE_BODY, "`(` node `)`")
X(indexation,    UNARY_NODE_BODY, "`[` node `]`")

/* logical expressions */
X(logical_negation,           UNARY_NODE_BODY,  "`!` node")
X(logical_conjunction,        BINARY_NODE_BODY, "node `&&` node")
X(logical_disjunction,        BINARY_NODE_BODY, "node `||` node")
X(logical_equality,           BINARY_NODE_BODY, "node `==` node")
X(logical_inequality,         BINARY_NODE_BODY, "node `!=` node")
X(logical_majority,           BINARY_NODE_BODY, "node `>` node")
X(logical_minority,           BINARY_NODE_BODY, "node `<` node")
X(logical_inclusive_majority, BINARY_NODE_BODY, "node `>=` node")
X(logical_inclusive_minority, BINARY_NODE_BODY, "node `<=` node")

/* arithmetic expressions */
X(negation,       UNARY_NODE_BODY,  "`-` node")  
X(addition,       BINARY_NODE_BODY, "node `+` node") 
X(subtraction,    BINARY_NODE_BODY, "node `-` node") 
X(multiplication, BINARY_NODE_BODY, "node `*` node") 
X(division,       BINARY_NODE_BODY, "node `/` node") 
X(modulus,        BINARY_NODE_BODY, "node `%` node") 

/* bitwise expressions */
X(bitwise_negation,              UNARY_NODE_BODY,  "`~` node")
X(bitwise_conjunction,           BINARY_NODE_BODY, "node `&` node")
X(bitwise_disjunction,           BINARY_NODE_BODY, "node `|` node")
X(bitwise_exclusive_disjunction, BINARY_NODE_BODY, "node `^` node")
X(bitwise_left_shift,            BINARY_NODE_BODY, "node `<<` node")
X(bitwise_right_shift,           BINARY_NODE_BODY, "node `>>` node")

/* assignment expressions */
X(assignment,                               BINARY_NODE_BODY, "node `=` node")
X(addition_assignment,                      BINARY_NODE_BODY, "node `+=` node")
X(subtraction_assignment,                   BINARY_NODE_BODY, "node `-=` node")
X(multiplication_assignment,                BINARY_NODE_BODY, "node `*=` node")
X(division_assignment,                      BINARY_NODE_BODY, "node `/=` node")
X(modulus_assignment,                       BINARY_NODE_BODY, "node `%=` node")
X(bitwise_conjunction_assignment,           BINARY_NODE_BODY, "node `&=` node")
X(bitwise_disjunction_assignment,           BINARY_NODE_BODY, "node `|=` node")
X(bitwise_exclusive_disjunction_assignment, BINARY_NODE_BODY, "node `^=` node")
X(bitwise_left_shift_assignment,            BINARY_NODE_BODY, "node `<<=` node")
X(bitwise_right_shift_assignment,           BINARY_NODE_BODY, "node `>>=` node")

/* other expressions */
X(reference,   UNARY_NODE_BODY,   "`@` node")
X(invocation,  BINARY_NODE_BODY,  "node node")
X(resolution,  BINARY_NODE_BODY,  "node `.` node")
X(list,        BINARY_NODE_BODY,  "node `,` node")
X(procedure,   TERNARY_NODE_BODY, "node `->` node")
X(declaration, TERNARY_NODE_BODY, "node `:` node")
X(condition,   TERNARY_NODE_BODY, "node `?` node [`:` node]")

#undef TERNARY_NODE_BODY
#undef BINARY_NODE_BODY
#undef UNARY_NODE_BODY

// literals
X(scope,      { node *nodes; uint nodes_count; }, "`{` node {`;` node} `}`")
X(identifier, { utf8 *runes; uint runes_count; }, "identifier")
X(string,     { utf8 *runes; uint runes_count; }, "string")
X(digital,    { uint64  value; }, "digital|hexadecimal|binary")
X(decimal,    { float64 value; }, "decimal|scientific")
