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


bool read_event_nb(std::istream& stream, Lansnoop::Event& event)
{
    //  Read the 4-byte message length field.
    //
    uint32_t serialized_length;
    char* ptr = reinterpret_cast<char*>(&serialized_length);
    char* end = ptr + sizeof(serialized_length);
    std::streamsize n = stream.readsome(ptr, end-ptr);
    if (n == 0)
        return false;
    ptr += n;
    while (ptr < end) {
        std::streamsize n = stream.readsome(ptr, end-ptr);
        ptr += n;
        //  TODO: fix this
    }
    serialized_length = ntohl(serialized_length);

    //  Read the full message.
    //  Block until we get the full message.  It will *probably*
    //  arrive in short order.
    //
    std::vector<char> serialized_buffer(serialized_length);
#if 1
    stream.read(serialized_buffer.data(), std::streamsize(serialized_length));
    if (!stream)
        throw std::runtime_error("bad field read at or before record");
#else
    std::streamsize nread = 0;
    while (nread < serialized_length) {
        n = stream.read(serialized_buffer.data() + nread, std::streamsize(serialized_length) - nread);
        if (!n)
            throw std::runtime_error("read_event_nb(): incomplete read");
        nread += n;
        //  TODO: check for stream failure.
    }
#endif

    if (!event.ParseFromArray(serialized_buffer.data(), serialized_length))
        throw std::runtime_error("failed deserializing event");

    return true;
}
