-- mod-version:2 -- lite-xl 2.0
local core = require "core"
local config = require "core.config"
local process = require "process"

config.plugins.immersive_title = {
  mica = true
}

math.randomseed(os.time())

local lock = false
local last_window_title
local system_set_window_title = system.set_window_title
function system.set_window_title(title)
  last_window_title = title
  if not lock then
    system_set_window_title(title)
  end
end

local function init()
  local name = string.format("LITE_XL_%08x_%08x", system.get_time(), math.random() * 0xFFFFFFFF)
  lock = true
  system_set_window_title(name)
  local proc = assert(process.start({
    USERDIR .. "/dark_mode_monitor",
    tostring(config.plugins.immersive_title.mica),
    name
  }, {
    stdout = process.REDIRECT_PIPE,
    stderr = process.REDIRECT_STDOUT,
    stdin = process.REDIRECT_DISCARD,
  }), "cannot start theme monitor")
  
  core.add_thread(function()
    local buf = ""
    while true do
      local read = proc:read_stdout()
      if not read then break end
    
      buf = buf .. read
      if buf:find("found!", 1, true) then
        core.log_quiet("monitoring theme changes...")
        system_set_window_title(last_window_title)
        lock = false
        break
      end
      coroutine.yield()
    end
  end)
  
  local core_quit = core.quit
  function core.quit(force)
    proc:terminate()
    core_quit(force)
  end
end

init()
