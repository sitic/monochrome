# Installing the Standalone Version

## Windows

Download `Monochrome.exe` from the [latest release](https://github.com/sitic/monochrome/releases/latest) and run it. You may need to install [Microsoft Visual C++ Redistributable 2019](https://aka.ms/vs/16/release/vc_redist.x86.exe) if you get an error about missing `VCRUNTIME140_1.dll`.

```{figure} /_static/Windows_Install.png
:class: bg-primary
:width: 75%
:align: center
:alt: Windows Defender SmartScreen warning

Because Monochrome is not digitally signed, Windows Defender might show a warning when you try to launch it. This is normal. To proceed, click on "More info" and then "Run anyway".
```

## macOS

Download the `.dmg` file from the [latest release](https://github.com/sitic/monochrome/releases/latest) and run it to copy `Monochrome.app` to your Applications folder. When launching Monochrome for the first time, you may need to right-click on the app in Finder and select "Open" to bypass the security warning. This is because the app is not notarized by Apple, which is a requirement for apps distributed outside of the Mac App Store. If the warning persists, you can remove the quarantine attribute from the app by running the following command in Terminal:

```bash
xattr -d com.apple.quarantine /Applications/Monochrome.app
```

For more information, see the "[open a Mac app from an unidentified developer](https://support.apple.com/guide/mac-help/open-a-mac-app-from-an-unidentified-developer-mh40616/mac)" guide from Apple.

## Linux

Download the `.AppImage` file from the [latest release](https://github.com/sitic/monochrome/releases/latest) and run it. You may need to mark the file as executable, see the [AppImage documentation](https://docs.appimage.org/introduction/quickstart.html) for more information.
