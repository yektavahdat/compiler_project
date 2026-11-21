#ifndef BM_SCANNER_HPP
#define BM_SCANNER_HPP

#include "bm_token.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace bm {

struct ScanDiagnostic {
    CodePos pos;
    std::string detail;
};

class Scanner {
public:
    Scanner(std::string input, std::string fileName = "<input>");

    std::vector<LexemeItem> scanAll();
    const std::vector<ScanDiagnostic>& diagnostics() const { return diagnostics_; }

private:
    std::string input_;
    std::string fileName_;
    size_t cursor_ = 0;
    int line_ = 1;
    int col_ = 1;

    std::vector<ScanDiagnostic> diagnostics_;
    static const std::unordered_map<std::string, SymKind> reservedMap_;

    bool atEnd() const;
    char lookAhead(size_t ahead = 0) const;
    char consume();
    CodePos currentPos() const;

    void skipIgnored();
    void recordDiag(const CodePos& pos, const std::string& detail);

    LexemeItem readWordOrKeyword();
    LexemeItem readNumeric();
    LexemeItem readQuotedString();
    LexemeItem readCharConst();
    LexemeItem readSymbol();
};

} // namespace bm

#endif // BM_SCANNER_HPP