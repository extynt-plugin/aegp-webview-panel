// Light/dark classification by Rec. 709 luminance, plus the theme's CSS and JSON (no printf, so no locale).
#include "wvp_theme.h"

#include <algorithm>
#include <cmath>

namespace wvp {

namespace {

constexpr int kChannelMax = 255;
constexpr double kDarkBelow = 0.5;

int to8Bit(int channel16) { return std::clamp(channel16, 0, 65535) >> 8; }

double luminanceOf(const Theme& theme) {
  return (0.2126 * theme.red + 0.7152 * theme.green + 0.0722 * theme.blue) / kChannelMax;
}

std::string formatUnit(double value) {
  const int milli = static_cast<int>(std::lround(std::clamp(value, 0.0, 1.0) * 1000.0));
  std::string fraction = std::to_string(milli % 1000);
  fraction.insert(0, 3 - fraction.size(), '0');
  return std::to_string(milli / 1000) + "." + fraction;
}

}

Theme themeFromChannels16(int red, int green, int blue) {
  Theme theme;
  theme.red = to8Bit(red);
  theme.green = to8Bit(green);
  theme.blue = to8Bit(blue);
  theme.luminance = luminanceOf(theme);
  theme.dark = theme.luminance < kDarkBelow;
  return theme;
}

std::string themeCss(const Theme& theme) {
  return "rgb(" + std::to_string(theme.red) + ", " + std::to_string(theme.green) + ", " +
         std::to_string(theme.blue) + ")";
}

std::string themeJson(const Theme& theme) {
  return "{\"background\":\"" + themeCss(theme) + "\",\"dark\":" + (theme.dark ? "true" : "false") +
         ",\"luminance\":" + formatUnit(theme.luminance) + ",\"red\":" + std::to_string(theme.red) +
         ",\"green\":" + std::to_string(theme.green) + ",\"blue\":" + std::to_string(theme.blue) + "}";
}

}
