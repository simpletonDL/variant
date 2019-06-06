#pragma once

#include "variant.hpp"

extern template class util::variant<int, double, float>;
using total_variant = util::variant<int, double, float>;