#include <rest.h>
#include <../include/sc.h>
#include <curl/curl.h>

namespace sc {
    size_t write_data(void *contents, size_t size, size_t nmemb, void *userp) {
        static_cast<std::string *>(userp)->append((char *) contents, size * nmemb);
        return size * nmemb;
    }

    std::map<std::string, std::string> rest::fetch_cache;


    rest::rest(const std::string &url) : url_(url) {
        headers_["Content-Type"] = "application/json";
    }

    namespace base64_impl {
        class curl {
        public:
            curl() : curl_(curl_easy_init()) {
            }

            bool run() {
                if (!curl_) {
                    last_error_ = "No connection available";
                    return false;
                }
                last_result = curl_easy_perform(curl_);
                last_error_ = curl_easy_strerror(last_result);
                return last_result == CURLE_OK;
            }

            std::string last_error() {
                return last_error_;
            }

            void option(const CURLoption option, const std::string &value) const {
                if (!curl_) return;
                curl_easy_setopt(curl_, option, value.c_str());
            }

            template<typename T>
            void option(const CURLoption option, const T &value) const {
                if (!curl_) return;
                curl_easy_setopt(curl_, option, value);
            }

            ~curl() {
                if (headers_) curl_slist_free_all(headers_);
                if (curl_) curl_easy_cleanup(curl_);
            }

            void headers(const std::map<std::string, std::string> &header_map) {
                for (const auto &[fst, snd]: header_map) {
                    headers_ = curl_slist_append(headers_, (fst + ": " + snd).c_str());
                }
                if (headers_) option(CURLOPT_HTTPHEADER, headers_);
            }

        private:
            CURL *curl_{nullptr};
            curl_slist *headers_{nullptr};
            CURLcode last_result{CURLE_FAILED_INIT};
            std::string last_error_{};
        };
    }


    bool rest::setup_curl(base64_impl::curl &conn) {
        conn.option(CURLOPT_URL, url_.c_str());
        conn.option(CURLOPT_CONNECTTIMEOUT, connect_timeout_secs_);
        conn.option(CURLOPT_TIMEOUT, timeout_secs_);
        conn.headers(headers_);
        conn.option(CURLOPT_WRITEFUNCTION, write_data);
        conn.option(CURLOPT_WRITEDATA, &response);
        return true;
    }

    std::string rest::get() {
        base64_impl::curl conn;
        setup_curl(conn);
        if (!conn.run()) {
            std::cerr << "Request failed: " << conn.last_error() << std::endl;
            response = "";
        }
        return response;
    }

    std::string rest::post(const std::string &jsonData) {
        base64_impl::curl conn;
        setup_curl(conn);
        conn.option(CURLOPT_POSTFIELDS, jsonData.c_str());
        if (!conn.run()) {
            std::cerr << "Request failed: " << conn.last_error() << std::endl;
            response = std::string(R"({"error": ")") + conn.last_error() + R"("})";
        }
        return response;
    }

    void rest::header(const std::string &key, const std::string &value) { headers_[key] = value; }

    void rest::bearer(const std::string &token) { header("Authorization", "Bearer " + token); }

    rest &rest::timeout(const long timeout) {
        timeout_secs_ = timeout;
        return *this;
    }

    void rest::connect_timeout(const long timeout) { connect_timeout_secs_ = timeout; }

    std::string rest::fetch(const std::string &url, const std::string &base_url) {
        if (url.size() > 100) return "";
        if (fetch_cache.find(url) != fetch_cache.end()) {
            return fetch_cache[url];
        }

        std::string content;

        if (std::filesystem::exists(url)) {
            content = file_get_contents(url);
        }

        if (content.empty() && std::filesystem::exists(url.substr(1))) {
            content = file_get_contents(url.substr(1));
        }

        if (content.empty() && url.substr(0, base_url.length()) == base_url) {
            auto local_path = url.substr(base_url.size());
            if (std::filesystem::exists(local_path)) {
                content = file_get_contents(local_path);
            }
        }

        if (content.empty()) {
            rest rest(url);
            content = rest.get();
        }

        fetch_cache[url] = content;
        return fetch_cache[url];
    }
}
