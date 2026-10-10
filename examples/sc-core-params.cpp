// Command-line arguments with sc::params: a structure lists the options and arguments, and
// sc::params reads a command line by it, GNU style, with -h/--help built in. This demo reads
// fixed command lines so it can show several; a program passes main()'s argc and argv instead,
// and then --help prints and exits by itself. Each line shows a call, as written, and what it
// returned. Needs no server.

#include <params.h>
#include <sc.h>

#include <string>
#include <vector>

namespace {
    const nlohmann::ordered_json structure{
        {"name", "copy"},
        {"description", "Copies INPUT to OUTPUT."},
        {"version", "1.0.0"},
        {"options", {
            {"verbose", {{"short", "v"}, {"type", "boolean"}, {"help", "say what is being done"}}},
            {"count", {{"short", "n"}, {"type", "integer"}, {"default", 10}, {"help", "copy at most this many"}}},
            {"mode", {{"choices", {"fast", "safe"}}, {"default", "safe"}, {"help", "how to copy"}}},
            {"define", {{"short", "D"}, {"multiple", true}, {"value", "NAME=VALUE"}, {"help", "set a variable"}}}}},
        {"positional", {
            {"input", {{"help", "the file to read"}}},
            {"output", {{"default", "-"}, {"help", "where to write"}}}}}};

    // What reading arguments gave, or the mistake it reported.
    std::string read(const std::vector<std::string> &arguments) {
        try {
            return sc::params{arguments, structure}.dump();
        } catch (const sc::params::error &mistake) {
            return std::string{"error: "} + mistake.what();
        }
    }
}

int main() {
    sc::console::title("simply-cpp core: command-line arguments");

    sc::console::heading("Reading a command line");
    SC_SHOW(read({"in.txt"}));                           // defaults fill in the rest
    SC_SHOW(read({"-v", "-n", "3", "in.txt", "out.txt"}));
    SC_SHOW(read({"-vn3", "--mode=fast", "in.txt"}));     // flags together, a value attached
    SC_SHOW(read({"-D", "a=1", "--define", "b=2", "in"})); // multiple: a list
    SC_SHOW(read({"--verb", "in", "--", "-odd-name"}));   // a prefix is enough; -- ends the options

    sc::console::heading("Reading the values");
    const sc::params args{{"-n", "5", "in.txt"}, structure};
    SC_SHOW(args["input"].get<std::string>());
    SC_SHOW(args["count"].get<int>());
    SC_SHOW(args.as<bool>("verbose"));
    SC_SHOW(args.as<std::string>("output"));

    sc::console::heading("Mistakes, as the person running it sees them");
    SC_SHOW(read({}));
    SC_SHOW(read({"in", "--colour"}));
    SC_SHOW(read({"in", "-n", "many"}));
    SC_SHOW(read({"in", "--mode=slow"}));
    SC_SHOW(read({"in", "out", "more"}));

    sc::console::heading("What --help prints");
    SC_SHOW(sc::params({"--help"}, structure).help_requested());
    std::cout << '\n' << args.help();
    return 0;
}
