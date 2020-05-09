# ModMoon
A mods manager for the 3DS, with fancy features and UI.

**Latest Release: 3.0.1**

# Readme
All the updated information about ModMoon is available here: https://gbatemp.net/threads/modmoon-a-beautiful-simple-and-compact-mods-manager-for-the-nintendo-3ds.519080/
For future civilizations reading this code through the Artic Code Vault, an alternative snapshot of the above forum as of February 1, 2020 is available under alt_description.md. Present-day users are still advised to read the above forum as it is actively updated.


# Building
You'll need the latest ctrulib, citro3d and zlib to build ModMoon. Just type "make" at the command line.

# I was led to this page by ModMoon with some weird error about a cartridge...?
Ack... This is a fun one. I wrote a special error message at [modpackdownload.cpp](https://github.com/Swiftloke/ModMoon/blob/master/source/modpackdownload.cpp#L141) in the unlikely circumstance that A. A user had just finished downloading a modpack, very slowly, through the 3DS' slow wifi card, and B. the modpack had no modpackinfo.txt, therefore requiring the user to report to ModMoon what game the modpack went to. ModMoon implements this by opening the Active Title Select menu in a special mode that returns the first title selected to the caller (the modpack downloader.) However, if the intended destination for the modpack is on a cartridge, and that cartridge is not currently inserted, the user will be unable to select a destination without restarting the software to allow MM to recognize the new cartridge.

I wrote that error message directing the user here to apologize to them. Sorry that you lost your downloaded modpack... I'm sure it was big and very time-consuming to download :( As it turns out, ModMoon DOES have support for on-the-fly removal and insertion of cartridges. I put in two weeks to support one of ModMoon's hardest to implement features ever. It allows the OS to alert MM to insertion and removal of cartridges, and MM handles the result. [Here is the relevant commit that adds support.](https://github.com/Swiftloke/ModMoon/commit/cb56019642ed05d363b0c897304b4c599d77191b) It took a solid two weeks of hard work and screaming at random segfaults to get it right. In the end, however, before release, [I was forced to disable the feature](https://github.com/Swiftloke/ModMoon/commit/fe620a7175b639775b887d17a4537884468ce809) due to a [very strange bug regarding shutdown time that I never actually figured out.](https://github.com/smealum/ctrulib/issues/410)

You can see the flexible cartridge system, as I dubbed it, for yourself by enabling the [EnableFlexibleCartridgeSystem](https://github.com/Swiftloke/ModMoon/blob/master/source/config.cpp#L109) config in ModMoon's config file. Again, sorry for your loss of time. I hope this detailed explanation at least helped soften the blow.