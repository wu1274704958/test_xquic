@echo off
set build_type=Release
set ANDROID_PLATFORM=21
set num=0
for %%a in (%*) do set /a num+=1
if %num% GEQ 1 set build_type=%1
if %num% GEQ 2 set ANDROID_PLATFORM=%2
cmake -S . -G "Unix Makefiles" -B build_arm64_android -DCMAKE_BUILD_TYPE=%build_type% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake -DVCPKG_TARGET_TRIPLET=arm64-android -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=%ANDROID_PLATFORM%
cmake -S . -G "Unix Makefiles" -B build_arm_android -DCMAKE_BUILD_TYPE=%build_type% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake -DVCPKG_TARGET_TRIPLET=arm-android -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=%ANDROID_PLATFORM%
:: liblsquic[core]:x86-android is only supported on '!x86'
:: cmake -S . -G "Unix Makefiles" -B build_x86_android -DCMAKE_BUILD_TYPE=%build_type% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake -DVCPKG_TARGET_TRIPLET=x86-android -DANDROID_ABI=x86 -DANDROID_PLATFORM=%ANDROID_PLATFORM%
cmake --build .\build_arm64_android
cmake --build .\build_arm_android
:: cmake --build .\build_x86_android