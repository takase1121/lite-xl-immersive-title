-- mod-version:3
local core = require "core"
local common = require "core.common"
local config = require "core.config"
local Object = require "core.object"
local process = require "process"

config.plugins.immersive_title = common.merge(config.plugins.immersive_title, {
  -- extend window frame into client area
  extend_frame = true,
  -- the backdrop type
  backdrop_type = "mica",
  -- changes theme based on Windows settings
  adaptive_theme = true,
  -- default dark theme
  theme_dark = "colors.default",
  -- default light theme
  theme_light = "colors.summer",

  config_spec = {
    name = "Mica",
    {
      label = "Extend window frame into client area",
      description = "When enabled, the window frame will be extended into the window, "
                    .. "applying Mica/Acrylic onto the window itself.",
      path = "extend_frame",
      type = "toggle",
      default = true
    },
    {
      label = "Backdrop type",
      description = "The type of backdrop for the window frame. "
                    .. "Only supports Windows 11 build 22621 and above.",
      path = "backdrop_type",
      type = "selection",
      values = {
        {"Default", "default" },
        { "None", "none" },
        { "Mica", "mica" },
        { "Acrylic", "acrylic" },
        { "Mica (Tabbed)", "tabbed" }
      },
      default = "default",
    },
    {
      label = "Adaptive theming",
      description = "Changes the theme according to Windows settings.",
      path = "adaptive_theme",
      type = "toggle",
      default = true
    },
    {
      label = "Light theme",
      description = "The theme to use when Windows is set to use light theme.",
      path = "theme_light",
      type = "string",
      default = "colors.default"
    },
    {
      label = "Dark theme",
      description = "The theme to use when Windows is set to use dark theme.",
      path = "theme_dark",
      type = "string",
      default = "colors.summer"
    }
  }
})

local Monitor = Object:extend()
function Monitor:new()
  self.proc = assert(process.start())
end

local function on_change()
end