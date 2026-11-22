#ifndef BM_AST_HPP
#define BM_AST_HPP

#include "bm_token.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bm {

enum class NodeKind {
    Unit,
    VarDef,
    FuncDef,
    FormalParam,
    TypeRef,
    Suite,
    CondBranch,
    WhileLoop,
    ForLoop,
    RetStmt,
    ExprStmt,
    SetExpr,
    BinOpExpr,
    PreUnaryExpr,
    PostUnaryExpr,
    InvokeExpr,
    SubscriptExpr,
    LitInt,
    LitFloat,
    LitChar,
    LitString,
    LitBool,
    IdentExpr
};

struct SyntaxNode {
    NodeKind kind;
    CodePos pos;
    // Reserved for later semantic analysis passes; empty in Phase 1
    std::optional<std::string> semType;

    SyntaxNode(NodeKind k, CodePos p)
        : kind(k), pos(std::move(p)), semType(std::nullopt) {}
    virtual ~SyntaxNode() = default;
};

using SyntaxNodePtr = std::unique_ptr<SyntaxNode>;

struct TypeRefNode : SyntaxNode {
    std::string typeName;
    bool isVector = false;
    SyntaxNodePtr dimExpr;

    TypeRefNode(CodePos p, std::string name, bool vec = false)
        : SyntaxNode(NodeKind::TypeRef, std::move(p)),
          typeName(std::move(name)),
          isVector(vec) {}
};

struct ExprBase : SyntaxNode {
    using SyntaxNode::SyntaxNode;
};

using ExprBasePtr = std::unique_ptr<ExprBase>;

struct StmtBase : SyntaxNode {
    using SyntaxNode::SyntaxNode;
};

using StmtBasePtr = std::unique_ptr<StmtBase>;

struct DeclBase : SyntaxNode {
    std::string ident;
    std::unique_ptr<TypeRefNode> typeRef;

    DeclBase(NodeKind k, CodePos p, std::string id, std::unique_ptr<TypeRefNode> t)
        : SyntaxNode(k, std::move(p)), ident(std::move(id)), typeRef(std::move(t)) {}
};

using DeclBasePtr = std::unique_ptr<DeclBase>;

struct FormalParamNode : SyntaxNode {
    std::string ident;
    std::unique_ptr<TypeRefNode> typeRef;

    FormalParamNode(CodePos p, std::string id, std::unique_ptr<TypeRefNode> t)
        : SyntaxNode(NodeKind::FormalParam, std::move(p)),
          ident(std::move(id)),
          typeRef(std::move(t)) {}
};

struct VarDefNode : DeclBase {
    ExprBasePtr initVal;

    VarDefNode(CodePos p, std::string id, std::unique_ptr<TypeRefNode> t, ExprBasePtr init = nullptr)
        : DeclBase(NodeKind::VarDef, std::move(p), std::move(id), std::move(t)),
          initVal(std::move(init)) {}
};

struct SuiteNode : StmtBase {
    std::vector<StmtBasePtr> items;

    explicit SuiteNode(CodePos p)
        : StmtBase(NodeKind::Suite, std::move(p)) {}
};

struct FuncDefNode : DeclBase {
    std::vector<FormalParamNode> params;
    std::unique_ptr<SuiteNode> body;

    FuncDefNode(CodePos p, std::string id, std::unique_ptr<TypeRefNode> retRef)
        : DeclBase(NodeKind::FuncDef, std::move(p), std::move(id), std::move(retRef)) {}
};

struct UnitNode : SyntaxNode {
    std::vector<DeclBasePtr> topDecls;

    explicit UnitNode(CodePos p)
        : SyntaxNode(NodeKind::Unit, std::move(p)) {}
};

struct CondBranchNode : StmtBase {
    ExprBasePtr testExpr;
    StmtBasePtr ifSuite;
    StmtBasePtr elseSuite;

    CondBranchNode(CodePos p, ExprBasePtr cond, StmtBasePtr thenS, StmtBasePtr elseS = nullptr)
        : StmtBase(NodeKind::CondBranch, std::move(p)),
          testExpr(std::move(cond)),
          ifSuite(std::move(thenS)),
          elseSuite(std::move(elseS)) {}
};

struct WhileLoopNode : StmtBase {
    ExprBasePtr testExpr;
    StmtBasePtr loopBody;

    WhileLoopNode(CodePos p, ExprBasePtr cond, StmtBasePtr b)
        : StmtBase(NodeKind::WhileLoop, std::move(p)),
          testExpr(std::move(cond)),
          loopBody(std::move(b)) {}
};

struct ForLoopNode : StmtBase {
    ExprBasePtr startExpr;
    ExprBasePtr stopExpr;
    ExprBasePtr stepExpr;
    StmtBasePtr loopBody;

    ForLoopNode(CodePos p, ExprBasePtr s, ExprBasePtr c, ExprBasePtr u, StmtBasePtr b)
        : StmtBase(NodeKind::ForLoop, std::move(p)),
          startExpr(std::move(s)),
          stopExpr(std::move(c)),
          stepExpr(std::move(u)),
          loopBody(std::move(b)) {}
};

struct RetStmtNode : StmtBase {
    ExprBasePtr retExpr;

    RetStmtNode(CodePos p, ExprBasePtr e = nullptr)
        : StmtBase(NodeKind::RetStmt, std::move(p)), retExpr(std::move(e)) {}
};

struct ExprStmtNode : StmtBase {
    ExprBasePtr expr;

    ExprStmtNode(CodePos p, ExprBasePtr e = nullptr)
        : StmtBase(NodeKind::ExprStmt, std::move(p)), expr(std::move(e)) {}
};

struct LitIntNode : ExprBase {
    std::string rawText;
    LitIntNode(CodePos p, std::string raw)
        : ExprBase(NodeKind::LitInt, std::move(p)), rawText(std::move(raw)) {}
};

struct LitFloatNode : ExprBase {
    std::string rawText;
    LitFloatNode(CodePos p, std::string raw)
        : ExprBase(NodeKind::LitFloat, std::move(p)), rawText(std::move(raw)) {}
};

struct LitCharNode : ExprBase {
    std::string rawText;
    LitCharNode(CodePos p, std::string raw)
        : ExprBase(NodeKind::LitChar, std::move(p)), rawText(std::move(raw)) {}
};

struct LitStringNode : ExprBase {
    std::string rawText;
    LitStringNode(CodePos p, std::string raw)
        : ExprBase(NodeKind::LitString, std::move(p)), rawText(std::move(raw)) {}
};

struct LitBoolNode : ExprBase {
    bool flag;
    LitBoolNode(CodePos p, bool f)
        : ExprBase(NodeKind::LitBool, std::move(p)), flag(f) {}
};

struct IdentExprNode : ExprBase {
    std::string name;
    IdentExprNode(CodePos p, std::string n)
        : ExprBase(NodeKind::IdentExpr, std::move(p)), name(std::move(n)) {}
};

struct PreUnaryExprNode : ExprBase {
    std::string op;
    ExprBasePtr target;

    PreUnaryExprNode(CodePos p, std::string oper, ExprBasePtr rhs)
        : ExprBase(NodeKind::PreUnaryExpr, std::move(p)), op(std::move(oper)), target(std::move(rhs)) {}
};

struct PostUnaryExprNode : ExprBase {
    std::string op;
    ExprBasePtr target;

    PostUnaryExprNode(CodePos p, std::string oper, ExprBasePtr expr)
        : ExprBase(NodeKind::PostUnaryExpr, std::move(p)), op(std::move(oper)), target(std::move(expr)) {}
};

struct BinOpExprNode : ExprBase {
    std::string op;
    ExprBasePtr left;
    ExprBasePtr right;

    BinOpExprNode(CodePos p, std::string oper, ExprBasePtr lhs, ExprBasePtr rhs)
        : ExprBase(NodeKind::BinOpExpr, std::move(p)),
          op(std::move(oper)),
          left(std::move(lhs)),
          right(std::move(rhs)) {}
};

struct SetExprNode : ExprBase {
    ExprBasePtr lhs;
    ExprBasePtr rhs;

    SetExprNode(CodePos p, ExprBasePtr l, ExprBasePtr r)
        : ExprBase(NodeKind::SetExpr, std::move(p)), lhs(std::move(l)), rhs(std::move(r)) {}
};

struct InvokeExprNode : ExprBase {
    std::string calleeName;
    std::vector<ExprBasePtr> arguments;

    InvokeExprNode(CodePos p, std::string fn, std::vector<ExprBasePtr> args)
        : ExprBase(NodeKind::InvokeExpr, std::move(p)),
          calleeName(std::move(fn)),
          arguments(std::move(args)) {}
};

struct SubscriptExprNode : ExprBase {
    ExprBasePtr baseArr;
    ExprBasePtr indexExpr;

    SubscriptExprNode(CodePos p, ExprBasePtr arr, ExprBasePtr idx)
        : ExprBase(NodeKind::SubscriptExpr, std::move(p)),
          baseArr(std::move(arr)),
          indexExpr(std::move(idx)) {}
};

} // namespace bm

#endif // BM_AST_HPP