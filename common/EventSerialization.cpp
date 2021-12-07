#include <stdexcept>
#include <vector>
#include <arpa/inet.h>
#include "EventSerialization.hpp"

/**
 *  In order to pack multiple protobuffer messages into a stream, we need
 *  to do our own framing.  So, each serialized message is preceeded by a
 *  4-byte (network byte order) value which tells the length of the
 *  serialized protobuf message immediately following.
 **/


std::ostream& operator<<(std::ostream& stream, const Lansnoop::Event& event)
{
    uint32_t serialized_length(htonl(event.ByteSizeLong()));
    stream.write(reinterpret_cast<char*>(&serialized_length), sizeof(serialized_length));
    event.SerializeToOstream(&stream);
    return stream;
}


std::istream& operator>>(std::istream& stream, Lansnoop::Event& event)
{
    uint32_t serialized_length;
    stream.read(reinterpret_cast<char*>(&serialized_length), sizeof serialized_length);
    if (stream.eof())
        return stream;
    if (!stream)
        throw std::runtime_error("bad field read at or before record length");
    serialized_length = ntohl(serialized_length);

    std::vector<char> serialized_buffer(serialized_length);
    stream.read(serialized_buffer.data(), std::streamsize(serialized_length));
    if (!stream)
        throw std::runtime_error("bad field read at or before record");

    if (!event.ParseFromArray(serialized_buffer.data(), serialized_length))
        throw std::runtime_error("failed deserializing event");

    return stream;
}
