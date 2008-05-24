#!/bin/bash
#
# Stupid little Apple TV installer script for the Perian components using the
# zip files in the distribution preference pane.
#
# Should be wrapped up with the zip files using makeself:
#  makeself --nocrc --nocomp --nox11 ATVComponents Perian-ATV.run Perian ./ATV-Perian-Install.sh
#
# New for 1.1: $0 [action] [prefix]
#
# Where action is "install" (the default), "uninstall", or "help".
#
# Install/uninstall take a prefix of the root to handle (for such as a patchstick)

SRCDIR="$PWD"
PREFIX=${2:-}
PERIAN_DEST="${PREFIX}/Library/QuickTime"
A52_DEST="${PREFIX}/Library/Audio/Plug-Ins/Components"

ACTION=${1:-install}


if [ "$ACTION" = "help" ]; then
  echo "Usage: $0 [action] [prefix]"
  echo
  echo "Where action is one of: "
  echo "  install          Install Perian"
  echo "  uninstall        Uninstall Perian"
  echo "  help             This message"
  echo
  echo "The install/uninstall actions accept a prefix to prefix to all"
  echo "paths."
  
  exit 1
fi

# make sure we're running as root
if [ "$USER" != "root" ]; then
  echo "This installer must be run as root."
  echo
  echo "Please enter your password below to authorize as root."
  echo "In most cases, this password is \"frontrow\"."
  sudo "$0" $*
  exit 0
fi

REMOUNT=0

# Check if / is mounted readonly
# Using the prefix option assumes it's already read-write
if [ "$PREFIX" = "" ]; then
  if mount | grep ' on / ' | grep -q 'read-only'; then
    REMOUNT=1
    /sbin/mount -uw /
  fi
fi

if [ "$ACTION" = "uninstall" ]; then
  echo == Removing Perian
  /bin/rm -rf "$PERIAN_DEST/Perian.component"
  /bin/rm -rf "$PERIAN_DEST/AC3MovieImport.component"
  /bin/rm -rf "$A52_DEST/A52Codec.component"
  
  if [[ -d "${PREFIX}/System/Library/QuickTime (disabled)/QuickTimeH264.component" && ! -d "${PREFIX}/System/Library/QuickTime/QuickTimeH264.component" ]]; then
    echo
    echo "You have previously disabled Apple's H.264 component."
    echo
    echo -n "Do you wish to restore it? (Y/n)"
    read -e restoreapple
    if [[ "$restoreapple" == "" || "$restoreapple" == "Y" || "$restoreapple" == "y" ]]; then
      mv "${PREFIX}/System/Library/QuickTime (disabled)/QuickTimeH264.component" "${PREFIX}/System/Library/QuickTime"
    fi
  fi
else
  if [ -d "$PERIAN_DEST/Perian.component" -o -d "$PERIAN_DEST/AC3MovieImport.component" -o -d "$A52_DEST/A52Codec.component" ]; then
    echo == Removing old Perian
    /bin/rm -rf "$PERIAN_DEST/Perian.component"
    /bin/rm -rf "$PERIAN_DEST/AC3MovieImport.component"
    /bin/rm -rf "$A52_DEST/A52Codec.component"
  fi
  
  echo == Extracting Perian
  /usr/bin/ditto -k -x --rsrc "$SRCDIR/Perian.zip" "$PERIAN_DEST"
  /usr/bin/ditto -k -x --rsrc "$SRCDIR/AC3MovieImport.zip" "$PERIAN_DEST"
  /usr/bin/ditto -k -x --rsrc "$SRCDIR/A52Codec.zip" "$A52_DEST"

  # Warn about Apple H.264 if it exists
  if [ -d "${PREFIX}/System/Library/QuickTime/QuickTimeH264.component" ]; then
    echo
    echo "You currently have Apple's H.264 component installed."
    echo
    echo "If you wish to play high-profile H.264, you need to disable Apple's decoder to"
    echo "allow Perian to take over."
    echo
    echo -n "Do you wish to do this now? (Y/n) "
    read -e removeapple
    if [[ "$removeapple" == "" || "$removeapple" == "Y" || "$removeapple" == "y" ]]; then
      mkdir -p "${PREFIX}/System/Library/QuickTime (disabled)"
      mv "${PREFIX}/System/Library/QuickTime/QuickTimeH264.component" "${PREFIX}/System/Library/QuickTime (disabled)"
    fi
  fi
fi

if [[ "$PREFIX" = "" ]]; then
	echo "$FINDER must be restarted to complete the installation"
	echo
	echo -n "Would you like to do this now? (Y/n) "
	read -e dorestart
	if [[ "$dorestart" == "" ||  "$dorestart" == "y" ||  "$dorestart" == "Y" ]]; then
		echo
		echo "== Restarting $FINDER"
		
		kill `ps awx | grep "[F]inder" | awk '{print $1}'`
	fi
fi


# Remount root readonly if it was that way when we started
if [ "$REMOUNT" = "1" ]; then 
  /sbin/mount -ur /
fi

echo "Perian is now installed."
