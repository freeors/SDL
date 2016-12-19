#SDL
http://www.libsdl.cn: Simple Chinese web site of SDL.

###Why is there this project
---
I am developing [Rose](https://github.com/freeors/Rose), a cross-platform C++ SDK based on SDL. Because used to GitHub, hope to push patch of SDL into it. But Official SDL is in "Mercurial", and replies that [there is no plan to GitHub](http://forums.libsdl.org/viewtopic.php?t=11578).

As long as possible, I will push these patch into the source tree of official SDL.

###Relation with Rose
---
You can take this project as an independent SDL source tree, and use it as the official SDL.

Of course, we recommend using with Rose, which can not only reuse experienced code written by other developer, but also simplify the process of compiling. For example, Studio app provided by Rose can help you automatically generate Android project file.

The following figure indicates the location of this project in the Rose's Work Kit.

![](http://www.libsdl.cn/download/sdl_location.png)

At present, Rose mainly for Windows, iOS and Android, so modification on SDL is also focused on them. For IDE, if you use the following tools, it will be very simple, otherwise it is very annoying, and may not be able to compile.

* Windows：Visual Studio 2017 or higher version
* iOS：Xcode 9.2 or higher version
* Android：Android Studio 3.0.1 + NDK-r16b or higher version

###Official SDL based on
---
This project integrates SDL as well as the derivative library.

* SDL2: 2.0.7     --20171023
* SDL2_image: 2.0.1
* SDL2_mixer: 2.0.1
* SDL2_net: 2.0.1
* SDL2_ttf: 2.0.14
* libogg-1.3.0: On iOS, compile SDL2_mixer requires it.
* libvorbis-1.3.2: On iOS, compile SDL2_mixer requires it.
* libvpx: --20161003
* ffmpeg: --20161213
* opencv: 3.4.1

###Major modification
----
#####Bluetooth(BLE)
Both Android and iOS have achieved all of the following functions, Windows is still in the laboratory version, especially WIN_WriteCharacteristic.

* SDL_BleScanPeripherals
* SDL_BleStopScanPeripherals
* SDL_BleReleasePeripheral
* SDL_BleConnectPeripheral
* SDL_BleDisconnectPeripheral
* SDL_BleGetServices
* SDL_BleGetCharacteristics
* SDL_BleReadCharacteristic
* SDL_BleWriteCharacteristic
* SDL_BleNotifyCharacteristic
* SDL_BleUuidEqual

#####File system
Windows has achieved all of the following functions. Android and iOS only to achieve part of the function.

* SDL_IsRootPath
* SDL_IsDirectory
* SDL_IsFile
* SDL_OpenDir
* SDL_ReadDir
* SDL_CloseDir
* SDL_GetStat
* SDL_MakeDirectory: Create directory
* SDL_DeleteFiles: Delete directory or file
* SDL_CopyFiles: Copy directory or file

#####Audio

* Modify SDL_PauseAudio, in order to make Android, iOS save power when playing sound.
* Modify the sound playback mechanism on Android, using Service instead of thread, in order to support the app can continue to play the sound in background.



###为什么会有这项目
---
正在写[Rose](https://github.com/freeors/Rose)，一个基于SDL的C++跨平台库，由于习惯了把源码放在GitHub，希望把修改后的SDL也放在那里。官方SDL是放在"Mercurial"，而且回复说[暂没有在GitHub开设的计划](http://forums.libsdl.org/viewtopic.php?t=11578)。

只要有可能，会尽快把新写的代码融入官方源码树。

###和Rose关系
---
你可以把此项目做为独立的SDL源码树，像官方SDL一样使用它。

当然，我们推荐和Rose配合使用，这不仅可重用它人写的功能，还能简化编译流程。举个例子，Rose提供的Rose Studio可帮你自动生成Android工程文件。

下图指示了本项目在Rose工作包中位置。

![](https://github.com/freeors/SDL/blob/master/sdl_location.png)

目前，Rose主要针对Windows、iOS、Android，于是对SDL的修改也集中在这三个平台。对IDE，如果你使用以下工具，那就会很简单，否则很烦，甚至有可能无法编译。

* Windows：Visual Studio 2017或更高版本
* iOS：Xcode 9.2或更高版本
* Android：Android Studio 3.0.1 + NDK-r16b或更高版本

###基于的SDL
---
该项目集成了SDL以及衍生库。

* SDL2: 2.0.7     --20171023
* SDL2_image: 2.0.1
* SDL2_mixer: 2.0.1
* SDL2_net: 2.0.1
* SDL2_ttf: 2.0.14
* libogg-1.3.0: iOS平台，编译SDL2_mixer时需要这个库。
* libvorbis-1.3.2: iOS平台，编译SDL2_mixer时需要这个库。
* libvpx: --20161003
* ffmpeg: --20161213
* opencv: 3.4.1

###主要修改
----
#####蓝牙（BLE）
iOS、Android已实现了以下所有函数，Windows尚处在实验室版本，尤其WIN_WriteCharacteristic。

* SDL_BleScanPeripherals
* SDL_BleStopScanPeripherals
* SDL_BleReleasePeripheral
* SDL_BleConnectPeripheral
* SDL_BleDisconnectPeripheral
* SDL_BleGetServices
* SDL_BleGetCharacteristics
* SDL_BleReadCharacteristic
* SDL_BleWriteCharacteristic
* SDL_BleNotifyCharacteristic
* SDL_BleUuidEqual

#####文件系统
Windows已实现了以下所有函数，iOS、Android只实现了部分功能，具体参考代码。

* SDL_IsRootPath
* SDL_IsDirectory
* SDL_IsFile
* SDL_OpenDir
* SDL_ReadDir
* SDL_CloseDir
* SDL_GetStat
* SDL_MakeDirectory：创建目录。
* SDL_DeleteFiles：删除文件或目录
* SDL_CopyFiles：复制文件或目录

#####声音

* 修改了SDL_PauseAudio，让iOS、Android在播放声音时更省电。
* 修改了Android声音播放机制，用Service代替线程，以支持app在后台时能继续播放声音。