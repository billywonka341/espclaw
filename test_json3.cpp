#include <iostream>
#include <fstream>
#include "ArduinoJson.h"

int main() {
    std::string body = "{\n  \"error\": {\n    \"code\": 400,\n    \"message\": \"API key not valid\"\n  }\n}";
    
    ArduinoJson::JsonDocument filter;
    filter["candidates"][0]["content"]["parts"][0]["text"] = true;
    filter["error"] = true;
    
    ArduinoJson::JsonDocument responseDoc;
    ArduinoJson::DeserializationError error = deserializeJson(responseDoc, body, ArduinoJson::DeserializationOption::Filter(filter));
    
    std::string out;
    serializeJson(responseDoc, out);
    std::cout << "Error: " << error.c_str() << std::endl;
    std::cout << "Contains error? " << responseDoc.containsKey("error") << std::endl;
    std::cout << "Doc: " << out << std::endl;
    
    return 0;
}
