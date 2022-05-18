## 资源
- https://github.com/tboox/ltui : 终端里面的 ui 库
```lua
-- really cold
local ltui        = require("ltui")
local application = ltui.application
local event       = ltui.event
local rect        = ltui.rect
local window      = ltui.window
local demo        = application()

function demo:init()
    application.init(self, "demo")
    self:background_set("blue")
    self:insert(window:new("window.main", rect {1, 1, self:width() - 1, self:height() - 1}, "main window", true))
end

demo:run()
```
- https://github.com/love2d/love : 游戏框架，里面的长颈鹿还挺好玩的

## 项目
- https://github.com/whitecatboard/Lua-RTOS-ESP32 : 使用 lua 在 esp32 上写 rtos，简直要素拉满啊
- metatables
  - https://hiphish.github.io/blog/2022/03/15/lua-metatables-for-neovim-plugin-settings/
