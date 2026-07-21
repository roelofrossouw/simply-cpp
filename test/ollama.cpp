#include <iostream>
#include "sc.h"

using namespace std;

int main() {
    sc::timer stopwatch;
    sc::ollama ai("qwen3-vl:2b-instruct-bf16", "127.0.0.1");
    ai.clearFormat();
    cout << ai.generate("Please describe in 10 words or less", {"./resource/image.jpg"});
    cout << endl << stopwatch << endl;
    // Test context retention
    cout << ai.generate("Describe in 50 words or less");
    cout << endl << stopwatch << endl;
    return 0;
}
