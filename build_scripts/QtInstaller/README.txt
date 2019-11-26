Deployment and installation of a Qt application in Linux, MacOSX, and Windows.
Reference:
https://www.walletfox.com/course/qtinstallerframeworkexample.php
https://doc.qt.io/qtinstallerframework/ifw-tutorial.html
https://doc.qt.io/qtinstallerframework/operations.html
https://forum.qt.io/topic/74181/installer-framework-createdesktopentry-help/2

Step to setup installers:
Linux setup: Greating AppImage and Linuxdeployqt:
1. qmake config="release" && make
2. mkdir dist
3. make INSTALL_ROOT=`pwd`/dist install
4. cd dist && linuxdeployqt usr/share/applications/*.desktop -bundle-non-qt-libs -verbose=3 -no-strip
5. appimagetool .  appImage.unseen.x84.app 
6. cp appImage.unseen.x84.app build_scripts/QtInstaller/packages/org.qtproject.ifw.example/data/unseenp2p.app
7. cp data/unseenp2p.xpm build_scripts/QtInstaller/packages/org.qtproject.ifw.example/data/
8. binarycreator --offline-only -c config/config.xml -p packages unseenp2p-ubuntu-installer -v


Setup Mac OSX with QT
"1. Build > Clean Project “ProjectName”.
2. Choose “release” build in project tab
3. Build > Build project “UnseenP2P” 
4. Go to ""build-RetroShare-Desktop_Qt_5_9_0_clang_64bit-Release"" folder
5. sudo macdeployqt unseenp2p.app -dmg (without -qml option)
or sudo macdeployqt unseenp2p.app -dmg -qmldir=/Users/ductai/RetroShare/retroshare-qml-app/src"
6. Creating Portal package with symlink to /Applications Folder
   1. mkdir ui-release
   2. cd ui-release
   3. ln -s /Applications
   4. cd ..
   5. hdiutil create -volname UnseenPortal -srcfolder ui-release/ -ov -format UDZO unseenp2p.dmg

7. Creating QT installer package
   1. copy unseenp2p.app to build_scripts/QtInstaller/packages/org.qtproject.ifw.example/data
   2. cd build_script &&  binarycreator --offline-only -c config/config.xml -p packages unseenp2p-version-macosx-installer -v 
