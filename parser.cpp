 #include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
using namespace std;
// Structure to hold the parsed HTTP message data
struct HttpParser {
    string method;
    string url;
    map<string, string> headers;
    string body;
};

// Enum for managing parser state
enum ParseState { REQUEST_LINE, HEADERS, BODY };

// Function to parse the HTTP request line
void parseRequestLine(const string& request_line, HttpParser& parser) {
    istringstream iss(request_line);
    iss >> parser.method >> parser.url;
    cout << "Method: " << parser.method << "\n";
    cout << "URL: " << parser.url << "\n";
}

// Function to parse HTTP headers
void parseHeaders(const vector<string>& header_lines, HttpParser& parser) {
    for (const auto& header : header_lines) {
        size_t pos = header.find(":");
        if (pos != string::npos) {
            string key = header.substr(0, pos);
            string value = header.substr(pos + 1);
            parser.headers[key] = value;
            cout << "Header: " << key << " = " << value << "\n";
        }
    }
}

// Function to parse the body of the HTTP message (optional)
void parseBody(const string& body_content, HttpParser& parser) {
    parser.body = body_content;
    cout << "Body: " << parser.body << "\n";
}

// Helper function to detect the end of headers
bool isEndOfHeaders(const string& line) {
    return line.empty();
}

// Main function to simulate parsing of an HTTP message
void handleParsing(const vector<string>& http_message, HttpParser& parser) {
    ParseState state = REQUEST_LINE;
    vector<string> headers;

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
    vector<string> http_message = {
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
    cout << "\n--- Parsed HTTP Message ---\n";
    cout << "Method: " << parser.method << "\n";
    cout << "URL: " << parser.url << "\n";
    cout << "Headers:\n";
    for (const auto& header : parser.headers) {
        cout << header.first << ": " << header.second << "\n";
    }
    cout << "Body: " << parser.body << "\n";

    return 0;
}
