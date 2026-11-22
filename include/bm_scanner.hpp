#include "bm_scanner.hpp"
#include <cctype>

namespace bm {

const std::unordered_map<std::string, SymKind> Scanner::reservedMap_ = {
    {"array",    SymKind::SYM_ARRAY},
    {"auto",     SymKind::SYM_AUTO},
    {"boolean",  SymKind::SYM_BOOLEAN},
    {"char",     SymKind::SYM_CHAR},
    {"else",     SymKind::SYM_ELSE},
    {"false",    SymKind::SYM_FALSE},
    {"float",    SymKind::SYM_FLOAT},
    {"for",      SymKind::SYM_FOR},
    {"function", SymKind::SYM_FUNCTION},
    {"if",       SymKind::SYM_IF},
    {"integer",  SymKind::SYM_INTEGER},
    {"return",   SymKind::SYM_RETURN},
    {"string",   SymKind::SYM_STRING},
    {"true",     SymKind::SYM_TRUE},
    {"void",     SymKind::SYM_VOID},
    {"while",    SymKind::SYM_WHILE}
};

std::string symKindName(SymKind kind) {
    switch (kind) {
        case SymKind::SYM_ARRAY: return "SYM_ARRAY";
        case SymKind::SYM_AUTO: return "SYM_AUTO";
        case SymKind::SYM_BOOLEAN: return "SYM_BOOLEAN";
        case SymKind::SYM_CHAR: return "SYM_CHAR";
        case SymKind::SYM_ELSE: return "SYM_ELSE";
        case SymKind::SYM_FALSE: return "SYM_FALSE";
        case SymKind::SYM_FLOAT: return "SYM_FLOAT";
        case SymKind::SYM_FOR: return "SYM_FOR";
        case SymKind::SYM_FUNCTION: return "SYM_FUNCTION";
        case SymKind::SYM_IF: return "SYM_IF";
        case SymKind::SYM_INTEGER: return "SYM_INTEGER";
        case SymKind::SYM_RETURN: return "SYM_RETURN";
        case SymKind::SYM_STRING: return "SYM_STRING";
        case SymKind::SYM_TRUE: return "SYM_TRUE";
        case SymKind::SYM_VOID: return "SYM_VOID";
        case SymKind::SYM_WHILE: return "SYM_WHILE";
        case SymKind::SYM_IDENT: return "SYM_IDENT";
        case SymKind::SYM_INT_LIT: return "SYM_INT_LIT";
        case SymKind::SYM_FLOAT_LIT: return "SYM_FLOAT_LIT";
        case SymKind::SYM_CHAR_LIT: return "SYM_CHAR_LIT";
        case SymKind::SYM_STR_LIT: return "SYM_STR_LIT";
        case SymKind::SYM_INC: return "SYM_INC";
        case SymKind::SYM_DEC: return "SYM_DEC";
        case SymKind::SYM_LE: return "SYM_LE";
        case SymKind::SYM_GE: return "SYM_GE";
        case SymKind::SYM_EQ: return "SYM_EQ";
        case SymKind::SYM_NE: return "SYM_NE";
        case SymKind::SYM_LAND: return "SYM_LAND";
        case SymKind::SYM_LOR: return "SYM_LOR";
        case SymKind::SYM_ADD: return "SYM_ADD";
        case SymKind::SYM_SUB: return "SYM_SUB";
        case SymKind::SYM_MUL: return "SYM_MUL";
        case SymKind::SYM_DIV: return "SYM_DIV";
        case SymKind::SYM_REM: return "SYM_REM";
        case SymKind::SYM_POW: return "SYM_POW";
        case SymKind::SYM_SET: return "SYM_SET";
        case SymKind::SYM_LT: return "SYM_LT";
        case SymKind::SYM_GT: return "SYM_GT";
        case SymKind::SYM_LNOT: return "SYM_LNOT";
        case SymKind::SYM_LPAREN: return "SYM_LPAREN";
        case SymKind::SYM_RPAREN: return "SYM_RPAREN";
        case SymKind::SYM_LBRACE: return "SYM_LBRACE";
        case SymKind::SYM_RBRACE: return "SYM_RBRACE";
        case SymKind::SYM_LBRACK: return "SYM_LBRACK";
        case SymKind::SYM_RBRACK: return "SYM_RBRACK";
        case SymKind::SYM_SEMI: return "SYM_SEMI";
        case SymKind::SYM_COLON: return "SYM_COLON";
        case SymKind::SYM_COMMA: return "SYM_COMMA";
        case SymKind::SYM_EOF: return "SYM_EOF";
        case SymKind::SYM_BAD: return "SYM_BAD";
    }
    return "SYM_UNKNOWN";
}

Scanner::Scanner(std::string input, std::string fileName)
    : input_(std::move(input)), fileName_(std::move(fileName)) {}

bool Scanner::atEnd() const {
    return cursor_ >= input_.size();
}

char Scanner::lookAhead(size_t ahead) const {
    if (cursor_ + ahead >= input_.size()) return '\0';
    return input_[cursor_ + ahead];
}

char Scanner::consume() {
    if (atEnd()) return '\0';
    char ch = input_[cursor_++];
    if (ch == '\n') {
        line_++;
        col_ = 1;
    } else {
        col_++;
    }
    return ch;
}

CodePos Scanner::currentPos() const {
    return CodePos{fileName_, line_, col_};
}

void Scanner::recordDiag(const CodePos& pos, const std::string& detail) {
    diagnostics_.push_back(ScanDiagnostic{pos, detail});
}

void Scanner::skipIgnored() {
    while (!atEnd()) {
        char ch = lookAhead();
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            consume();
        } else if (ch == '/' && lookAhead(1) == '/') {
            consume();
            consume();
            while (!atEnd() && lookAhead() != '\n') {
                consume();
            }
        } else if (ch == '/' && lookAhead(1) == '*') {
            CodePos start = currentPos();
            consume();
            consume();
            bool terminated = false;
            while (!atEnd()) {
                if (lookAhead() == '*' && lookAhead(1) == '/') {
                    consume();
                    consume();
                    terminated = true;
                    break;
                }
                consume();
            }
            if (!terminated) {
                recordDiag(start, "Unterminated block comment");
            }
        } else {
            break;
        }
    }
}

LexemeItem Scanner::readWordOrKeyword() {
    CodePos start = currentPos();
    std::string buf;
    while (!atEnd() && (std::isalnum(static_cast<unsigned char>(lookAhead())) || lookAhead() == '_')) {
        buf.push_back(consume());
    }
    auto found = reservedMap_.find(buf);
    if (found != reservedMap_.end()) {
        return LexemeItem{found->second, buf, start};
    }
    return LexemeItem{SymKind::SYM_IDENT, buf, start};
}

LexemeItem Scanner::readNumeric() {
    CodePos start = currentPos();
    std::string buf;
    while (!atEnd() && std::isdigit(static_cast<unsigned char>(lookAhead()))) {
        buf.push_back(consume());
    }
    if (lookAhead() == '.' && std::isdigit(static_cast<unsigned char>(lookAhead(1)))) {
        buf.push_back(consume());
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(lookAhead()))) {
            buf.push_back(consume());
        }
        return LexemeItem{SymKind::SYM_FLOAT_LIT, buf, start};
    }
    return LexemeItem{SymKind::SYM_INT_LIT, buf, start};
}

LexemeItem Scanner::readQuotedString() {
    CodePos start = currentPos();
    std::string buf;
    buf.push_back(consume());
    bool closed = false;

    while (!atEnd()) {
        char ch = lookAhead();
        if (ch == '"') {
            buf.push_back(consume());
            closed = true;
            break;
        }
        if (ch == '\\') {
            buf.push_back(consume());
            if (!atEnd()) {
                buf.push_back(consume());
            }
        } else {
            buf.push_back(consume());
        }
    }

    if (!closed) {
        recordDiag(start, "Unterminated string literal");
        return LexemeItem{SymKind::SYM_BAD, buf, start};
    }
    return LexemeItem{SymKind::SYM_STR_LIT, buf, start};
}

LexemeItem Scanner::readCharConst() {
    CodePos start = currentPos();
    std::string buf;
    buf.push_back(consume());
    bool closed = false;

    while (!atEnd()) {
        char ch = lookAhead();
        if (ch == '\'') {
            buf.push_back(consume());
            closed = true;
            break;
        }
        if (ch == '\\') {
            buf.push_back(consume());
            if (!atEnd()) {
                buf.push_back(consume());
            }
        } else {
            buf.push_back(consume());
        }
    }

    if (!closed) {
        recordDiag(start, "Unterminated character literal");
        return LexemeItem{SymKind::SYM_BAD, buf, start};
    }
    return LexemeItem{SymKind::SYM_CHAR_LIT, buf, start};
}

LexemeItem Scanner::readSymbol() {
    CodePos pos = currentPos();
    char ch = lookAhead();
    char nxt = lookAhead(1);

    if (ch == '+' && nxt == '+') { consume(); consume(); return LexemeItem{SymKind::SYM_INC, "++", pos}; }
    if (ch == '-' && nxt == '-') { consume(); consume(); return LexemeItem{SymKind::SYM_DEC, "--", pos}; }
    if (ch == '<' && nxt == '=') { consume(); consume(); return LexemeItem{SymKind::SYM_LE, "<=", pos}; }
    if (ch == '>' && nxt == '=') { consume(); consume(); return LexemeItem{SymKind::SYM_GE, ">=", pos}; }
    if (ch == '=' && nxt == '=') { consume(); consume(); return LexemeItem{SymKind::SYM_EQ, "==", pos}; }
    if (ch == '!' && nxt == '=') { consume(); consume(); return LexemeItem{SymKind::SYM_NE, "!=", pos}; }
    if (ch == '&' && nxt == '&') { consume(); consume(); return LexemeItem{SymKind::SYM_LAND, "&&", pos}; }
    if (ch == '|' && nxt == '|') { consume(); consume(); return LexemeItem{SymKind::SYM_LOR, "||", pos}; }

    consume();
    std::string s(1, ch);

    switch (ch) {
        case '+': return LexemeItem{SymKind::SYM_ADD, s, pos};
        case '-': return LexemeItem{SymKind::SYM_SUB, s, pos};
        case '*': return LexemeItem{SymKind::SYM_MUL, s, pos};
        case '/': return LexemeItem{SymKind::SYM_DIV, s, pos};
        case '%': return LexemeItem{SymKind::SYM_REM, s, pos};
        case '^': return LexemeItem{SymKind::SYM_POW, s, pos};
        case '=': return LexemeItem{SymKind::SYM_SET, s, pos};
        case '<': return LexemeItem{SymKind::SYM_LT, s, pos};
        case '>': return LexemeItem{SymKind::SYM_GT, s, pos};
        case '!': return LexemeItem{SymKind::SYM_LNOT, s, pos};
        case '(': return LexemeItem{SymKind::SYM_LPAREN, s, pos};
        case ')': return LexemeItem{SymKind::SYM_RPAREN, s, pos};
        case '{': return LexemeItem{SymKind::SYM_LBRACE, s, pos};
        case '}': return LexemeItem{SymKind::SYM_RBRACE, s, pos};
        case '[': return LexemeItem{SymKind::SYM_LBRACK, s, pos};
        case ']': return LexemeItem{SymKind::SYM_RBRACK, s, pos};
        case ';': return LexemeItem{SymKind::SYM_SEMI, s, pos};
        case ':': return LexemeItem{SymKind::SYM_COLON, s, pos};
        case ',': return LexemeItem{SymKind::SYM_COMMA, s, pos};
        default:
            recordDiag(pos, "Unexpected character '" + s + "'");
            return LexemeItem{SymKind::SYM_BAD, s, pos};
    }
}

std::vector<LexemeItem> Scanner::scanAll() {
    std::vector<LexemeItem> items;
    while (true) {
        skipIgnored();
        if (atEnd()) {
            items.push_back(LexemeItem{SymKind::SYM_EOF, "", currentPos()});
            break;
        }
        char ch = lookAhead();
        if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
            items.push_back(readWordOrKeyword());
        } else if (std::isdigit(static_cast<unsigned char>(ch))) {
            items.push_back(readNumeric());
        } else if (ch == '"') {
            items.push_back(readQuotedString());
        } else if (ch == '\'') {
            items.push_back(readCharConst());
        } else {
            items.push_back(readSymbol());
        }
    }
    return items;
}

} // namespace bm