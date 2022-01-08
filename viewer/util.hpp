#pragma once

#include <iosfwd>
#include <glm/glm.hpp>


std::ostream& operator <<(std::ostream& o, const glm::vec3& v);
std::ostream& operator <<(std::ostream& o, const glm::vec4& v);
