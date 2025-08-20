#include <algorithm>
#include <numeric>
#include <print>
#include <memory>
#include <vector>
#include <ranges>
#include <cmath>

namespace rl {
#include <raylib.h>
#include <raymath.h>
}

namespace views = std::views;

struct Node {
    std::vector<std::unique_ptr<Node>> children;
};

class TreeRenderer {
public:
    TreeRenderer() = default;

    void render(const Node& root) {

        rl::Vector2 origin {
            rl::GetScreenWidth()/2.0f,
            rl::GetScreenHeight()/4.0f,
        };

        render_rec(root, origin, 100);
    }

private:
    void recur(const Node& child, rl::Vector2 pos, int idx, float spacing, bool right_else_left) {
        rl::Vector2 offset { 0, 50 };
        rl::Vector2 offset_cpy = offset;
        int sign = right_else_left ? 1 : -1;
        offset_cpy.x = sign * spacing * (idx+1);
        rl::DrawLineV(pos, pos+offset_cpy, rl::GRAY);
        render_rec(child, pos+offset_cpy, spacing/2);
    }

    void render_rec(const Node& root, rl::Vector2 pos, float spacing) {

        rl::DrawCircleV(pos, 5.0, rl::BLUE);
        rl::Vector2 offset { 0, 50 };

        size_t child_count = root.children.size();

        if (child_count % 2 == 0) {

            auto middle = child_count/2;

            for (auto&& [idx, child] : root.children | views::take(middle) | views::enumerate) {
                recur(*child, pos, idx, spacing, false);
            }

            for (auto&& [idx, child] : root.children | views::drop(middle) | views::enumerate) {
                recur(*child, pos, idx, spacing, true);
            }

        } else {

            auto nums = views::iota(1ul, child_count+1);
            int sum = std::accumulate(nums.begin(), nums.end(), 0);
            int middle = sum/child_count;

            rl::DrawLineV(pos, pos+offset, rl::GRAY);
            render_rec(*root.children[middle], pos+offset, spacing/2);

            for (auto&& [idx, child] : root.children | views::take(middle-1) | views::enumerate) {
                recur(*child, pos, idx, spacing, false);
            }

            for (auto&& [idx, child] : root.children | views::drop(middle) | views::enumerate) {
                recur(*child, pos, idx, spacing, true);
            }

        }

    }

};





int main() {

    auto ax = std::make_unique<Node>();
    auto bx = std::make_unique<Node>();
    auto cx = std::make_unique<Node>();
    auto ay = std::make_unique<Node>();
    auto by = std::make_unique<Node>();
    auto cy = std::make_unique<Node>();
    auto a = std::make_unique<Node>();
    auto b = std::make_unique<Node>();
    auto c = std::make_unique<Node>();
    auto d = std::make_unique<Node>();
    auto root = std::make_unique<Node>();
    a->children.push_back(std::move(ax));
    a->children.push_back(std::move(ay));
    b->children.push_back(std::move(bx));
    b->children.push_back(std::move(by));
    c->children.push_back(std::move(cx));
    c->children.push_back(std::move(cy));
    root->children.push_back(std::move(a));
    root->children.push_back(std::move(b));
    root->children.push_back(std::move(c));
    // root->children.push_back(std::move(d));

    TreeRenderer rd;
    rl::InitWindow(1600, 900, "cc-vis");

    while (!rl::WindowShouldClose()) {
        rl::BeginDrawing();

        rl::ClearBackground(rl::BLACK);
        rd.render(*root);

        rl::EndDrawing();
    }

    rl::CloseWindow();

    return 0;
}
