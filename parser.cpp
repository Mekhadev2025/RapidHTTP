 #include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

// Structure to hold the parsed HTTP message data
struct HttpParser {
    string method;        // For requests
    string url;           // For requests
    string status_code;   // For responses
    string status_message; // For responses
    map<string, string> headers;
    string body;
    
    // For multipart/form-data support
    struct MultipartPart {
        string name;
        string filename;
        string content_type;
        string content;
    };
    vector<MultipartPart> multipart_parts;

    bool hasError;        // Flag for error state
    string errorMessage;  // Detailed error message

    // To handle HTTP/2 and HTTP/3 (placeholder)
    bool isHttp2;         // Flag to indicate if the message is HTTP/2
    bool isHttp3;         // Flag to indicate if the message is HTTP/3
};

// Enum for managing parser state
enum ParseState { REQUEST_LINE, RESPONSE_LINE, HEADERS, BODY, MULTIPART };

// Function to report errors
void reportError(HttpParser& parser, const string& message) {
    parser.hasError = true;
    parser.errorMessage = message;
    cerr << "Error: " << message << "\n";
}

// Function to parse the HTTP request line
void parseRequestLine(const string& request_line, HttpParser& parser) {
    istringstream iss(request_line);
    iss >> parser.method >> parser.url;

    // Validate HTTP method and URL
    if (parser.method.empty() || parser.url.empty()) {
        reportError(parser, "Malformed request line: " + request_line);
    } else {
        cout << "Method: " << parser.method << "\n";
        cout << "URL: " << parser.url << "\n";
    }
}

// Function to parse the HTTP response line
void parseResponseLine(const string& response_line, HttpParser& parser) {
    istringstream iss(response_line);
    iss >> parser.status_code;
    getline(iss, parser.status_message);

    // Validate status code and message
    if (parser.status_code.empty() || parser.status_message.empty()) {
        reportError(parser, "Malformed response line: " + response_line);
    } else {
        cout << "Status Code: " << parser.status_code << "\n";
        cout << "Status Message: " << parser.status_message << "\n";
    }
}

// Function to parse HTTP headers
void parseHeaders(const vector<string>& header_lines, HttpParser& parser) {
    for (const auto& header : header_lines) {
        size_t pos = header.find(":");
        if (pos == string::npos) {
            reportError(parser, "Malformed header: " + header);
            continue;  // Skip this header but continue parsing
        }

        string key = header.substr(0, pos);
        string value = header.substr(pos + 1);
        if (key.empty() || value.empty()) {
            reportError(parser, "Empty key or value in header: " + header);
            continue;  // Skip this header but continue parsing
        }

        parser.headers[key] = value;
        cout << "Header: " << key << " = " << value << "\n";
    }
}

// Function to parse multipart/form-data
void parseMultipartBody(const string& body_content, HttpParser& parser) {
    // Find the boundary from the headers
    auto it = parser.headers.find("Content-Type");
    if (it != parser.headers.end()) {
        string content_type = it->second;
        size_t boundary_pos = content_type.find("boundary=");
        if (boundary_pos != string::npos) {
            string boundary = "--" + content_type.substr(boundary_pos + 9);  // Get the boundary value
            size_t start = 0;
            size_t end = 0;

            // Parse the multipart content
            while ((start = body_content.find(boundary, end)) != string::npos) {
                end = body_content.find(boundary, start + boundary.length());
                if (end == string::npos) break;

                // Extract the part content
                string part = body_content.substr(start + boundary.length(), end - start - boundary.length());
                size_t header_end = part.find("\r\n\r\n");
                if (header_end == string::npos) continue; // Invalid part format

                // Extract headers and content
                string headers_content = part.substr(0, header_end);
                string content = part.substr(header_end + 4); // Skip "\r\n\r\n"

                // Parse headers for the multipart part
                HttpParser::MultipartPart multipart_part;
                istringstream headers_stream(headers_content);
                string header_line;
                while (getline(headers_stream, header_line)) {
                    if (header_line.find("Content-Disposition:") != string::npos) {
                        size_t name_pos = header_line.find("name=\"");
                        if (name_pos != string::npos) {
                            size_t name_end = header_line.find("\"", name_pos + 6);
                            multipart_part.name = header_line.substr(name_pos + 6, name_end - (name_pos + 6));
                        }

                        size_t filename_pos = header_line.find("filename=\"");
                        if (filename_pos != string::npos) {
                            size_t filename_end = header_line.find("\"", filename_pos + 10);
                            multipart_part.filename = header_line.substr(filename_pos + 10, filename_end - (filename_pos + 10));
                        }
                    }
                    if (header_line.find("Content-Type:") != string::npos) {
                        multipart_part.content_type = header_line.substr(header_line.find(":") + 2);
                    }
                }

                multipart_part.content = content;
                parser.multipart_parts.push_back(multipart_part);

                cout << "Parsed multipart part: name=" << multipart_part.name 
                     << ", filename=" << multipart_part.filename 
                     << ", content_type=" << multipart_part.content_type 
                     << ", content=" << multipart_part.content.substr(0, 20) << "...\n"; // Print first 20 chars of content
            }
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
            if (parser.hasError) return;  // Stop if error occurred
            state = HEADERS;
        } else if (state == RESPONSE_LINE) {
            parseResponseLine(line, parser);
            if (parser.hasError) return;  // Stop if error occurred
            state = HEADERS;
        } else if (state == HEADERS) {
            if (isEndOfHeaders(line)) {
                state = BODY;
                parseHeaders(headers, parser);
                if (parser.hasError) return;  // Stop if error occurred
            } else {
                headers.push_back(line);
            }
        } else if (state == BODY) {
            // Check for multipart/form-data
            auto content_type_it = parser.headers.find("Content-Type");
            if (content_type_it != parser.headers.end() && 
                content_type_it->second.find("multipart/form-data") != string::npos) {
                parseMultipartBody(parser.body, parser);
            } else {
                appendBody(line, parser); // Stream body content
            }
        }
    }

    if (!headers.empty() && state == BODY) {
        parseHeaders(headers, parser);
    }
}

int main() {
    // Example HTTP request with multipart/form-data
    vector<string> http_request = {
        "POST /upload HTTP/1.1",
        "Host: example.com",
        "Content-Type: multipart/form-data; boundary=boundary123",
        "",
        "--boundary123\r\n"
        "Content-Disposition: form-data; name=\"field1\"\r\n\r\n"
        "value1\r\n"
        "--boundary123\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"file.txt\"\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "file content here\r\n"
        "--boundary123--"
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

    if (!parser.hasError) {
        // Output the parsed results for the request
        cout << "\n--- Parsed HTTP Request ---\n";
        cout << "Method: " << parser.method << "\n";
        cout << "URL: " << parser.url << "\n";
        cout << "Headers:\n";
        for (const auto& header : parser.headers) {
            cout << header.first << ": " << header.second << "\n";
        }
        cout << "Body: " << parser.body << "\n";

        // Output multipart parts
        cout << "Multipart Parts:\n";
        for (const auto& part : parser.multipart_parts) {
            cout << "Part Name: " << part.name << ", Filename: " << part.filename 
                 << ", Content Type: " << part.content_type << "\n";
            cout << "Content: " << part.content.substr(0, 20) << "...\n"; // Print first 20 chars of content
        }
    }

    // Clear the parser for the next message
    parser = HttpParser();

    // Simulate parsing the HTTP response
    cout << "\nParsing HTTP Response:\n";
    handleParsing(http_response, parser, false);

    if (!parser.hasError) {
        // Output the parsed results for the response
        cout << "\n--- Parsed HTTP Response ---\n";
        cout << "Status Code: " << parser.status_code << "\n";
        cout << "Status Message: " << parser.status_message << "\n";
        cout << "Headers:\n";
        for (const auto& header : parser.headers) {
            cout << header.first << ": " << header.second << "\n";
        }
        cout << "Body: " << parser.body << "\n";
    }

    return 0;
}
