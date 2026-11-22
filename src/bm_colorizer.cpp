#include "bm_colorizer.hpp"
#include <algorithm>
#include <sstream>

namespace bm {

std::string AstColorizer::spanKey(int line, int col) {
    return std::to_string(line) + ":" + std::to_string(col);
}

void AstColorizer::walkStmt(
    const StmtBase* stmt,
    std::unordered_map<std::string, HighlightStyle>& overrides) {
    if (!stmt) return;

    switch (stmt->kind) {
        case NodeKind::Suite: {
            const auto* s = static_cast<const SuiteNode*>(stmt);
            for (const auto& item : s->items) {
                walkStmt(item.get(), overrides);
            }
            break;
        }
        case NodeKind::CondBranch: {
            const auto* c = static_cast<const CondBranchNode*>(stmt);
            walkExpr(c->testExpr.get(), overrides);
            walkStmt(c->ifSuite.get(), overrides);
            walkStmt(c->elseSuite.get(), overrides);
            break;
        }
        case NodeKind::WhileLoop: {
            const auto* w = static_cast<const WhileLoopNode*>(stmt);
            walkExpr(w->testExpr.get(), overrides);
            walkStmt(w->loopBody.get(), overrides);
            break;
        }
        case NodeKind::ForLoop: {
            const auto* f = static_cast<const ForLoopNode*>(stmt);
            walkExpr(f->startExpr.get(), overrides);
            walkExpr(f->stopExpr.get(), overrides);
            walkExpr(f->stepExpr.get(), overrides);
            walkStmt(f->loopBody.get(), overrides);
            break;
        }
        case NodeKind::RetStmt: {
            const auto* r = static_cast<const RetStmtNode*>(stmt);
            walkExpr(r->retExpr.get(), overrides);
            break;
        }
        case NodeKind::ExprStmt: {
            const auto* e = static_cast<const ExprStmtNode*>(stmt);
            walkExpr(e->expr.get(), overrides);
            break;
        }
        default:
            break;
    }
}

void AstColorizer::walkExpr(
    const ExprBase* expr,
    std::unordered_map<std::string, HighlightStyle>& overrides) {
    if (!expr) return;

    switch (expr->kind) {
        case NodeKind::IdentExpr: {
            const auto* id = static_cast<const IdentExprNode*>(expr);
            overrides[spanKey(id->pos.line, id->pos.col)] = HighlightStyle::IdentName;
            break;
        }
        case NodeKind::InvokeExpr: {
            const auto* call = static_cast<const InvokeExprNode*>(expr);
            overrides[spanKey(call->pos.line, call->pos.col)] = HighlightStyle::RoutineName;
            for (const auto& arg : call->arguments) {
                walkExpr(arg.get(), overrides);
            }
            break;
        }
        case NodeKind::SubscriptExpr: {
            const auto* sub = static_cast<const SubscriptExprNode*>(expr);
            walkExpr(sub->baseArr.get(), overrides);
            walkExpr(sub->indexExpr.get(), overrides);
            break;
        }
        case NodeKind::PreUnaryExpr: {
            const auto* u = static_cast<const PreUnaryExprNode*>(expr);
            walkExpr(u->target.get(), overrides);
            break;
        }
        case NodeKind::PostUnaryExpr: {
            const auto* u = static_cast<const PostUnaryExprNode*>(expr);
            walkExpr(u->target.get(), overrides);
            break;
        }
        case NodeKind::BinOpExpr: {
            const auto* b = static_cast<const BinOpExprNode*>(expr);
            walkExpr(b->left.get(), overrides);
            walkExpr(b->right.get(), overrides);
            break;
        }
        case NodeKind::SetExpr: {
            const auto* s = static_cast<const SetExprNode*>(expr);
            walkExpr(s->lhs.get(), overrides);
            walkExpr(s->rhs.get(), overrides);
            break;
        }
        default:
            break;
    }
}

std::vector<SpanTag> AstColorizer::collectSpans(
    const UnitNode* unit,
    const std::vector<LexemeItem>& tokens) {
    std::unordered_map<std::string, HighlightStyle> astOverrides;

    if (unit) {
        for (const auto& decl : unit->topDecls) {
            if (!decl) continue;
            if (decl->kind == NodeKind::FuncDef) {
                const auto* fn = static_cast<const FuncDefNode*>(decl.get());
                astOverrides[spanKey(fn->pos.line, fn->pos.col)] = HighlightStyle::RoutineName;
                for (const auto& p : fn->params) {
                    astOverrides[spanKey(p.pos.line, p.pos.col)] = HighlightStyle::IdentName;
                }
                walkStmt(fn->body.get(), astOverrides);
            } else if (decl->kind == NodeKind::VarDef) {
                const auto* v = static_cast<const VarDefNode*>(decl.get());
                astOverrides[spanKey(v->pos.line, v->pos.col)] = HighlightStyle::IdentName;
                walkExpr(v->initVal.get(), astOverrides);
            }
        }
    }

    std::vector<SpanTag> result;
    for (const auto& tok : tokens) {
        if (tok.kind == SymKind::SYM_EOF) continue;

        HighlightStyle st = HighlightStyle::Plain;
        switch (tok.kind) {
            case SymKind::SYM_INTEGER:
            case SymKind::SYM_BOOLEAN:
            case SymKind::SYM_CHAR:
            case SymKind::SYM_STRING:
            case SymKind::SYM_FLOAT:
            case SymKind::SYM_VOID:
            case SymKind::SYM_AUTO:
            case SymKind::SYM_ARRAY:
                st = HighlightStyle::TypeKw;
                break;

            case SymKind::SYM_ELSE:
            case SymKind::SYM_FALSE:
            case SymKind::SYM_FOR:
            case SymKind::SYM_FUNCTION:
            case SymKind::SYM_IF:
            case SymKind::SYM_RETURN:
            case SymKind::SYM_TRUE:
            case SymKind::SYM_WHILE:
                st = HighlightStyle::DirectiveKw;
                break;

            case SymKind::SYM_INT_LIT:
            case SymKind::SYM_FLOAT_LIT:
                st = HighlightStyle::NumConst;
                break;

            case SymKind::SYM_CHAR_LIT:
            case SymKind::SYM_STR_LIT:
                st = HighlightStyle::TextConst;
                break;

            case SymKind::SYM_IDENT:
                st = HighlightStyle::IdentName;
                break;

            case SymKind::SYM_LPAREN:
            case SymKind::SYM_RPAREN:
            case SymKind::SYM_LBRACE:
            case SymKind::SYM_RBRACE:
            case SymKind::SYM_LBRACK:
            case SymKind::SYM_RBRACK:
            case SymKind::SYM_SEMI:
            case SymKind::SYM_COLON:
            case SymKind::SYM_COMMA:
                st = HighlightStyle::BracePunct;
                break;

            case SymKind::SYM_BAD:
                st = HighlightStyle::ErrHighlight;
                break;

            default:
                st = HighlightStyle::OpSym;
                break;
        }

        auto hit = astOverrides.find(spanKey(tok.pos.line, tok.pos.col));
        if (hit != astOverrides.end() && st != HighlightStyle::ErrHighlight) {
            st = hit->second;
        }

        result.push_back(SpanTag{
            tok.pos.line,
            tok.pos.col,
            static_cast<int>(tok.text.size()),
            st
        });
    }

    return result;
}

const char* AstColorizer::ansiCode(HighlightStyle style) {
    switch (style) {
        case HighlightStyle::DirectiveKw:  return "\033[1;35m";
        case HighlightStyle::TypeKw:       return "\033[1;36m";
        case HighlightStyle::IdentName:    return "\033[0;37m";
        case HighlightStyle::RoutineName:  return "\033[1;33m";
        case HighlightStyle::NumConst:     return "\033[0;32m";
        case HighlightStyle::TextConst:    return "\033[0;31m";
        case HighlightStyle::OpSym:        return "\033[1;34m";
        case HighlightStyle::BracePunct: return "\033[0;90m";
        case HighlightStyle::ErrHighlight: return "\033[1;41;97m";
        case HighlightStyle::Plain:
        default:                           return "\033[0m";
    }
}

const char* AstColorizer::htmlClass(HighlightStyle style) {
    switch (style) {
        case HighlightStyle::DirectiveKw:  return "bm-kw";
        case HighlightStyle::TypeKw:       return "bm-ty";
        case HighlightStyle::IdentName:    return "bm-id";
        case HighlightStyle::RoutineName:  return "bm-fn";
        case HighlightStyle::NumConst:     return "bm-num";
        case HighlightStyle::TextConst:    return "bm-str";
        case HighlightStyle::OpSym:        return "bm-op";
        case HighlightStyle::BracePunct: return "bm-punc";
        case HighlightStyle::ErrHighlight: return "bm-err";
        case HighlightStyle::Plain:
        default:                           return "bm-plain";
    }
}

std::string AstColorizer::escapeHtml(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (char c : raw) {
        switch (c) {
            case '&':  out += "&amp;"; break;
            case '<':  out += "&lt;"; break;
            case '>':  out += "&gt;"; break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default:   out.push_back(c); break;
        }
    }
    return out;
}

std::string AstColorizer::renderAnsi(
    const std::string& source,
    const std::vector<SpanTag>& spans) {
    std::vector<std::vector<const SpanTag*>> lineSpans(2);
    int curLine = 1;
    int curCol = 1;
    size_t idx = 0;

    std::unordered_map<std::string, const SpanTag*> spanMap;
    for (const auto& sp : spans) {
        spanMap[spanKey(sp.line, sp.col)] = &sp;
    }

    std::ostringstream out;
    while (idx < source.size()) {
        auto found = spanMap.find(spanKey(curLine, curCol));
        if (found != spanMap.end() && found->second->len > 0) {
            const SpanTag* sp = found->second;
            out << ansiCode(sp->style);
            for (int k = 0; k < sp->len && idx < source.size(); ++k) {
                char ch = source[idx++];
                out << ch;
                if (ch == '\n') {
                    curLine++;
                    curCol = 1;
                } else {
                    curCol++;
                }
            }
            out << "\033[0m";
        } else {
            char ch = source[idx++];
            out << ch;
            if (ch == '\n') {
                curLine++;
                curCol = 1;
            } else {
                curCol++;
            }
        }
    }
    return out.str();
}

std::string AstColorizer::renderHtml(
    const std::string& source,
    const std::vector<SpanTag>& spans,
    const std::string& pageTitle) {
    std::unordered_map<std::string, const SpanTag*> spanMap;
    for (const auto& sp : spans) {
        spanMap[spanKey(sp.line, sp.col)] = &sp;
    }

    std::ostringstream codeBuf;
    int curLine = 1;
    int curCol = 1;
    size_t idx = 0;

    while (idx < source.size()) {
        auto found = spanMap.find(spanKey(curLine, curCol));
        if (found != spanMap.end() && found->second->len > 0) {
            const SpanTag* sp = found->second;
            std::string lex;
            for (int k = 0; k < sp->len && idx < source.size(); ++k) {
                char ch = source[idx++];
                lex.push_back(ch);
                if (ch == '\n') {
                    curLine++;
                    curCol = 1;
                } else {
                    curCol++;
                }
            }
            codeBuf << "<span class=\"" << htmlClass(sp->style) << "\">"
                    << escapeHtml(lex) << "</span>";
        } else {
            char ch = source[idx++];
            codeBuf << escapeHtml(std::string(1, ch));
            if (ch == '\n') {
                curLine++;
                curCol = 1;
            } else {
                curCol++;
            }
        }
    }

    std::ostringstream doc;
    doc << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\">\n"
        << "<title>" << escapeHtml(pageTitle) << "</title>\n<style>\n"
        << "body { background: #14171f; color: #e6edf3; font-family: Consolas, monospace; padding: 24px; }\n"
        << "pre { line-height: 1.55; font-size: 14px; }\n"
        << ".bm-kw { color: #ff7b72; font-weight: 600; }\n"
        << ".bm-ty { color: #79c0ff; font-weight: 600; }\n"
        << ".bm-id { color: #e6edf3; }\n"
        << ".bm-fn { color: #d2a8ff; font-weight: 600; }\n"
        << ".bm-num { color: #a5d6ff; }\n"
        << ".bm-str { color: #a5d6ff; }\n"
        << ".bm-op { color: #ffa657; }\n"
        << ".bm-punc { color: #8b949e; }\n"
        << ".bm-err { background: #da3633; color: #ffffff; padding: 1px 3px; border-radius: 3px; }\n"
        << "</style>\n</head>\n<body>\n<pre><code>"
        << codeBuf.str()
        << "</code></pre>\n</body>\n</html>\n";
    return doc.str();
}

} // namespace bm