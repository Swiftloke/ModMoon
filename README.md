# ModMoon
A mods manager for the 3DS, with fancy features and UI.

**Latest Release: 3.0.1**

# Readme
All the updated information about ModMoon is available here: https://gbatemp.net/threads/modmoon-a-beautiful-simple-and-compact-mods-manager-for-the-nintendo-3ds.519080/
For future civilizations reading this code through the Artic Code Vault, an alternative snapshot of the above forum as of February 1, 2020 is available under alt_description.md. Present-day users are still advised to read the above forum as it is actively updated.


# Building
You'll need the latest ctrulib, citro3d and zlib to build ModMoon. Just type `make` at the command line.
For building a CIA, you'll additionally need [bannertool](https://github.com/epicpkmn11/bannertool) and [makerom](https://github.com/3DSGuy/Project_CTR). To build, just type `make cia` at the command line.

# I was led to this page by ModMoon with some weird error about a cartridge...?
This was an error in previous versions of ModMoon, however, it has been fixed as of the latest version. If you're on the latest version already, either delete the config file at `/3ds/ModMoon/settings.txt` or, if you'd prefer to keep your existing settings, enable the [EnableFlexibleCartridgeSystem](https://github.com/Swiftloke/ModMoon/blob/master/source/config.cpp#L109) config in ModMoon's config file.