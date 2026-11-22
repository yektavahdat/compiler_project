#include "bm_colorizer.hpp"
#include "bm_parser.hpp"
#include "bm_scanner.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string readFileText(const std::string& path, bool& ok) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in.is_open()) {
        ok = false;
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    ok = true;
    return ss.str();
}

void printUsage() {
    std::cout
        << "Usage: bm_front <command> <source_file> [options]\n"
        << "Commands:\n"
        << "  --scan       Run lexical scanner and list tokens\n"
        << "  --parse      Run lexical scanner + LL recursive-descent parser\n"
        << "  --highlight  Render syntax-highlighted output (ANSI or --html <out.html>)\n"
        << "Options:\n"
        << "  --html <file>  Write standalone HTML/CSS document to <file>\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage();
        return 1;
    }

    std::string mode = argv[1];
    std::string inputPath = argv[2];
    std::string htmlOutPath;

    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--html" && i + 1 < argc) {
            htmlOutPath = argv[++i];
        }
    }

    bool fileOk = false;
    std::string source = readFileText(inputPath, fileOk);
    if (!fileOk) {
        std::cerr << "Error: cannot open input file '" << inputPath << "'\n";
        return 1;
    }

    bm::Scanner scanner(source, inputPath);
    auto tokens = scanner.scanAll();

    for (const auto& diag : scanner.diagnostics()) {
        std::cerr << diag.pos.file << ":" << diag.pos.line << ":" << diag.pos.col
                  << ": lexical warning/error: " << diag.detail << "\n";
    }

    if (mode == "--scan") {
        for (const auto& tok : tokens) {
            std::cout << tok.pos.line << ":" << tok.pos.col << "\t"
                      << bm::symKindName(tok.kind) << "\t" << tok.text << "\n";
        }
        return scanner.diagnostics().empty() ? 0 : 2;
    }

    bm::SyntaxParser parser(tokens);
    auto unit = parser.parseUnit();

    for (const auto& err : parser.issues()) {
        std::cerr << err.pos.file << ":" << err.pos.line << ":" << err.pos.col
                  << ": syntax error: " << err.message << "\n";
    }

    if (mode == "--parse") {
        std::cout << "Parsed top-level declarations: "
                  << (unit ? unit->topDecls.size() : 0)
                  << " | Syntax issues: " << parser.issues().size() << "\n";
        return (scanner.diagnostics().empty() && parser.issues().empty()) ? 0 : 2;
    }

    if (mode == "--highlight") {
        auto spans = bm::AstColorizer::collectSpans(unit.get(), tokens);
        if (!htmlOutPath.empty()) {
            std::ofstream out(htmlOutPath, std::ios::out | std::ios::binary);
            if (!out.is_open()) {
                std::cerr << "Error: cannot write HTML output to '" << htmlOutPath << "'\n";
                return 1;
            }
            out << bm::AstColorizer::renderHtml(source, spans, inputPath);
            std::cout << "Wrote HTML output to " << htmlOutPath << "\n";
        } else {
            std::cout << bm::AstColorizer::renderAnsi(source, spans);
        }
        return 0;
    }

    printUsage();
    return 1;
}