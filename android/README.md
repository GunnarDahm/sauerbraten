# Android port (test build)

The engine runs on Android through SDL2, with [gl4es](https://github.com/ptitSeb/gl4es)
translating its desktop OpenGL calls to OpenGL ES 2. USB and Bluetooth game controllers
work through SDL's GameController support.

## Getting a build

Every push builds an APK on GitHub Actions (workflow `build`, job "Android APK").
Download the `sauerbraten-android` artifact from the run and unzip it to get `app-release.apk`.

To build locally you need JDK 17, the Android SDK (platform 34) and NDK 27.2.12479018:

```sh
android/fetch-deps.sh
cd android && ./gradlew assembleRelease
```

The APK lands in `android/app/build/outputs/apk/release/`. It is signed with the
debug key, so it can be sideloaded but not uploaded to the Play Store.

## Installing and testing

```sh
adb install -r app-release.apk
adb logcat -s sauerbraten SDL GL4ES AndroidRuntime
```

On first launch the app unpacks about 640 MB of game data into its storage, which takes
a minute or so. Later launches go straight into the game.

### Controller

Pair the controller in Android's Bluetooth settings first; it can also be connected
or disconnected while the game is running.

| Control | In game | In menus |
|---|---|---|
| Left stick | move | move cursor |
| Right stick | look | move cursor |
| RT | attack | |
| LT (hold) | zoom | |
| A | jump | click |
| Left stick click | jump | |
| B | drop flag | back |
| X | cycle weapons | |
| RB / LB | next / previous weapon | |
| D-pad | up: rocket launcher, down: shotgun, left: chaingun, right: rifle | |
| Y / Back | scoreboard | |
| Start | main menu | close menu |

Buttons are bindable keys named `PAD_A`, `PAD_B`, ..., `PAD_LT`, `PAD_RT` (see
`data/gamepad.cfg`). Tunables: `paddeadzone`, `padyawspeed`, `padpitchspeed`,
`padlookcurve`, `padinvert`, `padcursorspeed`, `padmenubuttons`.

### Touch

Tap to click menu items. In game, dragging a finger turns the view. There are no
on-screen move or fire buttons yet, so playing needs a controller or keyboard.
The Android back button acts as Escape.
