# RMStylusButton
Standalone tool that turns the button on the Lamy Pen into an eraser on the reMarkable.



Also confirmed to work with these other styli:
 * Kindle Scribe Pen
 * Samsung S6 S Pen
 * Wacom One Pen CP91300B2Z

*As an alternative, consider using [this](https://github.com/ddvk/remarkable-stylus). (If you're already using ddvk-hacks, I'd defintely reccomend this route. This tool is for people who are looking for a less invasive option, and prefer the unaltered look of the reMarkable interface.)*

The tool will definitely break when the reMarkable updates. When that happens, just reinstall!
# Install Instructions

# Uninstall Instrucions


# Usage
Press and hold to erase, release to use as a normal pen. Double click the button to undo. Note that at the moment, double pressing to undo only works for portrait orientation documents.

Further customization can be done by adding arguments to ExecStart line of the RMStylusButton.service file. This can be opened with `nano /lib/systemd/system/RMStylusButton.service`.
The supported arguments are:
`--press`   Press and hold to erase, release to use as a normal pen. *This is the default behavior.*
`--toggle`  Press the button to erase, press the button again to swtich back to a normal pen.
`--double-press undo` Double click the button to undo. *This is the default behavior.*
`--double-press redo` Double click the button to redo.
`--left-handed` Use this option if you are using left handed mode.
For example, this line would use the toggle mode and redo on a double click:
`ExecStart=RMStylusButton --toggle --double-press redo`


To apply your config, run these commands:
``` Shell
systemctl stop RMStylusButton.service
systemctl daemon-reload
systemctl start RMStylusButton.service
```
# How it works
When you press the button on the Lamy Pen, an input event with code BTN_TOOL_RUBBER is sent into dev/input/event1. Essentially, this tricks the reMarkable into
thinking you are using the eraser side of the Marker Plus.

# How to build

* Download the latest toolchain for your device from <https://remarkablewiki.com/devel/toolchain> (e.g. `codex-x86_64-cortexa9hf-neon-rm10x-toolchain-3.1.15.sh`)
* run that file to install the toolchain (e.g. `sudo sh codex-x86_64-cortexa9hf-neon-rm10x-toolchain-3.1.15.sh`)
* source the printed environment file (e.g. `source /opt/codex/rm11x/3.1.15/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi`)
* compile `main.c` using the `CC` environment variable (e.g. `$CC -O2 main.c`)
    * if there is an error like `no such file or directory`, copy the command and execute it directly instead of using `$CC`, e.g. `arm-remarkable-linux-gnueabi-gcc  -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 --sysroot=/opt/codex/rm11x/3.1.15/sysroots/cortexa7hf-neon-remarkable-linux-gnueabi -O2 main.c`)

# TODO:
- [x] RM1 support (testers needed)
- [x] Nice install script
- [ ] toltec package
- [x] config file (as opposed to current command line argument system) -__V2__
- [x] flexible triggers (such as "click", "press and hold", "double click", "double click and hold", etc.) -__V2__
- [x] freely assignable actions (as listed below, able to assign to any trigger above) -__V2__

