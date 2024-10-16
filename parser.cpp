#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
using namespace std;

// Structure to hold the parsed HTTP message data
struct HttpParser {
    string method;        // For requests
    string url;           // For requests
    string status_code;   // For responses
    string status_message; // For responses
    map<string, string> headers;
    string body;
};

// Enum for managing parser state
enum ParseState { REQUEST_LINE, RESPONSE_LINE, HEADERS, BODY };

// Function to parse the HTTP request line
void parseRequestLine(const string& request_line, HttpParser& parser) {
    istringstream iss(request_line);
    iss >> parser.method >> parser.url;
    cout << "Method: " << parser.method << "\n";
    cout << "URL: " << parser.url << "\n";
}

// Function to parse the HTTP response line
void parseResponseLine(const string& response_line, HttpParser& parser) {
    istringstream iss(response_line);
    iss >> parser.status_code;
    getline(iss, parser.status_message);
    cout << "Status Code: " << parser.status_code << "\n";
    cout << "Status Message: " << parser.status_message << "\n";
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

// Function to append to the body of the HTTP message (for streaming)
void appendBody(const string& body_content, HttpParser& parser) {
    parser.body += body_content;
    cout << "Body (streamed): " << parser.body << "\n";
}

// Helper function to detect the end of headers
bool isEndOfHeaders(const string& line) {
    return line.empty();
}

// Main function to simulate parsing of an HTTP message
void handleParsing(const vector<string>& http_message, HttpParser& parser, bool isRequest) {
    ParseState state = isRequest ? REQUEST_LINE : RESPONSE_LINE;
    vector<string> headers;

    for (const auto& line : http_message) {
        if (state == REQUEST_LINE) {
            parseRequestLine(line, parser);
            state = HEADERS;
        } else if (state == RESPONSE_LINE) {
            parseResponseLine(line, parser);
            state = HEADERS;
        } else if (state == HEADERS) {
            if (isEndOfHeaders(line)) {
                state = BODY;
                parseHeaders(headers, parser);
            } else {
                headers.push_back(line);
            }
        } else if (state == BODY) {
            appendBody(line, parser); // Stream body content
        }
    }

    if (!headers.empty() && state == BODY) {
        parseHeaders(headers, parser);
    }
}

int main() {
    // Example HTTP request
    vector<string> http_request = {
        "GET /index.html HTTP/1.1",
        "Host: example.com",
        "Connection: keep-alive",
        "Accept: text/html",
        "",
        "Body content part 1.",
        "Body content part 2.",
        "Body content part 3."
    };

    // Example HTTP response
    vector<string> http_response = {
        "HTTP/1.1 200 OK",
        "Content-Type: text/html",
        "Content-Length: 1234",
        "Connection: keep-alive",
        "",
        "<html><body>Hello, world!</body></html> Part 1.",
        "<html><body> Goodbye!</body></html> Part 2."
    };

    // Initialize the HTTP parser structure
    HttpParser parser;

    // Simulate parsing the HTTP request
    cout << "Parsing HTTP Request:\n";
    handleParsing(http_request, parser, true);
    
    // Output the parsed results for the request
    cout << "\n--- Parsed HTTP Request ---\n";
    cout << "Method: " << parser.method << "\n";
    cout << "URL: " << parser.url << "\n";
    cout << "Headers:\n";
    for (const auto& header : parser.headers) {
        cout << header.first << ": " << header.second << "\n";
    }
    cout << "Body: " << parser.body << "\n";

    // Clear the parser for the next message
    parser = HttpParser();

    // Simulate parsing the HTTP response
    cout << "\nParsing HTTP Response:\n";
    handleParsing(http_response, parser, false);

    // Output the parsed results for the response
    cout << "\n--- Parsed HTTP Response ---\n";
    cout << "Status Code: " << parser.status_code << "\n";
    cout << "Status Message: " << parser.status_message << "\n";
    cout << "Headers:\n";
    for (const auto& header : parser.headers) {
        cout << header.first << ": " << header.second << "\n";
    }
    cout << "Body: " << parser.body << "\n";

    return 0;
}
