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
        static constexpr std::string DEFAULT_IP = "127.0.0.1";
        static constexpr int DEFAULT_PORT = 11434;

    public:
        ollama(const std::string &model = {}, const std::string &ip_address = DEFAULT_IP, int port = DEFAULT_PORT);

        std::vector<std::string> models();

        std::string generate(const std::string &prompt, std::vector<std::string> images = {});

        // Getters
        std::string getUrl() const { return url; }
        std::string getModel() const { return model; }
        std::string getFormat() const { return format; }
        std::string getKeepAlive() const { return keep_alive; }
        int getMaxTokens() const { return max_tokens; }
        bool isStream() const { return stream; }
        float getTemperature() const { return temperature; }
        bool isThink() const { return think; }

        // Setters
        void setUrl(const std::string &url) { this->url = url; }
        void setModel(const std::string &model) { this->model = model; }
        void setFormat(std::string format) { this->format = format; }
        void setKeepAlive(std::string keep_alive) { this->keep_alive = keep_alive; }
        void setMaxTokens(int max_tokens) { this->max_tokens = max_tokens; }
        void setStream(bool stream) { this->stream = stream; }
        void setTemperature(float temperature) { this->temperature = temperature; }
        void setThink(bool think) { this->think = think; }

    private:
        std::string url;
        std::string model;
        std::string format{"json"};
        std::string keep_alive{"30m"};
        int max_tokens{20480};
        bool stream{false};
        float temperature{0.5};
        bool think{false};

        std::string last_result{};
    };
}

#endif //SC_OLLAMA_H
