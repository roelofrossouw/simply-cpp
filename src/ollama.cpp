#include "ollama.h"
#include "sc.h"
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

namespace sc {
    ollama::ollama(const string &model_name, const string &host, const int port)
        : url(host + ":" + to_string(port) + "/api/"),
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

    string ollama::generate(const std::string &prompt, const vector<string> &images) {
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

        for (const auto &image: images) {
            auto image_data = image.length() >= 1000 ? image : base64::encode(file_get_contents(image));
            if (image_data.empty()) return "Could not read the image";
            data["images"].push_back(image_data);
        }


        data["prompt"] = prompt;

        /* OpenAI/chat format
        json messages{
            {
                {"type", "text"},
                {"text", prompt}
            }
        };
        for (const auto &image: images) {
            auto image_data = image.length() >= 1000 ? image : base64::encode(file_get_contents(image));
            if (image_data.empty()) return "Could not read the image";
            messages.push_back({
                    {"type", "image_url"},
                    {"image_url", {{"url", "data:image/jpg;base64," + image_data}}}
                }
            );
        }
        data["messages"].push_back({
            {"role", "user"},
            {"content", messages}
        });
                */

        last_result = rest(url + "generate").post(data.dump());
        try {
            auto result = json::parse(last_result);
            if (!result.contains("response")) return result.dump(4);
            try {
                if (result.contains("context")) {
                    context = result["context"].get<std::vector<int> >();
                }
            } catch (exception &e) {
                cerr << "Could not get context " << e.what();
            }
            return result["response"];
        } catch (json::exception &e) {
            cerr << "Could not parse json " << e.what();
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

    std::string ollama::process(const std::string &json_request) {
        nlohmann::json output;
        try {
            auto request = nlohmann::json::parse(json_request);
            ollama ai(request["model"].get<string>()
                      , request["server"].get<string>()
                      , request["port"].get<int>());
            if (request.contains("temperature")) ai.setTemperature(request["temperature"].get<float>());
            if (request.contains("think")) ai.setThink(request["think"].get<bool>());
            if (request.contains("max_tokens")) ai.setMaxTokens(request["max_tokens"].get<int>());
            ai.setFormat(request["schema"].dump());
            timer stopwatch;
            vector<string> images;
            if (request.contains("images")) images = request["images"].get<vector<std::string> >();
            auto instructions = request["instruction"].get<string>();
            if (instructions.length() < 1000) {
                auto file_contents = file_get_contents(instructions);
                if (!file_contents.empty()) instructions = file_contents;
            }
            if (request.contains("data")) instructions += request["data"].get<std::string>();
            auto result = ai.generate(instructions, images);
            try {
                output = nlohmann::json::parse(result);
            } catch (nlohmann::detail::exception &e) {
                output["error"] = e.what();
                output["raw"] = result;
            }
            output["processing_time"] = (string) stopwatch;
        } catch (exception &e) {
            output["error"] = e.what();
        }
        return output.dump(2);
    }
}
