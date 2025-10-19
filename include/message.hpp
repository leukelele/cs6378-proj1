#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <vector>
#include <string>

struct Message {
    int sender_id;
    std::vector<int> vector_clock;
    std::string payload;
};

#endif // MESSAGE_HPP
