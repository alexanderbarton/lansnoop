#include <ostream>

#include "event.pb.h"

std::ostream& operator<<(std::ostream&, const Lansnoop::Event&);
std::istream& operator>>(std::istream&, Lansnoop::Event&);
