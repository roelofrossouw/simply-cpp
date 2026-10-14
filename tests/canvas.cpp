// sc::canvas turns every draw() into a few primitives; a canvas that records them shows which, with
// what, and what it leaves out.
#include <sc.h>

#include "sc_test.h"

#include <string>
#include <vector>

namespace {
    struct recording_canvas : sc::canvas {
        std::vector<std::string> calls;
        sc::polygon last_polygon;
        sc::color last_stroke, last_fill;
        double last_width = -1;

    protected:
        void paint_polygon(const sc::polygon &shape, const sc::color &stroke, const sc::color &fill, const double width) override {
            calls.push_back("polygon " + std::to_string(shape.size()));
            last_polygon = shape;
            last_stroke = stroke;
            last_fill = fill;
            last_width = width;
        }

        void paint_circle(const sc::circle &shape, const sc::color &stroke, const sc::color &fill, const double width) override {
            calls.push_back("circle " + std::to_string(static_cast<int>(shape.radius())));
            last_stroke = stroke;
            last_fill = fill;
            last_width = width;
        }

        void paint_line(const sc::point &, const sc::point &, const sc::color &, const double width) override {
            calls.push_back("line " + std::to_string(static_cast<int>(width)));
        }

        void paint_text(const sc::text &sample, const sc::color &) override {
            calls.push_back("text " + sample.content + " " + sample.font.family);
        }
    };
}

int main() {
    SECTION("Shapes become primitives");
    {
        recording_canvas canvas;
        canvas.draw(sc::rect{10, 20, 30, 40});
        CHECK_EQ(canvas.calls.back(), std::string{"polygon 4"});
        CHECK_EQ(canvas.last_polygon[2], (sc::point{40, 60}));
        CHECK(canvas.last_stroke == sc::color::Black); // the defaults: black outline, no fill, 1 wide
        CHECK(canvas.last_fill == sc::color::Transparent);
        CHECK_EQ(canvas.last_width, 1.0);

        canvas.draw(sc::rect_i{1, 2, 3, 4}, sc::color::Red, sc::color::Blue, 2);
        CHECK_EQ(canvas.last_polygon[0], (sc::point{1, 2}));
        CHECK(canvas.last_fill == sc::color::Blue);
        CHECK_EQ(canvas.last_width, 2.0);

        canvas.draw(sc::rotated_rect{{0, 0}, {2, 2}, 45});
        CHECK_NEAR(canvas.last_polygon[0].y(), -std::sqrt(2.0), 1e-12); // exact corners

        canvas.draw(sc::polygon_i{{0, 0}, {5, 0}, {5, 5}});
        CHECK_EQ(canvas.calls.back(), std::string{"polygon 3"});

        canvas.draw(sc::circle{{5, 5}, 3});
        canvas.draw(sc::circle_i{{5, 5}, 4});
        CHECK_EQ(canvas.calls.back(), std::string{"circle 4"});

        canvas.draw(sc::text{"Hello", {10, 20}, {"serif", 14}});
        CHECK_EQ(canvas.calls.back(), std::string{"text Hello serif"});
        canvas.draw_line({0, 0}, {10, 10}, sc::color::Black, 3);
        CHECK_EQ(canvas.calls.back(), std::string{"line 3"});

        canvas.draw(std::vector<sc::circle>{{{1, 1}, 1}, {{2, 2}, 2}}, sc::color::Green);
        CHECK_EQ(canvas.calls.back(), std::string{"circle 2"});
    }

    SECTION("Rounded corners");
    {
        recording_canvas canvas;
        canvas.draw(sc::rect{0, 0, 100, 50}, sc::color::Black, sc::color::Transparent, 1, 10);
        CHECK_EQ(canvas.calls.back(), std::string{"polygon 36"}); // four arcs of 8 segments
        const auto bounds = static_cast<sc::rect>(canvas.last_polygon);
        CHECK_NEAR(bounds.left(), 0.0, 1e-9);
        CHECK_NEAR(bounds.bottom(), 50.0, 1e-9);
        CHECK_NEAR(canvas.last_polygon[0].x(), 0.0, 1e-9); // starts on the left edge, 10 down
        CHECK_NEAR(canvas.last_polygon[0].y(), 10.0, 1e-9);
        canvas.draw(sc::rect{0, 0, 100, 50}, sc::color::Black, sc::color::Transparent, 1, 500); // clamped to 25
        const auto clamped = static_cast<sc::rect>(canvas.last_polygon);
        CHECK_NEAR(clamped.height(), 50.0, 1e-9);
    }

    SECTION("What draws nothing");
    {
        recording_canvas canvas;
        canvas.draw(sc::rect{0, 0, 10, 10}, sc::color::Transparent);                 // no stroke, no fill
        canvas.draw(sc::rect{0, 0, 10, 10}, sc::color::Black, sc::color::Transparent, 0); // no width
        canvas.draw(sc::polygon{{1, 1}});
        canvas.draw(sc::circle{{1, 1}, 0});
        canvas.draw(sc::text{"", {0, 0}});
        canvas.draw(sc::text{"x", {0, 0}}, sc::color::Transparent);
        canvas.draw_line({0, 0}, {1, 1}, sc::color::Black, 0);
        CHECK(canvas.calls.empty());

        canvas.draw(sc::rect{0, 0, 10, 10}, sc::color::Black, sc::color::Red, 0); // fill only
        CHECK(canvas.last_stroke == sc::color::Transparent);
        CHECK_EQ(canvas.last_width, 0.0);
        CHECK(canvas.last_fill == sc::color::Red);
    }

    SECTION("Fonts");
    {
        const sc::font plain;
        CHECK_EQ(plain.family, std::string{"sans-serif"});
        CHECK_EQ(plain.size, 12.0);
        CHECK_EQ(plain.weight, 400);
        CHECK_EQ(sc::font::bold("serif", 20).weight, 700);
        const sc::font small{"serif"}, large{"serif", 14};
        CHECK(small < large);
    }
    TEST_SUMMARY();
}
