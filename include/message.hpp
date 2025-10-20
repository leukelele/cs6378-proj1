// include/message.hpp
#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <vector>
#include <string>
#include <sstream>

struct Message {
    int sender_id;
    std::vector<int> vector_clock; // carries VC only for application messages
    std::string payload;
};

// --- Minimal helpers for APP messages: "APP|<sender>|v0,v1,...|<payload>"
inline std::string encode_app_message(int sender_id,
                                      const std::vector<int>& vc,
                                      const std::string& payload)
{
    std::ostringstream oss;
    oss << "APP|" << sender_id << "|";
    for (size_t i = 0; i < vc.size(); ++i) {
        if (i) oss << ",";
        oss << vc[i];
    }
    oss << "|" << payload;
    return oss.str();
}

inline bool decode_app_message(const std::string& s,
                               int &sender_id,
                               std::vector<int>& vc_out,
                               std::string& payload_out)
{
    // Expect 4 parts split by '|'
    size_t p1 = s.find('|');
    if (p1 == std::string::npos) return false;
    if (s.substr(0, p1) != "APP") return false;

    size_t p2 = s.find('|', p1 + 1);
    size_t p3 = s.find('|', p2 == std::string::npos ? p1 + 1 : p2 + 1);
    if (p2 == std::string::npos || p3 == std::string::npos) return false;

    try {
        sender_id = std::stoi(s.substr(p1 + 1, p2 - (p1 + 1)));
    } catch (...) { return false; }

    // parse VC "v0,v1,..."
    vc_out.clear();
    std::string vc_str = s.substr(p2 + 1, p3 - (p2 + 1));
    std::istringstream vcss(vc_str);
    std::string token;
    while (std::getline(vcss, token, ',')) {
        if (token.empty()) continue;
        try { vc_out.push_back(std::stoi(token)); }
        catch (...) { return false; }
    }
    payload_out = s.substr(p3 + 1);
    return true;
}

#endif // MESSAGE_HPP
