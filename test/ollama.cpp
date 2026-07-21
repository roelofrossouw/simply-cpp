#include <iostream>
#include "include/ollama.h"

#include "include/timer.h"
using namespace std;

int main() {
    sc::ollama ai("qwen3-vl:2b-instruct-bf16", "ai.v.1web.co.za");
    ai.setFormat("");
    // cout << ai.generate("Hello");
    // cout << ai.generate("Please describe in 30 words or less", {"./resource/image.jpg"});
    sc::timer stopwatch;
    // cout << ai.generate("Generate minimal json for ingestion by llms", {"./resource/test1.png"});

    cout << ai.generate(
                "I'm going to send you pages of a document to analyze one by one to build up a context for me to ask questions later. You do not have to answer anything, just add the contents to your context until I start asking specific questions.")
            << endl;
    ai.display_stats();
    cout << ai.generate("This is page 1", {"./resource/page1.png"}) << endl;
    ai.display_stats();
    cout << ai.generate("This is page 2.", {"./resource/page2.png"}) << endl;
    ai.display_stats();
    cout << ai.generate("This is page 3.", {"./resource/page3.png"}) << endl;
    ai.display_stats();
    cout << ai.generate("How many pages is in the document?") << endl;
    cout << ai.generate("What contained on page 2?") << endl;


    cout << endl << stopwatch << endl;


    return 0;
}
