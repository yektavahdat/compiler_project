#ifndef BM_TOKEN_HPP
#define BM_TOKEN_HPP

#include <string>

namespace bm {

struct CodePos {
    std::string file;
    int line = 1;
    int col = 1;
};

enum class SymKind {
    // Reserved keywords
    SYM_ARRAY,
    SYM_AUTO,
    SYM_BOOLEAN,
    SYM_CHAR,
    SYM_ELSE,
    SYM_FALSE,
    SYM_FLOAT,
    SYM_FOR,
    SYM_FUNCTION,
    SYM_IF,
    SYM_INTEGER,
    SYM_RETURN,
    SYM_STRING,
    SYM_TRUE,
    SYM_VOID,
    SYM_WHILE,

    // Identifiers & literals
    SYM_IDENT,
    SYM_INT_LIT,
    SYM_FLOAT_LIT,
    SYM_CHAR_LIT,
    SYM_STR_LIT,

    // Operators
    SYM_INC,      // ++
    SYM_DEC,      // --
    SYM_LE,       // <=
    SYM_GE,       // >=
    SYM_EQ,       // ==
    SYM_NE,       // !=
    SYM_LAND,     // &&
    SYM_LOR,      // ||
    SYM_ADD,      // +
    SYM_SUB,      // -
    SYM_MUL,      // *
    SYM_DIV,      // /
    SYM_REM,      // %
    SYM_POW,      // ^
    SYM_SET,      // =
    SYM_LT,       // <
    SYM_GT,       // >
    SYM_LNOT,     // !

    // Delimiters & punctuation
    SYM_LPAREN,   // (
    SYM_RPAREN,   // )
    SYM_LBRACE,   // {
    SYM_RBRACE,   // }
    SYM_LBRACK,   // [
    SYM_RBRACK,   // ]
    SYM_SEMI,     // ;
    SYM_COLON,    // :
    SYM_COMMA,    // ,

    // End & error tokens
    SYM_EOF,
    SYM_BAD
};

struct LexemeItem {
    SymKind kind = SymKind::SYM_BAD;
    std::string text;
    CodePos pos;
};

std::string symKindName(SymKind kind);

} // namespace bm

#endif // BM_TOKEN_HPP