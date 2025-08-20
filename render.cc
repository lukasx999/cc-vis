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
    static constexpr rl::Vector2 m_node_offset { 0, 100 };
    static constexpr float m_node_radius = 10.0;
    static constexpr rl::Color m_node_color = rl::BLUE;
    static constexpr rl::Color m_line_color = rl::GRAY;
    static constexpr float m_spacing = 200.0;

public:
    TreeRenderer() = default;

    void render(const Node& root, rl::Vector2 origin) {
        render_rec(root, origin, m_spacing);
    }

private:
    void recur_with_offset(const Node& child, rl::Vector2 pos, int idx, float spacing, bool right_else_left) {
        rl::Vector2 offset = m_node_offset;
        int sign = right_else_left ? 1 : -1;
        offset.x = sign * spacing * (idx+1);
        rl::DrawLineV(pos, pos+offset, m_line_color);
        render_rec(child, pos+offset, spacing/2);
    }

    void recur(const Node& child, rl::Vector2 pos, float spacing) {
        rl::Vector2 offset = pos+m_node_offset;
        rl::DrawLineV(pos, offset, m_line_color);
        render_rec(child, offset, spacing/2);
    }

    void render_rec(const Node& root, rl::Vector2 pos, float spacing) {

        rl::DrawCircleV(pos, m_node_radius, m_node_color);

        size_t child_count = root.children.size();

        if (child_count % 2 == 0) {

            auto middle = child_count/2;

            for (auto&& [idx, child] : root.children | views::take(middle) | views::enumerate) {
                recur_with_offset(*child, pos, idx, spacing, false);
            }

            for (auto&& [idx, child] : root.children | views::drop(middle) | views::enumerate) {
                recur_with_offset(*child, pos, idx, spacing, true);
            }

        } else {

            auto nums = views::iota(1ul, child_count+1);
            int sum = std::accumulate(nums.begin(), nums.end(), 0);
            int middle = sum/child_count;

            recur(*root.children[middle], pos, spacing);

            for (auto&& [idx, child] : root.children | views::take(middle-1) | views::enumerate) {
                recur_with_offset(*child, pos, idx, spacing, false);
            }

            for (auto&& [idx, child] : root.children | views::drop(middle) | views::enumerate) {
                recur_with_offset(*child, pos, idx, spacing, true);
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

        rl::Vector2 origin {
            rl::GetScreenWidth()/2.0f,
            rl::GetScreenHeight()/4.0f,
        };

        rd.render(*root, origin);

        rl::EndDrawing();
    }

    rl::CloseWindow();

    return 0;
}
