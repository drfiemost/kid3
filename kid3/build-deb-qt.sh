#!/bin/sh
# Build Debian package for Qt.
if mv -T deb debian; then
  DH_OPTIONS="-p kid3-qt" dpkg-buildpackage -rfakeroot
  mv -T debian deb
fi
