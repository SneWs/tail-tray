#!/bin/bash

if [ -z "$1" ]; then
    echo "The build directory where the binary can be found is not set"
    exit
fi

buildDir="$1"

echo "Will use build directory $buildDir"

# Create the directories 
mkdir -p ./packaging/deb/tail-tray/usr/local/bin
mkdir -p ./packaging/deb/tail-tray/usr/local/lib/tail-tray
mkdir -p ./packaging/deb/tail-tray/usr/local/share/applications
mkdir -p ./packaging/deb/tail-tray/usr/local/share/icons/hicolor/128x128/apps

# Copy over files
cp "./icons/tailscale.png" ./packaging/deb/tail-tray/usr/local/share/icons/hicolor/128x128/apps
cp "./tail-tray.desktop"   ./packaging/deb/tail-tray/usr/local/share/applications
cp "$buildDir/tail-tray"   ./packaging/deb/tail-tray/usr/local/bin
cp  $buildDir/*.qm          ./packaging/deb/tail-tray/usr/local/lib/tail-tray

# Make sure the binary is executable
chmod +x ./packaging/deb/tail-tray/usr/local/bin/tail-tray

# And let's go to the deb pkg root and kick off the deb build
cd ./packaging/deb

# And build...
dpkg-deb --build tail-tray

# And back to root
cd ../../

# Copy over the DEB files to build output
cp ./packaging/deb/*.deb "$buildDir"

