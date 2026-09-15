#include <iostream>
#include "sc.h"

using namespace std;

int main() {
    auto answer = sc::ollama::process(sc::file_get_contents("resource/process.json"));
    cout << answer << endl;

    cout << "\n\n==============================\n\n";

    sc::timer stopwatch;
    sc::ollama ai("qwen3.5:4b-mlx");
    ai.clearFormat();

    cout << ai.generate("Summarize in 20 words.", {"resource/image.jpg"});
    cout << endl << stopwatch << endl;

    cout << "\n\n==============================\n\n";

    // Test context retention.
    stopwatch.reset();
    cout << ai.generate("How many dogs are there?");
    cout << endl << stopwatch << endl;
    return 0;
}
