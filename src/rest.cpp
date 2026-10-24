#include <rest.h>
#include <../include/sc.h>
#include <curl/curl.h>

namespace sc {
    // One response header line, "Name: value", into the map by its lowercased name. A status line
    // (a new response, after a redirect or a 100 Continue) starts the map again.
    static size_t header_data(char *contents, size_t size, size_t nmemb, void *userp) {
        auto &headers = *static_cast<std::map<std::string, std::string> *>(userp);
        std::string line(contents, size * nmemb);
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.starts_with("HTTP/")) {
            headers.clear();
        } else if (const auto colon = line.find(':'); colon != std::string::npos) {
            auto value = line.substr(colon + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            headers[lowercase(std::string_view(line).substr(0, colon))] = value;
        }
        return size * nmemb;
    }

    static size_t write_data(void *contents, size_t size, size_t nmemb, void *userp) {
        static_cast<std::string *>(userp)->append(static_cast<char *>(contents), size * nmemb);
        return size * nmemb;
    }

    std::map<std::string, std::string> rest::fetch_cache;

    rest::rest(const std::string &url) : url_(url) {
        headers_["Content-Type"] = "application/json";
    }

    namespace detail {
        class curl {
        public:
            curl() : curl_(curl_easy_init()) {
            }

            curl(const curl &) = delete;
            curl &operator=(const curl &) = delete;

            ~curl() {
                if (headers_) curl_slist_free_all(headers_);
                if (curl_) curl_easy_cleanup(curl_);
            }

            bool run() {
                if (!curl_) {
                    last_error_ = "No connection available";
                    return false;
                }
                const CURLcode result = curl_easy_perform(curl_);
                last_error_ = curl_easy_strerror(result);
                return result == CURLE_OK;
            }

            [[nodiscard]] const std::string &last_error() const { return last_error_; }

            // 0 when there was no response.
            [[nodiscard]] long status() const {
                long code = 0;
                if (curl_) curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &code);
                return code;
            }

            template<typename T>
            void option(const CURLoption option, const T &value) const {
                if (curl_) curl_easy_setopt(curl_, option, value);
            }

            void headers(const std::map<std::string, std::string> &header_map) {
                for (const auto &[name, value]: header_map)
                    headers_ = curl_slist_append(headers_, (name + ": " + value).c_str());
                if (headers_) option(CURLOPT_HTTPHEADER, headers_);
            }

        private:
            CURL *curl_{nullptr};
            curl_slist *headers_{nullptr};
            std::string last_error_{};
        };
    }

    std::string rest::perform(const std::string *post_body) {
        detail::curl conn;
        conn.option(CURLOPT_URL, url_.c_str());
        conn.option(CURLOPT_CONNECTTIMEOUT, connect_timeout_secs_);
        conn.option(CURLOPT_TIMEOUT, timeout_secs_);
        conn.headers(headers_);
        conn.option(CURLOPT_WRITEFUNCTION, write_data);
        response.clear();
        conn.option(CURLOPT_WRITEDATA, &response);
        response_headers_.clear();
        conn.option(CURLOPT_HEADERFUNCTION, header_data);
        conn.option(CURLOPT_HEADERDATA, &response_headers_);
        if (!certificate_file_.empty()) {
            conn.option(CURLOPT_SSLCERT, certificate_file_.c_str());
            conn.option(CURLOPT_SSLKEY, key_file_.c_str());
        }
        if (!ca_file_.empty()) conn.option(CURLOPT_CAINFO, ca_file_.c_str());
        if (!verify_host_) conn.option(CURLOPT_SSL_VERIFYHOST, 0L);
        if (!pins_.empty()) conn.option(CURLOPT_PINNEDPUBLICKEY, pins_.c_str());
        if (post_body) {
            // With the size given, bodies with zero bytes go out whole.
            conn.option(CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(post_body->size()));
            conn.option(CURLOPT_POSTFIELDS, post_body->data());
        }
        const bool ran = conn.run();
        status_ = ran ? conn.status() : 0;
        error_ = ran ? std::string{} : conn.last_error();
        if (!ran) {
            if (!quiet_) std::cerr << "Request failed: " << conn.last_error() << std::endl;
            response = post_body ? R"({"error": ")" + conn.last_error() + R"("})" : "";
        }
        return response;
    }

    std::string rest::get() { return perform(nullptr); }

    std::string rest::post(const std::string &jsonData) { return perform(&jsonData); }

    void rest::header(const std::string &key, const std::string &value) { headers_[key] = value; }

    void rest::bearer(const std::string &token) { header("Authorization", "Bearer " + token); }

    rest &rest::timeout(const long timeout) {
        timeout_secs_ = timeout;
        return *this;
    }

    void rest::connect_timeout(const long timeout) { connect_timeout_secs_ = timeout; }

    rest &rest::client_certificate(const std::string &certificate_file, const std::string &key_file) {
        certificate_file_ = certificate_file;
        key_file_ = key_file;
        return *this;
    }

    rest &rest::trust(const std::string &ca_file) {
        ca_file_ = ca_file;
        return *this;
    }

    rest &rest::verify_host(const bool on) {
        verify_host_ = on;
        return *this;
    }

    rest &rest::pinned_public_key(const std::string &pins) {
        pins_ = pins;
        return *this;
    }

    rest &rest::quiet(const bool on) {
        quiet_ = on;
        return *this;
    }

    std::string rest::fetch(const std::string &url, const std::string &base_url) {
        if (url.empty() || url.size() > 100) return "";
        if (const auto cached = fetch_cache.find(url); cached != fetch_cache.end()) return cached->second;

        std::string content;
        if (std::filesystem::exists(url)) content = file_get_contents(url);
        if (content.empty() && std::filesystem::exists(url.substr(1))) content = file_get_contents(url.substr(1));
        if (content.empty() && url.starts_with(base_url)) {
            const auto local_path = url.substr(base_url.size());
            if (std::filesystem::exists(local_path)) content = file_get_contents(local_path);
        }
        if (content.empty()) content = rest(url).get();

        return fetch_cache[url] = content;
    }
}
