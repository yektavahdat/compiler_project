#include "bm_parser.hpp"
#include <utility>

namespace bm {

SyntaxParser::SyntaxParser(std::vector<LexemeItem> tokens)
    : tokens_(std::move(tokens)) {}

bool SyntaxParser::atEnd() const {
    return idx_ >= tokens_.size() || tokens_[idx_].kind == SymKind::SYM_EOF;
}

const LexemeItem& SyntaxParser::peek(size_t ahead) const {
    size_t target = idx_ + ahead;
    if (target >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[target];
}

const LexemeItem& SyntaxParser::lastConsumed() const {
    if (idx_ == 0) return tokens_.front();
    return tokens_[idx_ - 1];
}

const LexemeItem& SyntaxParser::advance() {
    if (!atEnd()) {
        idx_++;
    }
    return lastConsumed();
}

bool SyntaxParser::check(SymKind kind) const {
    if (atEnd()) return kind == SymKind::SYM_EOF;
    return peek().kind == kind;
}

bool SyntaxParser::match(const std::vector<SymKind>& kinds) {
    for (SymKind k : kinds) {
        if (check(k)) {
            advance();
            return true;
        }
    }
    return false;
}

bool SyntaxParser::expect(SymKind kind, const std::string& msg) {
    if (check(kind)) {
        advance();
        return true;
    }
    logIssue(peek().pos, msg + " (found '" + peek().text + "')");
    return false;
}

bool SyntaxParser::startsType() const {
    switch (peek().kind) {
        case SymKind::SYM_INTEGER:
        case SymKind::SYM_BOOLEAN:
        case SymKind::SYM_CHAR:
        case SymKind::SYM_STRING:
        case SymKind::SYM_FLOAT:
        case SymKind::SYM_VOID:
        case SymKind::SYM_AUTO:
        case SymKind::SYM_ARRAY:
            return true;
        default:
            return false;
    }
}

bool SyntaxParser::isRecoveryAnchor() const {
    switch (peek().kind) {
        case SymKind::SYM_SEMI:
        case SymKind::SYM_RBRACE:
        case SymKind::SYM_IF:
        case SymKind::SYM_WHILE:
        case SymKind::SYM_FOR:
        case SymKind::SYM_RETURN:
        case SymKind::SYM_EOF:
            return true;
        default:
            return false;
    }
}

void SyntaxParser::logIssue(const CodePos& pos, const std::string& msg) {
    issues_.push_back(SyntaxIssue{pos, msg});
}

// Panic-mode recovery: discard tokens until reaching a safe synchronization point
void SyntaxParser::panicSync() {
    if (check(SymKind::SYM_SEMI)) {
        advance();
        return;
    }
    while (!atEnd() && !isRecoveryAnchor()) {
        advance();
    }
    if (check(SymKind::SYM_SEMI)) {
        advance();
    }
}

std::unique_ptr<UnitNode> SyntaxParser::parseUnit() {
    CodePos startPos = peek().pos;
    auto unit = std::make_unique<UnitNode>(startPos);

    while (!atEnd()) {
        DeclBasePtr decl = parseTopDecl();
        if (decl) {
            unit->topDecls.push_back(std::move(decl));
        } else {
            panicSync();
            if (!atEnd() && !check(SymKind::SYM_IDENT)) {
                advance();
            }
        }
    }
    return unit;
}

DeclBasePtr SyntaxParser::parseTopDecl() {
    if (!check(SymKind::SYM_IDENT)) {
        logIssue(peek().pos, "Expected identifier at start of declaration");
        return nullptr;
    }

    LexemeItem idItem = advance();
    if (!expect(SymKind::SYM_COLON, "Expected ':' after declaration identifier")) {
        return nullptr;
    }

    if (match({SymKind::SYM_FUNCTION})) {
        return parseFuncDef(idItem.text, idItem.pos);
    }
    return parseVarDef(idItem.text, idItem.pos);
}

DeclBasePtr SyntaxParser::parseVarDef(const std::string& name, CodePos pos) {
    auto typeNode = parseTypeRef();
    if (!typeNode) {
        return nullptr;
    }

    ExprBasePtr initVal = nullptr;
    if (match({SymKind::SYM_SET})) {
        initVal = parseExpr();
    }

    if (!expect(SymKind::SYM_SEMI, "Expected ';' at end of variable definition")) {
        panicSync();
    }

    return std::make_unique<VarDefNode>(std::move(pos), name, std::move(typeNode), std::move(initVal));
}

DeclBasePtr SyntaxParser::parseFuncDef(const std::string& name, CodePos pos) {
    auto retType = parseTypeRef();
    if (!retType) {
        return nullptr;
    }

    if (!expect(SymKind::SYM_LPAREN, "Expected '(' after function return type")) {
        return nullptr;
    }

    std::vector<FormalParamNode> params;
    if (!check(SymKind::SYM_RPAREN)) {
        params = parseFormalList();
    }

    if (!expect(SymKind::SYM_RPAREN, "Expected ')' after formal parameter list")) {
        panicSync();
    }

    auto fnNode = std::make_unique<FuncDefNode>(std::move(pos), name, std::move(retType));
    fnNode->params = std::move(params);

    if (check(SymKind::SYM_LBRACE)) {
        fnNode->body = parseSuite();
    } else if (match({SymKind::SYM_SEMI})) {
        fnNode->body = nullptr;
    } else {
        logIssue(peek().pos, "Expected '{' or ';' after function header");
        panicSync();
    }

    return fnNode;
}

std::vector<FormalParamNode> SyntaxParser::parseFormalList() {
    std::vector<FormalParamNode> list;
    list.push_back(parseFormalParam());
    while (match({SymKind::SYM_COMMA})) {
        list.push_back(parseFormalParam());
    }
    return list;
}

FormalParamNode SyntaxParser::parseFormalParam() {
    CodePos pos = peek().pos;
    std::string paramIdent;
    if (check(SymKind::SYM_IDENT)) {
        paramIdent = advance().text;
    } else {
        logIssue(pos, "Expected parameter identifier");
    }
    expect(SymKind::SYM_COLON, "Expected ':' after parameter identifier");
    auto typeNode = parseTypeRef();
    return FormalParamNode(std::move(pos), paramIdent, std::move(typeNode));
}

std::unique_ptr<TypeRefNode> SyntaxParser::parseTypeRef() {
    CodePos pos = peek().pos;
    if (match({SymKind::SYM_ARRAY})) {
        SyntaxNodePtr dim = nullptr;
        if (expect(SymKind::SYM_LBRACK, "Expected '[' after 'array'")) {
            if (!check(SymKind::SYM_RBRACK)) {
                dim = parseExpr();
            }
            expect(SymKind::SYM_RBRACK, "Expected ']' after array dimension");
        }
        auto elemType = parseTypeRef();
        std::string baseName = elemType ? elemType->typeName : "integer";
        auto vecNode = std::make_unique<TypeRefNode>(std::move(pos), baseName, true);
        vecNode->dimExpr = std::move(dim);
        return vecNode;
    }

    if (startsType()) {
        LexemeItem tok = advance();
        return std::make_unique<TypeRefNode>(std::move(pos), tok.text, false);
    }

    logIssue(pos, "Expected type specifier");
    return nullptr;
}

std::unique_ptr<SuiteNode> SyntaxParser::parseSuite() {
    CodePos pos = peek().pos;
    expect(SymKind::SYM_LBRACE, "Expected '{' to begin compound statement");
    auto suite = std::make_unique<SuiteNode>(std::move(pos));

    while (!atEnd() && !check(SymKind::SYM_RBRACE)) {
        size_t savedIdx = idx_;
        StmtBasePtr stmt = parseStmt();
        if (stmt) {
            suite->items.push_back(std::move(stmt));
        } else {
            panicSync();
            if (idx_ == savedIdx && !atEnd() && !check(SymKind::SYM_RBRACE)) {
                advance();
            }
        }
    }

    expect(SymKind::SYM_RBRACE, "Expected '}' to end compound statement");
    return suite;
}

StmtBasePtr SyntaxParser::parseStmt() {
    if (check(SymKind::SYM_IF)) return parseIfBranch();
    if (check(SymKind::SYM_WHILE)) return parseWhile();
    if (check(SymKind::SYM_FOR)) return parseFor();
    if (check(SymKind::SYM_RETURN)) return parseReturn();
    if (check(SymKind::SYM_LBRACE)) return parseSuite();

    // Local variable declaration inside block: IDENT ':' type
    if (check(SymKind::SYM_IDENT) && peek(1).kind == SymKind::SYM_COLON) {
        CodePos pos = peek().pos;
        std::string name = advance().text;
        advance(); // consume ':'
        DeclBasePtr decl = parseVarDef(name, std::move(pos));
        if (!decl) return nullptr;
        return std::make_unique<ExprStmtNode>(decl->pos, nullptr);
    }

    return parseSimpleStmt();
}

StmtBasePtr SyntaxParser::parseIfBranch() {
    CodePos pos = advance().pos; // consume 'if'
    expect(SymKind::SYM_LPAREN, "Expected '(' after 'if'");
    ExprBasePtr cond = parseExpr();
    expect(SymKind::SYM_RPAREN, "Expected ')' after if condition");
    StmtBasePtr thenStmt = parseStmt();

    StmtBasePtr elseStmt = nullptr;
    if (match({SymKind::SYM_ELSE})) {
        elseStmt = parseStmt();
    }

    return std::make_unique<CondBranchNode>(std::move(pos), std::move(cond), std::move(thenStmt), std::move(elseStmt));
}

StmtBasePtr SyntaxParser::parseWhile() {
    CodePos pos = advance().pos; // consume 'while'
    expect(SymKind::SYM_LPAREN, "Expected '(' after 'while'");
    ExprBasePtr cond = parseExpr();
    expect(SymKind::SYM_RPAREN, "Expected ')' after while condition");
    StmtBasePtr body = parseStmt();
    return std::make_unique<WhileLoopNode>(std::move(pos), std::move(cond), std::move(body));
}

StmtBasePtr SyntaxParser::parseFor() {
    CodePos pos = advance().pos; // consume 'for'
    expect(SymKind::SYM_LPAREN, "Expected '(' after 'for'");

    ExprBasePtr initE = nullptr;
    if (!check(SymKind::SYM_SEMI)) {
        initE = parseExpr();
    }
    expect(SymKind::SYM_SEMI, "Expected ';' after for-loop initializer");

    ExprBasePtr condE = nullptr;
    if (!check(SymKind::SYM_SEMI)) {
        condE = parseExpr();
    }
    expect(SymKind::SYM_SEMI, "Expected ';' after for-loop condition");

    ExprBasePtr stepE = nullptr;
    if (!check(SymKind::SYM_RPAREN)) {
        stepE = parseExpr();
    }
    expect(SymKind::SYM_RPAREN, "Expected ')' after for-loop header");

    StmtBasePtr body = parseStmt();
    return std::make_unique<ForLoopNode>(
        std::move(pos), std::move(initE), std::move(condE), std::move(stepE), std::move(body));
}

StmtBasePtr SyntaxParser::parseReturn() {
    CodePos pos = advance().pos; // consume 'return'
    ExprBasePtr val = nullptr;
    if (!check(SymKind::SYM_SEMI)) {
        val = parseExpr();
    }
    if (!expect(SymKind::SYM_SEMI, "Expected ';' after return statement")) {
        panicSync();
    }
    return std::make_unique<RetStmtNode>(std::move(pos), std::move(val));
}

StmtBasePtr SyntaxParser::parseSimpleStmt() {
    CodePos pos = peek().pos;
    ExprBasePtr expr = nullptr;
    if (!check(SymKind::SYM_SEMI)) {
        expr = parseExpr();
    }
    if (!expect(SymKind::SYM_SEMI, "Expected ';' at end of statement")) {
        panicSync();
    }
    return std::make_unique<ExprStmtNode>(std::move(pos), std::move(expr));
}

ExprBasePtr SyntaxParser::parseExpr() {
    return parseAssign();
}

ExprBasePtr SyntaxParser::parseAssign() {
    ExprBasePtr left = parseOr();
    if (match({SymKind::SYM_SET})) {
        CodePos pos = lastConsumed().pos;
        ExprBasePtr right = parseAssign();
        return std::make_unique<SetExprNode>(std::move(pos), std::move(left), std::move(right));
    }
    return left;
}

ExprBasePtr SyntaxParser::parseOr() {
    ExprBasePtr expr = parseAnd();
    while (match({SymKind::SYM_LOR})) {
        CodePos pos = lastConsumed().pos;
        std::string op = lastConsumed().text;
        ExprBasePtr right = parseAnd();
        expr = std::make_unique<BinOpExprNode>(std::move(pos), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprBasePtr SyntaxParser::parseAnd() {
    ExprBasePtr expr = parseEquality();
    while (match({SymKind::SYM_LAND})) {
        CodePos pos = lastConsumed().pos;
        std::string op = lastConsumed().text;
        ExprBasePtr right = parseEquality();
        expr = std::make_unique<BinOpExprNode>(std::move(pos), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprBasePtr SyntaxParser::parseEquality() {
    ExprBasePtr expr = parseRelational();
    while (match({SymKind::SYM_EQ, SymKind::SYM_NE})) {
        CodePos pos = lastConsumed().pos;
        std::string op = lastConsumed().text;
        ExprBasePtr right = parseRelational();
        expr = std::make_unique<BinOpExprNode>(std::move(pos), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprBasePtr SyntaxParser::parseRelational() {
    ExprBasePtr expr = parseAdditive();
    while (match({SymKind::SYM_LT, SymKind::SYM_LE, SymKind::SYM_GT, SymKind::SYM_GE})) {
        CodePos pos = lastConsumed().pos;
        std::string op = lastConsumed().text;
        ExprBasePtr right = parseAdditive();
        expr = std::make_unique<BinOpExprNode>(std::move(pos), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprBasePtr SyntaxParser::parseAdditive() {
    ExprBasePtr expr = parseMultiplicative();
    while (match({SymKind::SYM_ADD, SymKind::SYM_SUB})) {
        CodePos pos = lastConsumed().pos;
        std::string op = lastConsumed().text;
        ExprBasePtr right = parseMultiplicative();
        expr = std::make_unique<BinOpExprNode>(std::move(pos), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprBasePtr SyntaxParser::parseMultiplicative() {
    ExprBasePtr expr = parsePreUnary();
    while (match({SymKind::SYM_MUL, SymKind::SYM_DIV, SymKind::SYM_REM})) {
        CodePos pos = lastConsumed().pos;
        std::string op = lastConsumed().text;
        ExprBasePtr right = parsePreUnary();
        expr = std::make_unique<BinOpExprNode>(std::move(pos), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprBasePtr SyntaxParser::parsePreUnary() {
    if (match({SymKind::SYM_SUB, SymKind::SYM_LNOT})) {
        CodePos pos = lastConsumed().pos;
        std::string op = lastConsumed().text;
        ExprBasePtr rhs = parsePreUnary();
        return std::make_unique<PreUnaryExprNode>(std::move(pos), op, std::move(rhs));
    }
    return parseSuffix();
}

ExprBasePtr SyntaxParser::parseSuffix() {
    ExprBasePtr expr = parseAtom();
    while (true) {
        if (match({SymKind::SYM_LBRACK})) {
            CodePos pos = lastConsumed().pos;
            ExprBasePtr idx = parseExpr();
            expect(SymKind::SYM_RBRACK, "Expected ']' after array subscript");
            expr = std::make_unique<SubscriptExprNode>(std::move(pos), std::move(expr), std::move(idx));
        } else if (match({SymKind::SYM_LPAREN})) {
            CodePos pos = lastConsumed().pos;
            std::vector<ExprBasePtr> args;
            if (!check(SymKind::SYM_RPAREN)) {
                args = parseActualList();
            }
            expect(SymKind::SYM_RPAREN, "Expected ')' after argument list");

            std::string fnName = "<callee>";
            if (auto* idNode = dynamic_cast<IdentExprNode*>(expr.get())) {
                fnName = idNode->name;
            }
            expr = std::make_unique<InvokeExprNode>(std::move(pos), fnName, std::move(args));
        } else if (match({SymKind::SYM_INC, SymKind::SYM_DEC})) {
            CodePos pos = lastConsumed().pos;
            std::string op = lastConsumed().text;
            expr = std::make_unique<PostUnaryExprNode>(std::move(pos), op, std::move(expr));
        } else {
            break;
        }
    }
    return expr;
}

std::vector<ExprBasePtr> SyntaxParser::parseActualList() {
    std::vector<ExprBasePtr> args;
    args.push_back(parseExpr());
    while (match({SymKind::SYM_COMMA})) {
        args.push_back(parseExpr());
    }
    return args;
}

ExprBasePtr SyntaxParser::parseAtom() {
    CodePos pos = peek().pos;

    if (match({SymKind::SYM_INT_LIT})) {
        return std::make_unique<LitIntNode>(std::move(pos), lastConsumed().text);
    }
    if (match({SymKind::SYM_FLOAT_LIT})) {
        return std::make_unique<LitFloatNode>(std::move(pos), lastConsumed().text);
    }
    if (match({SymKind::SYM_CHAR_LIT})) {
        return std::make_unique<LitCharNode>(std::move(pos), lastConsumed().text);
    }
    if (match({SymKind::SYM_STR_LIT})) {
        return std::make_unique<LitStringNode>(std::move(pos), lastConsumed().text);
    }
    if (match({SymKind::SYM_TRUE})) {
        return std::make_unique<LitBoolNode>(std::move(pos), true);
    }
    if (match({SymKind::SYM_FALSE})) {
        return std::make_unique<LitBoolNode>(std::move(pos), false);
    }
    if (match({SymKind::SYM_IDENT})) {
        return std::make_unique<IdentExprNode>(std::move(pos), lastConsumed().text);
    }
    if (match({SymKind::SYM_LPAREN})) {
        ExprBasePtr inner = parseExpr();
        expect(SymKind::SYM_RPAREN, "Expected ')' after grouped expression");
        return inner;
    }

    logIssue(pos, "Expected expression (found '" + peek().text + "')");
    return nullptr;
}

} // namespace bm