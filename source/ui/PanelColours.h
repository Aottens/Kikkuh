#pragma once

#include <cstdint>

// FAST: the aluminium plate. Yellow means one thing: a kick sounds.
namespace panel
{
inline constexpr std::uint32_t plate = 0xffc8cbc6, ink = 0xff141513, print = 0xff383b37, well = 0xffacb0a9, sounding = 0xfff7c600;
// FAST-05m: the printed lines between the sections, as x in stage points.
inline constexpr int sectionLines[] = { 196, 478, 671, 860, 966 };
}
