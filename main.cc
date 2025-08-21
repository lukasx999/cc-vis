#include <print>
#include <string>
#include <cmath>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <memory>
#include <vector>

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/AST/AST.h>
#include <clang/AST/Expr.h>

namespace rl {
#include <raylib.h>
#include <raymath.h>
}

namespace tooling = clang::tooling;
namespace views = std::views;

class ASTRenderer : public clang::ASTConsumer {
    clang::ASTContext& m_ctx;
    rl::Vector2 m_new_node_offset { 0, 100 };
    static constexpr float m_spacing = 200.0;
    static constexpr rl::Color m_node_color = rl::BLUE;
    static constexpr rl::Color m_line_color = rl::GRAY;
    static constexpr float m_node_radius = 10.0;

public:
    ASTRenderer(clang::ASTContext& ctx) : m_ctx(ctx) { }

    void HandleTranslationUnit(clang::ASTContext& ctx) override {
        auto decl = ctx.getTranslationUnitDecl();

        rl::Vector2 origin {
            rl::GetScreenWidth()/2.0f,
            rl::GetScreenHeight()/4.0f,
        };

        State state { origin, m_spacing };
        handle_decl(decl, state);
    }

private:
    struct State {
        rl::Vector2 pos;
        float spacing;
    };

    enum class Direction {
        Left, Right
    };

    void handle_decl(clang::Decl* decl, State state) {

        rl::DrawCircleV(state.pos, m_node_radius, m_node_color);

        auto decl_ctx = llvm::dyn_cast_or_null<clang::DeclContext>(decl);
        if (decl_ctx != nullptr) {
            for (auto& decl : decl_ctx->decls())
                handle_decl(decl, state);
        }

        if (decl->hasBody())
            handle_stmt(decl->getBody(), state);

    }

    void recur(State state, clang::Stmt* child, size_t idx, Direction dir) {

        int sign = [&] {
            switch (dir) {
                using enum Direction;
                case Left: return -1;
                case Right: return 1;
            }
        }();

        m_new_node_offset.x = sign * state.spacing * (idx+1);
        rl::DrawLineV(state.pos, state.pos+m_new_node_offset, m_line_color);

        state.pos += m_new_node_offset;
        state.spacing /= 2;
        handle_stmt(child, state);
    }

    void recur_without_offset(State state, clang::Stmt* child) {
        rl::DrawLineV(state.pos, state.pos+m_new_node_offset, m_line_color);
        state.pos += m_new_node_offset;
        state.spacing /= 2;
        handle_stmt(child, state);
    }

    void handle_stmt(clang::Stmt* stmt, State state) {

        rl::DrawCircleV(state.pos, m_node_radius, m_node_color);

        std::vector<clang::Stmt*> children;
        for (auto& child : stmt->children()) {
            children.push_back(child);
        }

        size_t child_count = children.size();

        if (child_count == 1) {
            recur_without_offset(state, children[0]);

        } else if (child_count % 2 == 0) {

            size_t middle = child_count/2;

            for (auto&& [idx, child] : children | views::take(middle) | views::enumerate) {
                recur(state, child, idx, Direction::Left);
            }

            for (auto&& [idx, child] : children | views::drop(middle) | views::enumerate) {
                recur(state, child, idx, Direction::Right);
            }

        } else {

            auto nums = views::iota(1ul, child_count+1);
            int sum = std::accumulate(nums.begin(), nums.end(), 0);
            int middle = sum/child_count;

            recur_without_offset(state, children[middle]);

            for (auto&& [idx, child] : children | views::take(middle-1) | views::enumerate) {
                recur(state, child, idx, Direction::Left);
            }

            for (auto&& [idx, child] : children | views::drop(middle) | views::enumerate) {
                recur(state, child, idx, Direction::Right);
            }

        }

    }

};

class ASTRendererAction : public clang::ASTFrontendAction {
public:
    ASTRendererAction() = default;

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &inst,
        [[maybe_unused]] llvm::StringRef in_file
    ) override {
        return std::make_unique<ASTRenderer>(inst.getASTContext());
    }

};

int main() {

    std::string error;
    auto db = tooling::CompilationDatabase::autoDetectFromSource("compile_flags.txt", error);
    std::println(stderr, "{}", error);

    std::vector<std::string> sources{"foo.cc"};
    tooling::ClangTool tool(*db, sources);

    rl::InitWindow(1600, 900, "cc-vis");

    while (!rl::WindowShouldClose()) {
        rl::BeginDrawing();
        {
            rl::ClearBackground(rl::BLACK);
            tool.run(tooling::newFrontendActionFactory<ASTRendererAction>().get());

        }
        rl::EndDrawing();
    }

    rl::CloseWindow();

}
