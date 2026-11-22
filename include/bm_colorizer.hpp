#ifndef BM_COLORIZER_HPP
#define BM_COLORIZER_HPP

#include "bm_ast.hpp"
#include "bm_token.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace bm {

enum class HighlightStyle {
    Plain,
    DirectiveKw,
    TypeKw,
    IdentName,
    RoutineName,
    NumConst,
    TextConst,
    OpSym,
    BracePunct,
    ErrHighlight
};

struct SpanTag {
    int line = 1;
    int col = 1;
    int len = 0;
    HighlightStyle style = HighlightStyle::Plain;
};

class AstColorizer {
public:
    static std::vector<SpanTag> collectSpans(
        const UnitNode* unit,
        const std::vector<LexemeItem>& tokens);

    static std::string renderAnsi(
        const std::string& source,
        const std::vector<SpanTag>& spans);

    static std::string renderHtml(
        const std::string& source,
        const std::vector<SpanTag>& spans,
        const std::string& pageTitle = "B-Minor Highlighted Output");

private:
    static std::string spanKey(int line, int col);
    static void walkStmt(
        const StmtBase* stmt,
        std::unordered_map<std::string, HighlightStyle>& overrides);
    static void walkExpr(
        const ExprBase* expr,
        std::unordered_map<std::string, HighlightStyle>& overrides);
    static const char* ansiCode(HighlightStyle style);
    static const char* htmlClass(HighlightStyle style);
    static std::string escapeHtml(const std::string& raw);
};

} // namespace bm

#endif // BM_COLORIZER_HPP