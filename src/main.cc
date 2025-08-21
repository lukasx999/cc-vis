#include <print>
#include <string>
#include <ranges>
#include <numeric>
#include <memory>
#include <vector>
#include <cmath>

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Execution.h>
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
concept ClangASTNode =
std::is_same_v<T, clang::Stmt*> ||
std::is_same_v<T, clang::Type*> ||
std::is_same_v<T, clang::Decl*>;

class ASTRenderer : public clang::ASTConsumer {
    clang::ASTContext& m_ctx;
    static constexpr rl::Vector2 m_new_node_offset { 0, 100 };
    static constexpr rl::Color m_stmt_color = rl::RED;
    static constexpr rl::Color m_decl_color = rl::BLUE;
    static constexpr rl::Color m_line_color = rl::GRAY;
    static constexpr float m_line_thickness = 2.0;

public:
    ASTRenderer(clang::ASTContext& ctx) : m_ctx(ctx) { }

    void HandleTranslationUnit(clang::ASTContext&) override {
        auto decl = m_ctx.getTranslationUnitDecl();
        handle_decl(decl, RenderState{});
    }

private:
    struct RenderState {
        rl::Vector2 pos {
            rl::GetScreenWidth()  / 2.0f,
            rl::GetScreenHeight() / 4.0f,
        };
        float spacing = 200.0;
        float radius = 10.0;
        RenderState() = default;
    };

    enum class Direction { Left, Right };

    template <ClangASTNode Node>
    using NodeHandler = std::function<void(Node, RenderState)>;

    template <ClangASTNode Node>
    static void render(rl::Vector2 offset, RenderState state, Node node, NodeHandler<Node> fn) {

#if 0
        rl::DrawLineEx(state.pos, state.pos + offset, m_line_thickness, m_line_color);
#else
        std::array points {
            state.pos,
            state.pos + rl::Vector2{offset.x / 2.0f, offset.y * 0.75f},
            state.pos + rl::Vector2{offset.x / 2.0f, offset.y * 0.25f},
            state.pos + offset
        };
        static_assert(points.size() == 4);
        rl::DrawSplineBezierCubic(points.data(), points.size(), m_line_thickness, m_line_color);
#endif

        state.pos += offset;
        state.spacing /= 2;
        state.radius *= 0.9;
        fn(node, state);
    }

    template <ClangASTNode Node>
    static void recur(RenderState state, Node node, size_t idx, Direction dir, NodeHandler<Node> fn) {

        int sign = [&] {
            switch (dir) {
                using enum Direction;
                case Left:  return -1;
                case Right: return  1;
            }
        }();

        rl::Vector2 offset = m_new_node_offset;
        offset.x = sign * state.spacing * (idx + 1);

        render(offset, state, node, fn);
    }

    template <ClangASTNode Node>
    static void recur_without_offset(RenderState state, Node node, NodeHandler<Node> fn) {
        render(m_new_node_offset, state, node, fn);
    }

    template <ClangASTNode Node>
    static void handle_nodes_even(RenderState state, std::span<Node> nodes, NodeHandler<Node> fn) {

        // slice the children list in half, first rendering the left side, then the right side
        size_t middle = nodes.size() / 2;

        for (auto&& [idx, child] : nodes | views::take(middle) | views::enumerate)
        recur(state, child, idx, Direction::Left, fn);

        for (auto&& [idx, child] : nodes | views::drop(middle) | views::enumerate)
        recur(state, child, idx, Direction::Right, fn);

    }

    template <ClangASTNode Node>
    static void handle_nodes_odd(RenderState state, std::span<Node> nodes, NodeHandler<Node> fn) {

        auto nums = views::iota(1ul, nodes.size()+1);
        int sum = std::accumulate(nums.begin(), nums.end(), 0);
        int middle = sum / nodes.size();

        recur_without_offset(state, nodes[middle], fn);

        for (auto&& [idx, child] : nodes | views::take(middle-1) | views::enumerate)
        recur(state, child, idx, Direction::Left, fn);

        for (auto&& [idx, child] : nodes | views::drop(middle) | views::enumerate)
        recur(state, child, idx, Direction::Right, fn);

    }

    // calls the given callback for all child nodes, calculating the correct position on the scren for each one
    // TODO: put this outside of the class
    template <ClangASTNode Node>
    static void handle_children(RenderState state, std::span<Node> children, NodeHandler<Node> fn) {

        if (children.empty()) return;

        if (children.size() == 1) {
            recur_without_offset(state, children.front(), fn);

        } else if (children.size() % 2 == 0) {
            handle_nodes_even(state, children, fn);

        } else {
            handle_nodes_odd(state, children, fn);
        }

    }

    void handle_decl(clang::Decl* decl, RenderState state) {

        if (auto decl_ctx = llvm::dyn_cast_or_null<clang::DeclContext>(decl))
            handle_decl_ctx(decl_ctx, state);

        if (decl->hasBody()) {

            // BUG: renders two new nodes (see probe.cc)
            // functiondecl is both declctx and has body
            auto thunk = [&](clang::Stmt* stmt, RenderState state) {
                handle_stmt(stmt, state);
            };

            recur_without_offset<clang::Stmt*>(state, decl->getBody(), thunk);

            // handle_stmt(decl->getBody(), state);
        }

        if (llvm::isa<clang::FunctionDecl>(decl))
            render_node(state, rl::PURPLE);
        else
            render_node(state, m_decl_color);
    }

    void handle_decl_ctx(clang::DeclContext* decl_ctx, RenderState state) {

        std::vector<clang::Decl*> children;
        for (auto& child : decl_ctx->decls()) {
            // skip implementation generated nodes
            if (child->isImplicit()) continue;
            children.push_back(child);
        }

        auto thunk = [&](clang::Decl* decl, RenderState state) {
            handle_decl(decl, state);
        };

        handle_children<clang::Decl*>(state, children, thunk);
    }

    void handle_decl_stmt(clang::DeclStmt* decl_stmt, RenderState state) {

        std::vector<clang::Decl*> children;
        for (auto& child : decl_stmt->decls()) {
            // skip implementation generated nodes
            if (child->isImplicit()) continue;
            children.push_back(child);
        }

        auto thunk = [&](clang::Decl* decl, RenderState state) {
            handle_decl(decl, state);
        };

        handle_children<clang::Decl*>(state, children, thunk);
    }

    void handle_stmt(clang::Stmt* stmt, RenderState state) {

        std::vector<clang::Stmt*> children;

        for (auto& child : stmt->children()) {
            children.push_back(child);
        }

        auto thunk = [&](clang::Stmt* stmt, RenderState state) {
            handle_stmt(stmt, state);
        };

        handle_children<clang::Stmt*>(state, children, thunk);

        if (auto decl_stmt = llvm::dyn_cast_or_null<clang::DeclStmt>(stmt))
            handle_decl_stmt(decl_stmt, state);

        // draw call down here, so line doesnt get rendered above circles
        render_node(state, m_stmt_color);
    }

    void render_node(const RenderState& state, rl::Color color) {
        rl::DrawCircleV(state.pos, state.radius, color);
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

    // TODO: use backend-agnostic interface (png, pdf, raylib, web)

    std::string error;
    // TODO: get default compilation database
    auto db = tooling::CompilationDatabase::autoDetectFromSource("compile_flags.txt", error);
    std::println(stderr, "{}", error);

    std::vector<std::string> sources{"probe.cc"};
    tooling::ClangTool tool(*db, sources);

    rl::SetTraceLogLevel(rl::LOG_ERROR);
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
