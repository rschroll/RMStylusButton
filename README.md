# RMStylusButton

RMStylusButton is a small stand-alone program that runs on the reMarkable 2 and converts button presses into actions.  It should work with any EMR (Wacom-compatible) stylus.  It has been tested with the [Lamy ALstar EMR stylus](https://www.lamy.com/en/digital-writing/classic-meets-smartness/#alstaremr), and similar tools have reported success with:
 * Kindle Scribe Pen
 * Samsung S6 S Pen
 * Wacom One Pen CP91300B2Z

## Installation

Installation of RMStylusButton requires an SSH connection to your reMarkable device.   This [reMarkable guide page](https://remarkable.guide/guide/access/ssh.html) can get you started.

To download RMStylusButton, SSH into your device and run
```
wget https://github.com/rschroll/RMStylusButton/releases/download/v3.0/RMStylusButton.tar.gz -O- | tar xz
```
This will download and unpack several files to a newly-created `RMStylusButton` directory.

To test it out, run
```
./RMStylusButton/RMStylusButton
```
(See below for usage details.)  Use `Ctrl-C` to stop the program.

To install RMStylusButton and have it start automatically, run
```
./RMStylusButton/manage.sh install
```
Your stylus button should work immediately, as well as after restarts.

### reMarkable upgrades

When you install an updated version of the reMarkable software, it will erase the changes that cause RMStylusButton to start automatically.  The files downloaded above should still be present, so just SSH in and again run
```
./RMStylusButton/manage.sh install
```

### Uninstalling

If you don't want RMStylusButton active anymore, SSH into your device and run
```
./RMStylusButton/manage.sh uninstall
```
If you desire, you can also delete the downloaded files:
```
rm -r RMStylusButton
```

<details>
<summary><b>A note about security</b></summary>

Downloading and running binaries from random people on the internet is not a great idea, security-wise.  For openness, the binary is built on GitHub Actions.  You can checkout the [workflow](https://github.com/rschroll/RMStylusButton/blob/main/.github/workflows/build.yml) and examine the [build logs](https://github.com/rschroll/RMStylusButton/actions).  A `sha256sum` of the tarball is computed in the build process.  Use this to verify that the files you downloaded was the same as was built in the GitHub Action.  On either your reMarkable or your computer, run
```
sha256sum RMStylusButton.tar.gz
```
The output should be the same as in the GitHub Actions log for the version that you have downloaded.
</details>

## Usage

Press and hold the button on your stylus to erase.  Release the button to switch back to whatever pen you had selected.

Double-click the button to undo the most recent action.  Triple-click the button to redo that action.

### Configuration

RMStylusButton can be configured by options passed to the binary or to the `manage.sh install` command.  Valid options are

<table><tr>
<td><code>--toggle</code></td>
<td>Single presses of the button toggle between erase mode and pen mode.</td>
</tr></table>

## Development

To build RMStylusButton for the reMarkable, you need the [reMarkable toolchain](https://remarkable.guide/devel/toolchains.html).  I've been using the [official 4.0.117 release for the reMarkable 2](https://storage.googleapis.com/remarkable-codex-toolchain/remarkable-platform-image-4.0.117-rm2-public-x86_64-toolchain.sh).  The toolchain is intended for Linux machines, but I know people have gotten it to run in Docker.

This file is a self-extracting shell script, which unpacks, by default to `/opt/codex`.  You may need to run the script as root.

You will need to `source` the environment-setup file that got installed with the toolchain to get everything set up for cross-compilation.  Then, simply running `make` should build the RMStylusButton binary.  `make RMStylusButton.tar.gz` will assemble a tarball with the binary and the `manage.sh` script.

To help with development, the `make deploy` target attempts to copy the tarball over to the reMarkable device.  It assumes that it can be reached at `remarkable.local`.  If that's not the case, set the `rmdevice` environmental variable with the right host name or IP address.

### How it works

Stylus events appear in `/dev/input/event1` on the reMarkable 2.  RMStylusButton watches this event stream for button presses.  When it sees events indicating that erasing should happen, it injects events corresponding to the eraser button.

Undo and redo actions are triggered by creating a uinput keyboard device and injecting `Ctrl-Z` and `Ctrl-Y` events.

## Alternatives

Other tools provide similar capabilities to RMStylusButton, but with different features and mechanisms.  Here are the ones I know about.

- [RemarkableLamyEraser](https://github.com/isaacwisdom/RemarkableLamyEraser) was the starting point for this project.  This project [has been paused](https://github.com/isaacwisdom/RemarkableLamyEraser/issues/70) by the developer.
- [slotThe's fork](https://github.com/slotThe/RemarkableLamyEraser) continues the work towards version 2 of RemarkableLamyEraser.  This features a configuration system to allow many different events to be triggered by a wide variety of button click patterns.  These events are triggered by simulating touch events on the relevant UI elements on the tablet screen.  This makes it sensitive to whether the device is in left-handed mode or landscape mode.  It is also vulnerable to changes to the UI in new versions of the reMarkable software.
- [remarkable-stylus](https://github.com/ddvk/remarkable-stylus) provides similar capabilites for users of the [remarkable-hacks](https://github.com/ddvk/remarkable-hacks) modified UI.

## Copyright

Copyright 2023-2024 Robert Schroll
<br/>
Copyright 2021-2023 Isaac Wisdom

RMStylusButton is released under the GPLv3.  See LICENSE for details.
