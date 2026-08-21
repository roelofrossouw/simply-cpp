#include "ollama.h"
#include "sc.h"
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::ordered_json;

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

    void ollama::remove_backtick(std::string &input, const std::string &marker, bool mid_string = false) {
        if (input.starts_with("```" + marker) && input.ends_with("```")) {
            input = input.substr(marker.length() + 3, input.size() - (marker.length() + 6));
        }
        if (mid_string) {
            size_t start_pos = input.find("```" + marker);
            size_t end_pos = input.rfind("```");
            if (start_pos != std::string::npos && end_pos != std::string::npos) {
                input = input.substr(start_pos + marker.length() + 3);
                while (input.back() == '`') input.pop_back();
            }
        }
    }

    string ollama::generate(const std::string &prompt, const vector<string> &images) {
        json data{
            {"model", model},
            {"stream", stream},
            {"think", think},
        };
        data["options"] =
        {
            {"max_tokens", max_tokens},
            {"keep_alive", keep_alive},
            {"temperature", temperature},
            {"top_p", top_p},
            {"top_k", top_k},
            {"repeat_penalty", repeat_penalty},
            {"num_predict", num_predict},
            {"num_ctx", num_ctx},
            {"seed", seed},
        };

        if (!empty(context)) data["context"] = context;

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
        last_result = rest(url + "generate").timeout(timeout).post(data.dump());
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

    string ollama::stats() {
        auto j = json::parse(last_result);
        j.erase("context");
        j.erase("response");
        return j.dump();
    }

    void ollama::setFormat(std::string format_string) {
        while (format_string.starts_with('\"')) format_string.erase(format_string.begin());
        while (format_string.ends_with('\"')) format_string.pop_back();
        this->format = format_string;
    }

    std::string ollama::process(const std::string &json_request) {
        json output, request;
        timer stopwatch;

        try {
            request = json::parse(json_request);
        } catch (exception &e) {
            output["error"] = "Invalid request";
            output["message"] = e.what();
            output["ai_data"]["requested_model"] = "Unknown";
            output["ai_data"]["model"] = "Unknown";
            return output;
        }
        try {
#ifdef NDEBUG
            ollama ai(request["model"].get<string>(), request["server"].get<string>(), request["port"].get<int>());
#else
            if (!request.contains("debug_model")) request["debug_model"] = request["model"].get<std::string>();
            if (!request.contains("debug_server")) request["debug_server"] = request["server"].get<std::string>();
            if (!request.contains("debug_port")) request["debug_port"] = request["port"].get<int>();
            ollama ai(request["debug_model"].get<string>(), request["debug_server"].get<string>(), request["debug_port"].get<int>());
#endif
            if (request.contains("temperature")) ai.setTemperature(request["temperature"].get<float>());
            if (request.contains("think")) ai.setThink(request["think"].get<bool>());
            if (request.contains("max_tokens")) ai.setMaxTokens(request["max_tokens"].get<int>());
            if (request.contains("top_p")) ai.setTopP(request["top_p"].get<float>());
            if (request.contains("top_k")) ai.setTopK(request["top_k"].get<int>());
            if (request.contains("repeat_penalty")) ai.setRepeatPenalty(request["repeat_penalty"].get<float>());
            if (request.contains("num_predict")) ai.setNumPredict(request["num_predict"].get<int>());
            if (request.contains("num_ctx")) ai.setNumCtx(request["num_ctx"].get<int>());
            if (request.contains("seed")) ai.setSeed(request["seed"].get<int>());
            if (request.contains("timeout")) ai.setTimeout(request["timeout"].get<int>());
            ai.setFormat(request["schema"].dump());
            vector<string> images;
            if (request.contains("images")) images = request["images"].get<vector<std::string> >();
            auto instructions = request["instruction"].get<string>();
            if (instructions.length() < 1000) {
                auto file_contents = file_get_contents(instructions);
                if (!file_contents.empty()) instructions = file_contents;
            }
            if (request.contains("data")) instructions += request["data"].get<std::string>();
            auto result = ai.generate(instructions, images);
            remove_backtick(result, "json");
            try {
                output = json::parse(result);
            } catch (nlohmann::detail::exception &e) {
                if (!empty(ai.getFormat())) output["json_error"] = e.what();
                output["response"] = result;
            }
            output["ai_data"] = json::parse(ai.stats());
            try {
                if (output["ai_data"].contains("total_duration"))
                    output["ai_data"]["total_duration"] = (string) timer::from_nanos(output["ai_data"]["total_duration"].get<long long>());
                if (output["ai_data"].contains("load_duration"))
                    output["ai_data"]["load_duration"] = (string) timer::from_nanos(output["ai_data"]["load_duration"].get<long long>());
                if (output["ai_data"].contains("prompt_eval_duration"))
                    output["ai_data"]["prompt_eval_duration"] = (string) timer::from_nanos(output["ai_data"]["prompt_eval_duration"].get<long long>());
                if (output["ai_data"].contains("eval_duration"))
                    output["ai_data"]["eval_duration"] = (string) timer::from_nanos(output["ai_data"]["eval_duration"].get<long long>());
            } catch (exception &e) {
                output["ai_data"]["stats_error"] = e.what();
            }
#ifdef NDEBUG
            output["ai_data"]["server"] = request["server"].get<string>();
            output["ai_data"]["requested_model"] = request["model"].get<string>();
            output["ai_data"]["debug"] = false;
#else
            if (!request.contains("debug_server")) request["debug_server"] = request["server"].get<std::string>();
            output["ai_data"]["server"] = request["debug_server"].get<string>();
            output["ai_data"]["requested_model"] = request["debug_model"].get<string>();
            output["ai_data"]["debug"] = true;
#endif
        } catch (exception &e) {
            try {
#ifdef NDEBUG
                output["ai_data"]["requested_model"] = request["model"].get<string>();
                output["ai_data"]["model"] = "Unknown";
                output["ai_data"]["server"] = request["server"].get<string>();
#else
                if (!request.contains("debug_model")) request["debug_model"] = request["model"].get<std::string>();
                if (!request.contains("debug_server")) request["debug_server"] = request["server"].get<std::string>();
                output["ai_data"]["requested_model"] = request["debug_model"].get<string>();
                output["ai_data"]["model"] = "Unknown";
                output["ai_data"]["server"] = request["debug_server"].get<string>();
                output["ai_data"]["debug"] = true;
#endif
            } catch (exception &e) {
                output["ai_data"]["model"] = "Unknown";
            }
            output["error"] = e.what();
        }
        output["ai_data"]["processing_time"] = (string) stopwatch;
        return output.dump(2);
    }
}
