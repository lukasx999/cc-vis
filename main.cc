#include <print>
#include <string>

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/AST/AST.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>

namespace tooling = clang::tooling;

class Visitor : public clang::RecursiveASTVisitor<Visitor> {
    clang::ASTContext& m_ctx;

public:
    Visitor(clang::ASTContext& ctx) : m_ctx(ctx) { }

    bool VisitDecl(clang::Decl* decl) {
        (void) decl;
        std::println("decl");
        return true;
    }

    bool VisitStmt(clang::Stmt* stmt) {
        (void) stmt;
        std::println("stmt");
        return true;
    }

    bool VisitFunctionDecl(clang::FunctionDecl* decl) {
        std::println("func: {}", decl->getNameAsString());
        return true;
    }

};

class Consumer : public clang::ASTConsumer {

public:
    Consumer() = default;

    void HandleTranslationUnit(clang::ASTContext& ctx) override {
        Visitor visitor(ctx);
        auto decl = ctx.getTranslationUnitDecl();
        visitor.TraverseDecl(decl);
    }

};

class Action : public clang::ASTFrontendAction {
public:
    Action() = default;

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        [[maybe_unused]] clang::CompilerInstance &inst,
        [[maybe_unused]] llvm::StringRef in_file
    ) override {
        return std::make_unique<Consumer>();
    }

};

int main() {

    std::string error;
    auto db = tooling::CompilationDatabase::autoDetectFromSource("compile_flags.txt", error);
    std::println(stderr, "{}", error);

    std::vector<std::string> sources{"foo.cc"};
    tooling::ClangTool tool(*db, sources);

    tool.run(tooling::newFrontendActionFactory<Action>().get());

}
