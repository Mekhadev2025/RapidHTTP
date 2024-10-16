#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
using namespace std;
// Structure to hold the parsed HTTP message data
struct HttpParser {
    std::string method;
    std::string url;
    std::map<std::string, std::string> headers;
    std::string body;
};

// Enum for managing parser state
enum ParseState { REQUEST_LINE, HEADERS, BODY };

// Function to parse the HTTP request line
void parseRequestLine(const std::string& request_line, HttpParser& parser) {
    std::istringstream iss(request_line);
    iss >> parser.method >> parser.url;
    std::cout << "Method: " << parser.method << "\n";
    std::cout << "URL: " << parser.url << "\n";
}

// Function to parse HTTP headers
void parseHeaders(const std::vector<std::string>& header_lines, HttpParser& parser) {
    for (const auto& header : header_lines) {
        size_t pos = header.find(":");
        if (pos != std::string::npos) {
            std::string key = header.substr(0, pos);
            std::string value = header.substr(pos + 1);
            parser.headers[key] = value;
            std::cout << "Header: " << key << " = " << value << "\n";
        }
    }
}

// Function to parse the body of the HTTP message (optional)
void parseBody(const std::string& body_content, HttpParser& parser) {
    parser.body = body_content;
    std::cout << "Body: " << parser.body << "\n";
}

// Helper function to detect the end of headers
bool isEndOfHeaders(const std::string& line) {
    return line.empty();
}

// Main function to simulate parsing of an HTTP message
void handleParsing(const std::vector<std::string>& http_message, HttpParser& parser) {
    ParseState state = REQUEST_LINE;
    std::vector<std::string> headers;

    for (const auto& line : http_message) {
        if (state == REQUEST_LINE) {
            parseRequestLine(line, parser);
            state = HEADERS;
        } else if (state == HEADERS) {
            if (isEndOfHeaders(line)) {
                state = BODY;
                parseHeaders(headers, parser);
            } else {
                headers.push_back(line);
            }
        } else if (state == BODY) {
            parseBody(line, parser);
        }
    }

    if (!headers.empty() && state == BODY) {
        parseHeaders(headers, parser);
    }
}

int main() {
    // Example HTTP request (as lines of a string)
    std::vector<std::string> http_message = {
        "GET /index.html HTTP/1.1",
        "Host: example.com",
        "Connection: keep-alive",
        "Accept: text/html",
        "",
        "Body content here (optional, usually for POST requests)"
    };

    // Initialize the HTTP parser structure
    HttpParser parser;

    // Simulate parsing the message
    handleParsing(http_message, parser);

    // Output the parsed results (optional)
    std::cout << "\n--- Parsed HTTP Message ---\n";
    std::cout << "Method: " << parser.method << "\n";
    std::cout << "URL: " << parser.url << "\n";
    std::cout << "Headers:\n";
    for (const auto& header : parser.headers) {
        std::cout << header.first << ": " << header.second << "\n";
    }
    std::cout << "Body: " << parser.body << "\n";

    return 0;
}
