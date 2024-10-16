 #include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp> // Include the nlohmann/json library

#include <tinyxml2.h>        // Include the tinyxml2 library


using namespace std;
using json = nlohmann::json; // Create an alias for the JSON library

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

    // Callback for async parsing
    function<void(const HttpParser&)> onComplete;
    function<void(const string&)> onError;

    HttpParser() : hasError(false), isHttp2(false), isHttp3(false) {}
};

// Enum for managing parser state
enum ParseState { REQUEST_LINE, RESPONSE_LINE, HEADERS, BODY, MULTIPART, JSON_BODY, XML_BODY };

// Function to report errors
void reportError(HttpParser& parser, const string& message) {
    parser.hasError = true;
    parser.errorMessage = message;
    cerr << "Error: " << message << "\n";
    if (parser.onError) parser.onError(message);
}

// Function to validate and report errors for headers
bool validateHeader(const string& key, const string& value, HttpParser& parser) {
    if (key.empty() || value.empty()) {
        reportError(parser, "Header key or value cannot be empty");
        return false;
    }

    // Example check for duplicates (simple implementation)
    if (parser.headers.find(key) != parser.headers.end()) {
        reportError(parser, "Duplicate header: " + key);
        return false;
    }

    // Add additional checks for forbidden headers, etc., as needed
    // (Example: checking for multiple Content-Length headers)
    if (key == "Content-Length" && parser.headers.find("Content-Length") != parser.headers.end()) {
        reportError(parser, "Multiple Content-Length headers found");
        return false;
    }

    return true;
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

        // Validate header
        if (!validateHeader(key, value, parser)) {
            continue;  // Skip invalid header
        }

        parser.headers[key] = value;
        cout << "Header: " << key << " = " << value << "\n";
    }
}

// Function to parse multipart/form-data
void parseMultipartBody(const string& body_content, HttpParser& parser) {
    auto it = parser.headers.find("Content-Type");
    if (it != parser.headers.end()) {
        string content_type = it->second;
        size_t boundary_pos = content_type.find("boundary=");
        if (boundary_pos != string::npos) {
            string boundary = "--" + content_type.substr(boundary_pos + 9);
            size_t start = 0;
            size_t end = 0;

            // Parse the multipart content
            while ((start = body_content.find(boundary, end)) != string::npos) {
                end = body_content.find(boundary, start + boundary.length());
                if (end == string::npos) break;

                string part = body_content.substr(start + boundary.length(), end - start - boundary.length());
                size_t header_end = part.find("\r\n\r\n");
                if (header_end == string::npos) continue; // Invalid part format

                string headers_content = part.substr(0, header_end);
                string content = part.substr(header_end + 4); // Skip "\r\n\r\n"

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
                     << ", content=" << multipart_part.content.substr(0, 20) << "...\n";
            }
        }
    }
}

// Function to append to the body of the HTTP message (for streaming)
void appendBody(const string& body_content, HttpParser& parser) {
    parser.body += body_content;
    cout << "Body (streamed): " << parser.body << "\n";
}

// Function to parse JSON body
void parseJsonBody(const string& body_content, HttpParser& parser) {
    try {
        json j = json::parse(body_content);
        cout << "Parsed JSON body:\n" << j.dump(4) << "\n"; // Pretty print JSON
    } catch (json::parse_error& e) {
        reportError(parser, "Failed to parse JSON: " + string(e.what()));
    }
}

// Function to parse XML body
void parseXmlBody(const string& body_content, HttpParser& parser) {
    tinyxml2::XMLDocument doc;
    if (doc.Parse(body_content.c_str()) != tinyxml2::XML_SUCCESS) {
        reportError(parser, "Failed to parse XML");
        return;
    }

    cout << "Parsed XML body:\n";
    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);
    cout << printer.CStr() << "\n"; // Print the XML content
}

// Helper function to detect the end of headers
bool isEndOfHeaders(const string& line) {
    return line.empty();
}

// Main function to simulate parsing of an HTTP message asynchronously
void handleParsingAsync(const string& http_message_chunk, HttpParser& parser) {
    vector<string> lines;
    istringstream iss(http_message_chunk);
    string line;

    // Split the incoming chunk into lines
    while (getline(iss, line)) {
        lines.push_back(line);
    }

    ParseState state = parser.method.empty() ? REQUEST_LINE : HEADERS;
    vector<string> headers;

    for (const auto& line : lines) {
        if (state == REQUEST_LINE) {
            parseRequestLine(line, parser);
            if (parser.hasError) {
                if (parser.onError) parser.onError(parser.errorMessage);
                return;  // Stop if error occurred
            }
            state = HEADERS;
        } else if (state == RESPONSE_LINE) {
            parseResponseLine(line, parser);
            if (parser.hasError) {
                if (parser.onError) parser.onError(parser.errorMessage);
                return;  // Stop if error occurred
            }
            state = HEADERS;
        } else if (state == HEADERS) {
            if (isEndOfHeaders(line)) {
                state = BODY;
                parseHeaders(headers, parser);
                if (parser.hasError) {
                    if (parser.onError) parser.onError(parser.errorMessage);
                    return;  // Stop if error occurred
                }
            } else {
                headers.push_back(line);
            }
        } else if (state == BODY) {
            // Check for multipart/form-data
            auto content_type_it = parser.headers.find("Content-Type");
            if (content_type_it != parser.headers.end() && 
                content_type_it->second.find("multipart/form-data") != string::npos) {
                parseMultipartBody(line, parser);
                if (parser.hasError) {
                    if (parser.onError) parser.onError(parser.errorMessage);
                    return;  // Stop if error occurred
                }
            } else if (content_type_it != parser.headers.end() && 
                       content_type_it->second.find("application/json") != string::npos) {
                parseJsonBody(line, parser);
                if (parser.hasError) {
                    if (parser.onError) parser.onError(parser.errorMessage);
                    return;  // Stop if error occurred
                }
            } else if (content_type_it != parser.headers.end() && 
                       content_type_it->second.find("application/xml") != string::npos) {
                parseXmlBody(line, parser);
                if (parser.hasError) {
                    if (parser.onError) parser.onError(parser.errorMessage);
                    return;  // Stop if error occurred
                }
            } else {
                appendBody(line, parser);
            }
        }
    }

    // Call onComplete callback if provided
    if (!parser.hasError && parser.onComplete) {
        parser.onComplete(parser);
    }
}

int main() {
    // Example HTTP request (with JSON body)
    string http_request_chunk = 
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 42\r\n"
        "\r\n"
        "{\"key\":\"value\", \"number\":123}\n";

    // Initialize the HTTP parser structure
    HttpParser parser;

    // Set up callbacks for async parsing
    parser.onComplete = [](const HttpParser& completedParser) {
        cout << "\n--- Parsed HTTP Request ---\n";
        cout << "Method: " << completedParser.method << "\n";
        cout << "URL: " << completedParser.url << "\n";
        cout << "Headers:\n";
        for (const auto& header : completedParser.headers) {
            cout << header.first << ": " << header.second << "\n";
        }
        cout << "Body: " << completedParser.body << "\n";

        // Output multipart parts
        cout << "Multipart Parts:\n";
        for (const auto& part : completedParser.multipart_parts) {
            cout << "Part Name: " << part.name << ", Filename: " << part.filename 
                 << ", Content Type: " << part.content_type << "\n";
            cout << "Content: " << part.content.substr(0, 20) << "...\n"; // Print first 20 chars of content
        }
    };

    parser.onError = [](const string& error) {
        cerr << "Error occurred during parsing: " << error << "\n";
    };

    // Simulate asynchronous parsing of the HTTP request
    handleParsingAsync(http_request_chunk, parser);

    // Clear the parser for the next message
    parser = HttpParser();

    return 0;
}
