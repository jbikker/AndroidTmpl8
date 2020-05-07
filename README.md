# AndroidTmpl8
Template for C/C++ Android application development.

# What It Is
This template provides you with a convenient and lean starting point for C/C++ development on Android. The template code compiles as-is in Android Studio 3.6.3. It runs some basic OpenGLES code, and plays a sound file using the included SoLoud library. The included 'howto' text file explains how to quickly add additional source files and assets, how to modify the application icon and how to rename the project: all operations that can easily become major obstacles for a fun Android side project.

# Plus Windows
The template includes a Visual Studio project that compiles the same template source files, but for Windows. This lets you develop right on your desktop, without the need for an emulator, enabling the full debugging capabilities of Visual Studio. This significantly simplifies your development cycle and limits your exposure to Android Studio. Which is a good thing.

# Advanced
The current version of the template already starts a properly initialized full-screen OpenGLES native activity. You also get access to the pen position for basic controls. PNG images can be loaded straight from the apk, as if they are in the main application directory. SoLoud is used to playback audio. You get convenient file access for audio assets and other application data.

# What It Wants To Be
There are other things that are trivial on a desktop machine, but pretty hard on Android, such as accessing the file system and the camera. These will be tackled in later revisions of the template. The idea is to tackle these once and for all, so you don't have to scour the internet for code snippets.

# What You Need
Just install Android Studio 3.6.3 and Visual Studio 2019 (community edition will do). Then, open the template folder in Android Studio to compile for Android, or open vs2019/Tmpl8win.sln in Visual Studio for development. Read howto.txt for advice on common actions.

Jacco Bikker, May 2020
