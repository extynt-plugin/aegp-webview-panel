// AE's UI background colour (16-bit channels from PF_AppGetBgColor) as light/dark, CSS and JSON.
#pragma once

#include <string>

namespace wvp {

struct Theme {
  int red = 0;
  int green = 0;
  int blue = 0;
  double luminance = 0;
  bool dark = true;
};

Theme themeFromChannels16(int red, int green, int blue);

std::string themeCss(const Theme& theme);

std::string themeJson(const Theme& theme);

}
