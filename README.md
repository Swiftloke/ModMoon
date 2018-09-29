# ModMoon
A mods manager for the 3DS, with fancy features and UI.

**Latest Release: 3.0.1**

# Readme
All the updated information about ModMoon is available here: https://gbatemp.net/threads/modmoon-a-beautiful-simple-and-compact-mods-manager-for-the-nintendo-3ds.519080/


# Building
You'll need the latest ctrulib and citro3d to build ModMoon. Just type "make" at the command line.
If you're building a 3DSX build (anything *except* "make cia"), you'll need to define BUILTFROM3DSX somewhere in the code. This allows the downloader to correctly do its thing. Otherwise, it will accidentally install updates as CIA files.