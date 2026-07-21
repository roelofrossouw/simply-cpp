#include "../include/ollama.h"
#include "sc.h"
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

namespace sc {
    ollama::ollama(const string &model_name, const string &ip_address, const int port)
        : url("http://" + ip_address + ":" + to_string(port) + "/api/"),
          model(model_name) {
        if (model.empty()) model = *(models().begin());
        last_result = "Nothing";
    }

    vector<string> ollama::models() {
        vector<string> list;
        auto models = json::parse(rest(url + "tags").get())["models"];
        for (auto model_info: models) list.emplace_back(model_info["name"]);
        return list;
    }

    string ollama::generate(const std::string &prompt, const vector<string> images) {
        json data{
            {"max_tokens", max_tokens},
            {"model", model},
            {"keep_alive", keep_alive},
            {"stream", stream},
            {"temperature", temperature},
            {"think", think}
        };

        if (!empty(context)) {
            data["context"] = context;
        }

        if (!format.empty()) {
            if (format == "json") data["format"] = format;
            else data["format"] = json::parse(format);
        }

        for (const auto image: images) {
            auto image_data = image.length() >= 1000 ? image : base64::encode(file_get_contents(image));
            if (image_data.empty()) return "Could not read the image";
            data["images"].push_back(image_data);
        }

        data["prompt"] = prompt;
        last_result = rest(url + "generate").post(data.dump());

        try {
            auto result = json::parse(last_result);
            if (!result.contains("response")) return result.dump(4);
            try {
                if (result.contains("context")) {
                    context = result["context"].get<std::vector<int>>();
                }
            } catch (exception &e) {
                // cerr << "Could not get context " << e.what();
            }
            return result["response"];
        } catch (json::exception &e) {
            return last_result;
        }
    }

    void ollama::display_stats() {
        auto j = json::parse(last_result);

        // Suppose you parsed the JSON into a nlohmann::json object called j
        int promptTokens = j["prompt_eval_count"];
        int completionTokens = j["eval_count"];
        int totalTokens = promptTokens + completionTokens;

        // Context length used
        int contextUsed = j["context"].size();

        // Remaining budget
        int remaining = 262144 - contextUsed;

        std::cout << "Prompt tokens: " << promptTokens << "\n";
        std::cout << "Completion tokens: " << completionTokens << "\n";
        std::cout << "Context used: " << contextUsed << "\n";
        std::cout << "Remaining budget: " << remaining << "\n";
    }
}
