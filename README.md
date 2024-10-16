# RapidHTTP
# HTTP Parser

## Overview

The HTTP Parser is a C++ library designed to parse HTTP requests and responses. It is a lightweight, easy-to-use parser that extracts essential components from HTTP messages, making it ideal for developers working on web applications or services.

## Features

- **Request Parsing:** Supports parsing of HTTP request lines, headers, and bodies.
- **Response Parsing:** Handles parsing of HTTP response lines, headers, and bodies.
- **Easy Integration:** Can be easily integrated into existing projects.
- **Extensible Design:** Allows for future enhancements, such as supporting chunked transfer encoding and streaming.

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/http-parser.git
   ```

2. Navigate to the project directory:
   ```bash
   cd http-parser
   ```

3. Compile the code (assuming you have a compatible C++ compiler installed):
   ```bash
   g++ -o http_parser main.cpp
   ```

4. Run the executable:
   ```bash
   ./http_parser
   ```

## Usage

### Parsing HTTP Requests

You can parse HTTP requests by passing a vector of strings that represent the lines of the HTTP request. Here’s a simple example:

```cpp
#include <iostream>
#include <vector>
using namespace std;

// Example of parsing an HTTP request
vector<string> http_request = {
    "GET /index.html HTTP/1.1",
    "Host: example.com",
    "Connection: keep-alive",
    "Accept: text/html",
    "",
    "Body content here (optional, usually for POST requests)"
};

HttpParser parser;
handleParsing(http_request, parser, true);
```

### Parsing HTTP Responses

To parse HTTP responses, use a similar approach with a vector of strings representing the lines of the HTTP response:

```cpp
#include <iostream>
#include <vector>
using namespace std;

// Example of parsing an HTTP response
vector<string> http_response = {
    "HTTP/1.1 200 OK",
    "Content-Type: text/html",
    "Content-Length: 1234",
    "Connection: keep-alive",
    "",
    "<html><body>Hello, world!</body></html>"
};

HttpParser parser;
handleParsing(http_response, parser, false);
```

## Contributing

Contributions are welcome! If you have suggestions for improvements or additional features, please open an issue or submit a pull request.

1. Fork the repository.
2. Create your feature branch:
   ```bash
   git checkout -b feature/my-feature
   ```
3. Commit your changes:
   ```bash
   git commit -m "Add some feature"
   ```
4. Push to the branch:
   ```bash
   git push origin feature/my-feature
   ```
5. Open a pull request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- Inspired by existing HTTP libraries and protocols.
- Thanks to the open-source community for their valuable contributions.