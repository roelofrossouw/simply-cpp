// Geometry in simply-cpp core: points and sizes, rectangles, rotated rectangles and polygons, and
// clustering with DBSCAN. Each line shows a call, as written, and what it returned. Needs no
// server.

#include <cmath>
#include <vector>

#include <sc.h>

int main() {
    sc::console::title("simply-cpp core: geometry");

    sc::console::heading("Points and sizes");
    sc::console::note("Arithmetic works on both coordinates at once, with another pair or a number.");
    SC_SHOW(sc::point{1.5, 2} + sc::point{1, 1});
    SC_SHOW(sc::point{1.5, 2} * 2);
    SC_SHOW(sc::point_i{7, 3} / 2); // whole-number coordinates
    SC_SHOW(sc::size{4, 3}.area());
    SC_SHOW(sc::size{4, 3} - 5); // a size never goes below zero

    sc::console::heading("Rectangles");
    sc::console::note("y grows downwards, as on a screen or in an image: bottom() is top() + height().");
    const sc::rect box{10, 20, 100, 50}; // left, top, width, height
    const auto other = sc::rect::ltrb(60, 40, 160, 120);
    SC_SHOW(box);
    SC_SHOW(box.right_bottom());
    SC_SHOW(box.center());
    SC_SHOW(other);

    sc::console::subheading("Comparing two rectangles");
    SC_SHOW(box.intersect(other));
    SC_SHOW(box.iou(other)); // intersection over union: 0 apart, 1 the same
    SC_SHOW(box.distance(sc::rect{200, 20, 10, 10}));

    sc::console::subheading("Growing one to include another");
    auto bounds = box;
    SC_STEP(bounds.include(other));
    SC_SHOW(bounds);

    sc::console::subheading("Grouping nearby boxes, such as the words on a page");
    const std::vector<sc::rect> words{{0, 0, 40, 10}, {45, 0, 30, 10}, {0, 14, 60, 10}, {200, 0, 40, 10}};
    for (const auto &[group, members]: sc::rect::group_adjacent(words, 8, 6))
        sc::console::note("boxes " + sc::console::format(members) + " fit in " + sc::console::format(group));

    sc::console::heading("Rotated rectangles and polygons");
    const sc::rotated_rect tilted{{50, 50}, {40, 20}, 30}; // centre, size, angle in degrees
    SC_SHOW(tilted);
    SC_SHOW(static_cast<sc::rect>(tilted)); // its upright bounding box
    const sc::polygon corners = tilted;    // its corners, rounded to whole pixels
    SC_SHOW(corners);
    SC_SHOW(corners.is_rect());
    SC_SHOW(corners.expanded(0.5)); // half as big again, both ways
    SC_SHOW(static_cast<sc::rotated_rect>(corners)); // and back from the corners

    sc::console::heading("Clustering with DBSCAN");
    sc::console::note("Each value gets a cluster label, counting from 0; -1 means noise, close to no others.");
    SC_SHOW(sc::dbscan(std::vector{1, 2, 3, 10, 11, 12, 30}, 2, 2));
    SC_SHOW(sc::dbscan(std::vector{350.0, 355.0, 5.0, 10.0, 180.0}, 15, 2, 360)); // compass bearings wrap at 360

    sc::console::subheading("With a distance of your own");
    const auto straight_line = [](const sc::point &a, const sc::point &b) { return std::hypot(a.x() - b.x(), a.y() - b.y()); };
    const std::vector<sc::point> spots{{0, 0}, {1, 1}, {0, 1}, {10, 10}, {11, 10}, {50, 50}};
    SC_SHOW(sc::dbscan_by(spots, 2, 2, straight_line));
    return 0;
}
