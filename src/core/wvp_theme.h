// The host theme: After Effects' UI background colour, classified and turned into text the page can use.
#pragma once

#include <string>

namespace wvp {

struct Theme {
  int red = 0;  // 0..255
  int green = 0;
  int blue = 0;
  double luminance = 0;  // 0 (black) .. 1 (white)
  bool dark = true;
};

// From 16-bit channels (what PF_AppGetBgColor reports, 0..65535).
Theme themeFromChannels16(int red, int green, int blue);

// "rgb(r, g, b)".
std::string themeCss(const Theme& theme);

// {"background":"rgb(...)","dark":true,"luminance":0.12,"red":..,"green":..,"blue":..}
std::string themeJson(const Theme& theme);

}  // namespace wvp
