#### Build & Install from source
1. Install the following dependencies
   * Git, QT 6, cmake and a c++ compiler, for example:
      * On Ubuntu and Ubuntu based distros
         ```bash
         sudo apt install git qt6-tools-dev qt6-tools-dev-tools g++ clang cmake davfs2
         ```
     * On Fedora
        ```bash
        sudo dnf install -y git g++ clang cmake qt6-qtbase-devel qt6-qttools-devel qt6-qtbase-private-devel davfs2
        ```
      * On Arch Linux
        * You can use the AUR package from here https://aur.archlinux.org/packages/tail-tray-git provided by @HeavenVolkoff
        * or, build it from source yourself:
         ```bash
         sudo pacman -S git clang cmake qt6-base qt6-tools
         ```
        ```bash 
        # For davfs2 we need to use the AUR
        yay -S davfs2
        ```
      * On openSUSE Tumbleweed and Leap there is a RPM in the repos already. See https://software.opensuse.org/download/package?package=tail-tray&project=openSUSE%3AFactory and https://build.opensuse.org/package/show/network:vpn/tail-tray for details and updates in Factory. A big thank you to Johannes Kastl for setting up the RPM builds!
      * On Windows (Unsupported but currently working)
        Make sure to download and install QT binaries
      * If you are running Gnome and not using Ubuntu, make sure to install AppIndicator so you can see your tray icons. See https://extensions.gnome.org/extension/615/appindicator-support/
2. Clone the repo
3. cd into the repo `cd tail-tray`
4. Run `cmake -B "./build" -DCMAKE_BUILD_TYPE="Release"`
   * If you want to disable DAVFS: `cmake -B "./build" -DDAVFS_ENABLED=OFF -DCMAKE_BUILD_TYPE="Release"`
5. Run `cmake --build "./build" --config Release`
6. Run `cd build`
7. Run `sudo make install`
8. It will now be installed to `/usr/local/bin/tail-tray` and can be started by running `tail-tray` in a terminal or by clicking the Tail Tray icon in the launcher.