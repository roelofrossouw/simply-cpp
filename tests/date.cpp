#include <iostream>
#include "sc.h"

using namespace std;

int main() {
    sc::date sample;
    cout << "Now: " << sample << endl;
    cout << "Now (fmt): " << sample.format("%d of %m in --> %Y") << endl;
    sc::date tp{"15/3/1999"};
    cout << "The ides: " << tp << endl;
    sc::date tp2{"asd;nasdjl "};
    cout << "Junk: " << tp2 << endl;
    sc::date tp3{"asd/1/asd"};
    cout << "Junk2: " << tp3 << endl;
    sample += 1;
    cout << "Tomorrow: " << sample << endl;
    sample -= 1;
    cout << "Back: " << sample << endl;
    cout << "Incr Pos: " << sample++ << endl;
    cout << "Show: " << sample << endl;
    cout << "Incr Pre: " << ++sample << endl;
    cout << "Show: " << sample << endl;
    cout << "As Julian: " << sample.as_julian() << endl;
    long jd = sample;
    cout << "As Long: " << jd << endl;
    cout << "From Julian: " << sc::date::from_julian(jd) << endl;
    cout << "Month(2): " << sc::date::month(2) << endl;
    cout << "Year(2): " << sc::date::year(2) << endl;
    cout << "Day(365): " << sc::date::day(365) << endl;
    cout << "Day(365*2): " << sc::date::day(365 * 2) << endl;
    cout << "Day(365*3): " << sc::date::day(365 * 3) << endl;
    cout << "Day(365*4): " << sc::date::day(365 * 4) << endl;
    sample = "1977-05-08";
    cout << "Assign 8/5/77: " << sample << endl;
    cout << "Sub 2Y: " << sample.sub(2, "Y") << endl;
    cout << "Sub 8D: " << sample.sub(8, "D") << endl;
    cout << "Sub 3M: " << sample.sub(6, "M") << endl;
    cout << "Trunc M: " << sample.trunc("M") << endl;
    cout << "Trunc Y: " << sample.trunc("Y") << endl;

    return 0;
}
