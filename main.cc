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

template <typename T>
concept ast_node = std::is_same_v<T, clang::Stmt*> ||
                   std::is_same_v<T, clang::Decl*>;

class ASTRenderer : public clang::ASTConsumer {
    clang::ASTContext& m_ctx;
    static constexpr rl::Vector2 m_new_node_offset { 0, 100 };
    static constexpr float m_spacing = 200.0;
    static constexpr rl::Color m_stmt_color = rl::RED;
    static constexpr rl::Color m_decl_color = rl::BLUE;
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

    enum class Direction { Left, Right };

    template <ast_node Node>
    using NodeHandler = std::function<void(Node, State)>;

    template <ast_node Node>
    void recur(State state, Node node, size_t idx, Direction dir, NodeHandler<Node> fn) {

        int sign = [&] {
            switch (dir) {
                using enum Direction;
                case Left: return -1;
                case Right: return 1;
            }
        }();

        rl::Vector2 offset = m_new_node_offset;
        offset.x = sign * state.spacing * (idx+1);
        rl::DrawLineV(state.pos, state.pos + offset, m_line_color);

        state.pos += offset;
        state.spacing /= 2;
        fn(node, state);
    }

    template <ast_node Node>
    void recur_without_offset(State state, Node node, NodeHandler<Node> fn) {
        rl::DrawLineV(state.pos, state.pos + m_new_node_offset, m_line_color);

        state.pos += m_new_node_offset;
        state.spacing /= 2;
        fn(node, state);
    }

    template <ast_node Node>
    void handle_nodes_even(State state, std::span<Node> nodes, NodeHandler<Node> fn) {

        size_t middle = nodes.size() / 2;

        for (auto&& [idx, child] : nodes | views::take(middle) | views::enumerate)
            recur(state, child, idx, Direction::Left, fn);

        for (auto&& [idx, child] : nodes | views::drop(middle) | views::enumerate)
            recur(state, child, idx, Direction::Right, fn);

    }

    template <ast_node Node>
    void handle_nodes_odd(State state, std::span<Node> nodes, NodeHandler<Node> fn) {

        auto nums = views::iota(1ul, nodes.size()+1);
        int sum = std::accumulate(nums.begin(), nums.end(), 0);
        int middle = sum / nodes.size();

        recur_without_offset(state, nodes[middle], fn);

        for (auto&& [idx, child] : nodes | views::take(middle-1) | views::enumerate)
            recur(state, child, idx, Direction::Left, fn);

        for (auto&& [idx, child] : nodes | views::drop(middle) | views::enumerate)
            recur(state, child, idx, Direction::Right, fn);

    }

    template <ast_node Node>
    void handle_children(State state, std::span<Node> children, NodeHandler<Node> fn) {

        if (children.size() == 1) {
            recur_without_offset(state, children[0], fn);

        } else if (children.size() % 2 == 0) {
            handle_nodes_even(state, children, fn);

        } else {
            handle_nodes_odd(state, children, fn);
        }

    }

    void handle_decl(clang::Decl* decl, State state) {

        rl::DrawCircleV(state.pos, m_node_radius, m_decl_color);

        auto decl_ctx = llvm::dyn_cast_or_null<clang::DeclContext>(decl);
        if (decl_ctx != nullptr) {
            for (auto& decl : decl_ctx->decls())
                handle_decl(decl, state);
        }

        if (decl->hasBody())
            handle_stmt(decl->getBody(), state);

    }

    void handle_stmt(clang::Stmt* stmt, State state) {

        std::vector<clang::Stmt*> children;
        for (auto& child : stmt->children()) {
            children.push_back(child);
        }

        auto thunk = [&](clang::Stmt* stmt, State state) {
            handle_stmt(stmt, state);
        };

        handle_children<clang::Stmt*>(state, children, thunk);

        // draw call down here, so line dont get rendered above circles
        rl::DrawCircleV(state.pos, m_node_radius, m_stmt_color);

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
