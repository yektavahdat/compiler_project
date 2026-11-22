#ifndef BM_PARSER_HPP
#define BM_PARSER_HPP

#include "bm_ast.hpp"
#include "bm_token.hpp"
#include <string>
#include <vector>

namespace bm {

struct SyntaxIssue {
    CodePos pos;
    std::string message;
};

class SyntaxParser {
public:
    explicit SyntaxParser(std::vector<LexemeItem> tokens);

    std::unique_ptr<UnitNode> parseUnit();
    const std::vector<SyntaxIssue>& issues() const { return issues_; }

private:
    std::vector<LexemeItem> tokens_;
    size_t idx_ = 0;
    std::vector<SyntaxIssue> issues_;

    bool atEnd() const;
    const LexemeItem& peek(size_t ahead = 0) const;
    const LexemeItem& lastConsumed() const;
    const LexemeItem& advance();
    bool check(SymKind kind) const;
    bool match(const std::vector<SymKind>& kinds);
    bool expect(SymKind kind, const std::string& msg);

    bool startsType() const;
    bool isRecoveryAnchor() const;
    void logIssue(const CodePos& pos, const std::string& msg);
    void panicSync();

    // Top-down grammar productions
    DeclBasePtr parseTopDecl();
    DeclBasePtr parseVarDef(const std::string& name, CodePos pos);
    DeclBasePtr parseFuncDef(const std::string& name, CodePos pos);
    std::vector<FormalParamNode> parseFormalList();
    FormalParamNode parseFormalParam();

    std::unique_ptr<TypeRefNode> parseTypeRef();
    std::unique_ptr<SuiteNode> parseSuite();
    StmtBasePtr parseStmt();
    StmtBasePtr parseIfBranch();
    StmtBasePtr parseWhile();
    StmtBasePtr parseFor();
    StmtBasePtr parseReturn();
    StmtBasePtr parseSimpleStmt();

    ExprBasePtr parseExpr();
    ExprBasePtr parseAssign();
    ExprBasePtr parseOr();
    ExprBasePtr parseAnd();
    ExprBasePtr parseEquality();
    ExprBasePtr parseRelational();
    ExprBasePtr parseAdditive();
    ExprBasePtr parseMultiplicative();
    ExprBasePtr parsePreUnary();
    ExprBasePtr parseSuffix();
    std::vector<ExprBasePtr> parseActualList();
    ExprBasePtr parseAtom();
};

} // namespace bm

#endif // BM_PARSER_HPP