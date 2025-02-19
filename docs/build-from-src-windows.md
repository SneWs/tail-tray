### Building on Windows
0. Install QT 6
1. Clone the repo
2. cd  into the repo `cd tail-tray`
3. Make a build directory `mkdir build`
4. Run `cmake -B "./build" -DCMAKE_BUILD_TYPE="Release" -DCMAKE_PREFIX_PATH:PATH=C:/Qt/6.8.2/msvc2022_64 -G "Visual Studio 17 2022"`
   1. NOTE: Make sure to replace the DCMAKE_PREFIX_PATH with the path to your QT install dir
5. Run `cmake --build "./build" --config Release`
6. You should be able to start it by typing `.\build\Release\tail-tray.exe` 