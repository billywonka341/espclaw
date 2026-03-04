#include <iostream>
#include <fstream>
#include "ArduinoJson.h"

int main() {
    std::string chunked_body = "d2\r\n{\n  \"error\": {\n    \"code\": 400,\n    \"message\": \"API key not valid\"\n  }\n}\r\n0\r\n\r\n";
    
    ArduinoJson::JsonDocument filter;
    filter["candidates"][0]["content"]["parts"][0]["text"] = true;
    filter["error"] = true;
    
    ArduinoJson::JsonDocument responseDoc;
    ArduinoJson::DeserializationError error = deserializeJson(responseDoc, chunked_body, ArduinoJson::DeserializationOption::Filter(filter));
    
    std::string out;
    serializeJson(responseDoc, out);
    std::cout << "Type: " << out << std::endl;
    
    return 0;
}
