#include <ostream>

#include "event.pb.h"

std::ostream& operator<<(std::ostream&, const Lansnoop::Event&);
std::istream& operator>>(std::istream&, Lansnoop::Event&);

//  Nonblocking read.
//  Reads up to one event from the input stream.
//  Returns true iff an event was read.
//  Returns false if no event was read.
//  Throws on error.
bool read_event_nb(std::istream&, Lansnoop::Event& event);
