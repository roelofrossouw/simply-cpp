#ifndef REST_H
#define REST_H

#include <string>
#include <map>
#include <memory>

struct curl_slist;

namespace sc {
    namespace base64_impl {
        class curl;
    }

    /**
     * @class rest
     * @brief A class to handle HTTP REST operations such as GET and POST requests.
     *
     * This class provides utilities to perform HTTP requests with support for headers,
     * authentication, and timeout configurations. It can also fetch content from a URL
     * with optional caching capabilities.
     */
    class rest {
    public:
        explicit rest(const std::string &url);

        bool setup_curl(base64_impl::curl &conn) const;

        /**
         * @brief Sends an HTTP GET request to the specified URL and retrieves the server response.
         *
         * This method uses libcurl to perform a GET request to the URL that was set when the `rest`
         * object was instantiated. Custom headers can be specified and will be included in the request.
         * Configurable connection and request timeouts are applied during the operation.
         *
         * In case of a request failure, an error message is logged, and an empty string is returned.
         *
         * @return A string containing the response received from the server, or an empty string if the request fails.
         */
        std::string get();

        /**
         * @brief Sends an HTTP POST request with JSON data to a predefined URL.
         *
         * This method uses libcurl to send a POST request to the URL specified during
         * the construction of the `rest` object. The JSON payload is sent with the
         * request body, and a "Content-Type: application/json" header is included by default.
         * Configurable connection and request timeouts are applied.
         *
         * In the event of a request failure, an error message is logged, and an empty
         * string is returned as the response.
         *
         * @param jsonData A JSON string containing the payload to be sent in the POST request.
         * @return A string containing the response retrieved from the server.
         */
        std::string post(const std::string &jsonData = R"({})");

        void header(const std::string &key, const std::string &value);

        void bearer(const std::string &token);

        rest &timeout(long timeout);

        void connect_timeout(long timeout);

        static std::string fetch(const std::string &url, const std::string &base_url = "");

    private:
        std::string url_;
        long connect_timeout_secs_ = 10;
        long timeout_secs_ = 30;
        std::map<std::string, std::string> parameters;
        std::map<std::string, std::string> headers_;
        mutable std::string response{};
        static std::map<std::string, std::string> fetch_cache;

        bool setup_curl(base64_impl::curl &conn);
    };
}

#endif //REST_H
