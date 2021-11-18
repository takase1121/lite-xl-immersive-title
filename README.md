### Immersive Title Bar for Lite XL on Windows

Ever wondered why your programs look crap in dark mode? Is the light themed title bar killing your eyes?
No problem, this plugin solves it for you!

No longer will you need to wait for [this PR][1] to be merged by the contributors. Oh wait...

This plugin gets the HWND of the window and sets the appropriate attributes for title bar. It can also
enable [Mica][2] for the titlebar if you're on Windows 11 and [the feature isn't removed yet][3].

The best solution is to wait for SDL2 to do this. I'm not sure if they'll do it nor when will they do it.

### Install
1. Download the binary from the releases and put it in `%USERPROFILE%/.config/lite-xl`
2. Download init.lua and install it like a normal plugin
3. Voila!

Some settings are:
```lua
config.plugins.immersive_title.mica = true -- enables or disables mica
```


[1]: https://github.com/lite-xl/lite-xl/pull/514
[2]: https://docs.microsoft.com/en-us/windows/apps/design/style/mica
[3]: https://github.com/microsoft/microsoft-ui-xaml/issues/6186

