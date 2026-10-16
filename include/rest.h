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

    /// HTTP GET and POST to one URL, over libcurl, with headers, a bearer token and timeouts.
    /// Requests send "Content-Type: application/json" unless header() says otherwise.
    ///
    ///     sc::rest api{"https://example.com/api/items"};
    ///     api.bearer(token);
    ///     const auto body = api.timeout(5).post(R"({"name": "x"})");
    ///     if (api.status() != 200) ...
    class rest {
    public:
        explicit rest(const std::string &url);

        bool setup_curl(base64_impl::curl &conn) const;

        /// The response body, whatever the status (see status()). When no response came, the
        /// error goes to std::cerr and the body is empty.
        std::string get();

        /// Posts jsonData and returns the response body, whatever the status. When no response
        /// came, the error goes to std::cerr and the body is {"error": "<curl's message>"}.
        std::string post(const std::string &jsonData = R"({})");

        /// Sets a request header, replacing one of the same name.
        void header(const std::string &key, const std::string &value);

        /// "Authorization: Bearer <token>".
        void bearer(const std::string &token);

        /// The HTTP status of the last get() or post() (200, 404, ...), 0 when it got no response.
        [[nodiscard]] long status() const { return status_; }

        /// The whole request's limit in seconds (default 30); connect_timeout() only connecting's
        /// (default 10).
        rest &timeout(long timeout);

        void connect_timeout(long timeout);

        /// The content at url, kept for the rest of the process: a local file of that path (or of it
        /// without its first character, or without base_url) if there is one, else a GET. Empty for a
        /// url over 100 characters. Not thread-safe: the cache is shared and unguarded.
        static std::string fetch(const std::string &url, const std::string &base_url = "");

    private:
        std::string url_;
        long connect_timeout_secs_ = 10;
        long timeout_secs_ = 30;
        long status_ = 0;
        std::map<std::string, std::string> parameters;
        std::map<std::string, std::string> headers_;
        mutable std::string response{};
        static std::map<std::string, std::string> fetch_cache;

        bool setup_curl(base64_impl::curl &conn);
    };
}

#endif //REST_H
