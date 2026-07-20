#include <iostream>
#include "include/ollama.h"
using namespace std;

int main() {
    sc::ollama ai("qwen3-vl:2b-instruct-bf16", "ai.v.1web.co.za");
    // ai.setFormat("json");
    ai.setFormat("");
    cout << ai.generate("Please describe in 30 words or less", {"/Users/roelof/simply-cpp-mupdf/cmake-build-debug/resource/image.jpg"});

    return 0;
}
