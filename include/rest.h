#ifndef REST_H
#define REST_H

#include <string>
#include <map>
#include <memory>

namespace sc {
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

        /// The response body, whatever the status (see status()). When no response came, the
        /// error goes to std::cerr and the body is empty.
        std::string get();

        /// Posts jsonData (any bytes, zeros included; header("Content-Type", ...) for other than JSON)
        /// and returns the response body, whatever the status. When no response
        /// came, the error goes to std::cerr and the body is {"error": "<curl's message>"}.
        std::string post(const std::string &jsonData = R"({})");

        /// Sets a request header, replacing one of the same name.
        void header(const std::string &key, const std::string &value);

        /// "Authorization: Bearer <token>".
        void bearer(const std::string &token);

        /// The HTTP status of the last get() or post() (200, 404, ...), 0 when it got no response.
        [[nodiscard]] long status() const { return status_; }
        /// The last response's headers by lowercased name ("content-type", "location"); empty when
        /// none came.
        [[nodiscard]] const std::map<std::string, std::string> &response_headers() const { return response_headers_; }

        /// The whole request's limit in seconds (default 30); connect_timeout() only connecting's
        /// (default 10).
        rest &timeout(long timeout);

        void connect_timeout(long timeout);
        /// A TLS client certificate and its key (PEM files), for a server that asks for one (LXD).
        rest &client_certificate(const std::string &certificate_file, const std::string &key_file);
        /// Trusts the server's certificate only when it's signed by (or is) one in this PEM file,
        /// rather than the system's authorities: a self-signed server's own certificate pins it.
        rest &trust(const std::string &ca_file);
        /// Whether the certificate must name the host connected to (default true). A server reached
        /// by an address its certificate doesn't list (a cluster member by IP) needs false; the
        /// certificate is still checked against trust().
        rest &verify_host(bool on);
        /// Accepts only a server whose public key is this, curl's "sha256//<base64>" (several
        /// joined by ';'), whatever else is trusted.
        rest &pinned_public_key(const std::string &pins);
        /// Keeps a failed request's error off std::cerr; error() still has it.
        rest &quiet(bool on = true);
        /// Why the last request got no response ("Couldn't connect to server"); empty when one came.
        [[nodiscard]] const std::string &error() const { return error_; }

        /// The content at url, kept for the rest of the process: a local file of that path (or of it
        /// without its first character, or without base_url) if there is one, else a GET. Empty for a
        /// url that is empty or over 100 characters. Not thread-safe: the cache is shared and unguarded.
        static std::string fetch(const std::string &url, const std::string &base_url = "");

    private:
        std::string url_;
        long connect_timeout_secs_ = 10;
        long timeout_secs_ = 30;
        long status_ = 0;
        std::string error_;
        std::string certificate_file_, key_file_, ca_file_, pins_;
        bool verify_host_ = true;
        bool quiet_ = false;
        std::map<std::string, std::string> headers_;
        std::string response;
        static std::map<std::string, std::string> fetch_cache;
        std::map<std::string, std::string> response_headers_;

        // A GET, or a POST of *post_body when it is set.
        std::string perform(const std::string *post_body);
    };
}

#endif //REST_H
