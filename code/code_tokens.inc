/* (identifier, code, representation) */
  
X(unknown, '\0', "unknown")
X(etx,     '\3', "ETX") /* end of text */

/* single charactered */
X(exclamation_mark,     '!', "`!`")
X(octothorpe,           '#', "`#`")
X(dollar_sign,          '$', "`$`")
X(percent_sign,         '%', "`%`")
X(ampersand,            '&', "`&`")
X(left_parenthesis,     '(', "`(`")
X(right_parenthesis,    ')', "`)`")
X(asterisk,             '*', "`*`")
X(plus_sign,            '+', "`+`")
X(comma,                ',', "`,`")
X(minus_sign,           '-', "`-`")
X(full_stop,            '.', "`.`")
X(slash,                '/', "`/`")
X(colon,                ':', "`:`")
X(semicolon,            ';', "`;`")
X(less_than_sign,       '<', "`<`")
X(equal_sign,           '=', "`=`")
X(greater_than_sign,    '>', "`>`")
X(question_sign,        '?', "`?`")
X(at_sign,              '@', "`@`")
X(left_square_bracket,  '[', "`[`")
X(backslask,            '\\', "`\\`")
X(right_square_bracket, ']', "`]`")
X(circumflex_accent,    '^', "`^`")
X(grave_accent,         '`', "```")
X(left_curly_bracket,   '{', "`{`")
X(vertical_bar,         '|', "`|`")
X(right_curly_bracket,  '}', "`}`")
X(tilde,                '~', "`~`")

/* multi charactered */
X(exclamation_mark_equal_sign,    128, "`!=`")
X(percent_sign_equal_sign,        129, "`%=`")
X(ampersand_equal_sign,           130, "`&=`")
X(ampersand_2,                    131, "`&&`")
X(asterisk_equal_sign,            132, "`*=`")
X(plus_sign_equal_sign,           133, "`+=`")
X(minus_sign_equal_sign,          135, "`-=`")
X(minus_sign_greater_than_sign,   136, "`->`")
X(slash_equal_sign,               137, "`/=`")
X(less_than_sign_2,               138, "`<<`")
X(less_than_sign_2_equal_sign,    139, "`<<=`")
X(less_than_sign_equal_sign,      141, "`<=`")
X(equal_sign_2,                   142, "`==`")
X(greater_than_sign_2,            143, "`>>`")
X(greater_than_sign_2_equal_sign, 144, "`>>=`")
X(greater_than_sign_equal_sign,   145, "`>=`")
X(circumflex_accent_equal_sign,   146, "`^=`")
X(vertical_bar_2,                 147, "`||`")
X(vertical_bar_equal_sign,        148, "`|=`")

/* literals */
X(identifier,  149, "identifier")  /* "<'_'|letter> {'_'|letter|'-'|digit}" */
X(string,      150, "string")      /* */
X(decimal,     151, "decimal")     /* */
X(scientific,  152, "scientific")  /* */
X(digital,     153, "digital")     /* */
X(hexadecimal, 154, "hexadecimal") /* */
X(binary,      155, "binary")      /* */
