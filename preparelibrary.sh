#!/bin/sh
git submodule update --init --recursive
cd TwitterOAuth
  ./preparelibrary.sh
  xcodebuild -project TwitterOAuth.xcodeproj -scheme TwitterOAuth -configuration Release
  buildPath=DerivedData/Build/Products/Release
  cp ${buildPath}/*.a lib/.
  cp ${buildPath}/usr/local/include/* include/.
cd ../
mv TwitterOAuth/lib .
mv TwitterOAuth/include .
