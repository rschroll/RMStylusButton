#!/bin/bash
set -e
cd "$(dirname "$0")"

SERVICE_NAME="RMStylusButton.service"
SERVICE_FILE="/lib/systemd/system/$SERVICE_NAME"

if [[ $# -ge 1 ]]; then
    command=$1
    shift
    args=$*
fi

disable_service() {
    # These will fail if the service isn't installed, but that's fine.
    set +e
    systemctl stop $SERVICE_NAME 2&> /dev/null
    systemctl disable $SERVICE_NAME 2&> /dev/null
    set -e
}

if [[ $command == "install" ]]; then
    disable_service

    echo "Installing service file at $SERVICE_FILE"
    cat <<EOD > $SERVICE_FILE
[Unit]
Description=Stylus Button Input
After=xochitl.service

[Service]
ExecStart=$(pwd)/RMStylusButton $args
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOD

    echo "Enabling service"
    systemctl daemon-reload
    systemctl enable $SERVICE_NAME
    systemctl start $SERVICE_NAME

elif [[ $command == "uninstall" ]]; then
    disable_service

    echo "Removing service file from $SERVICE_FILE"
    rm -f $SERVICE_FILE

    echo "Disabling auto-start"
    systemctl daemon-reload
    systemctl reset-failed

else
    cat <<EOD
RMStylusButton management script
Usage: manage.sh command [args]

command may be one of:
    install     Install or update RMStylusButton
    uninstall   Uninstall RMStylusButton

args may be zero or more of
    --toggle    The stylus button will toggle between writing and erasing,
                instead of erasing only when pressed.
    --verbose   Turn on verbose output from the service.
EOD
fi
