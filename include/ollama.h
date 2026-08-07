#ifndef SC_OLLAMA_H
#define SC_OLLAMA_H
#include <map>
#include <string>

enum {
    SC_OLLAMA_FORMAT_JSON,
    SC_OLLAMA_FORMAT_STRING
};

namespace sc {
    class ollama {
        static constexpr std::string DEFAULT_IP = "http://127.0.0.1";
        static constexpr int DEFAULT_PORT = 11434;
        static constexpr int DEFAULT_MAX_TOKENS = 512;

    public:
        ollama(const std::string &model = {}, const std::string &host = DEFAULT_IP, int port = DEFAULT_PORT);

        std::vector<std::string> models();

        std::string generate(const std::string &prompt, const std::vector<std::string> &images = {});

        void display_stats();

        int context_size() { return context.size(); }

        // Getters
        std::string getLastResult() const { return last_result; }
        std::string getUrl() const { return url; }
        std::string getModel() const { return model; }
        std::string getFormat() const { return format; }
        std::string getKeepAlive() const { return keep_alive; }
        int getMaxTokens() const { return max_tokens; }
        bool isStream() const { return stream; }
        float getTemperature() const { return temperature; }
        bool isThink() const { return think; }
        std::vector<int> getContext() { return context; }

        // Setters
        void setUrl(const std::string &url) { this->url = url; }
        void setModel(const std::string &model) { this->model = model; }
        void setFormat(std::string format) { this->format = format; }
        void clearFormat() { this->format = ""; }
        void setKeepAlive(std::string keep_alive) { this->keep_alive = keep_alive; }
        void setMaxTokens(int max_tokens) { this->max_tokens = max_tokens; }
        void setTopP(double top_p) { this->top_p = top_p; }
        void setTopK(double top_k) { this->top_k = top_k; }
        void setRepeatPenalty(double repeat_penalty) { this->repeat_penalty = repeat_penalty; }
        void setNumPredict(double num_predict) { this->num_predict = num_predict; }
        void setNumCtx(double num_ctx) { this->num_ctx = num_ctx; }
        void setSeed(double seed) { this->seed = seed; }
        void setStream(bool stream) { this->stream = stream; }
        void setTemperature(float temperature) { this->temperature = temperature; }
        void setThink(bool think) { this->think = think; }
        void setContext(std::vector<int> context) { this->context = context; }

        static std::string process(const std::string &json_request);

    private:
        std::string url;
        std::string model;
        std::string format{"json"};
        std::string keep_alive{"30m"};
        int max_tokens{DEFAULT_MAX_TOKENS};
        float top_p{1.0};
        float top_k{1.0};
        float repeat_penalty{1.0};
        int num_predict{4096};
        int num_ctx{8192};
        int seed{180001};
        bool stream{false};
        float temperature{0.5};
        bool think{false};

        std::string last_result{};
        std::vector<int> context{};
    };
}

#endif //SC_OLLAMA_H
