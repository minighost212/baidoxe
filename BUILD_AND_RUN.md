# Build & Chạy AEON Parking Simulation

Toolchain dùng: **MSYS2 MinGW64 (GCC 16.2.0)** tại `C:\msys64\mingw64\bin`.

> ⚠️ **Lưu ý quan trọng:** GCC 16.2.0 ở chế độ Release mặc định (`-O2`) bị lỗi codegen làm crash khi parse file OBJ.
> Phải build với **`-O1`** (hoặc `-O0`/Debug) thì ứng dụng mới chạy được.

---

## 1. Cấu hình (chỉ cần chạy 1 lần)

```powershell
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH

cmake -S "d:\Dohoa\DHMT2\DHMT2\DHMT" `
      -B "d:\Dohoa\DHMT2\DHMT2\DHMT\build-mingw-o1" `
      -G "MinGW Makefiles" `
      -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_C_COMPILER="C:/msys64/mingw64/bin/gcc.exe" `
      -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe" `
      -DCMAKE_C_FLAGS_RELEASE="-O1 -g -DNDEBUG" `
      -DCMAKE_CXX_FLAGS_RELEASE="-O1 -g -DNDEBUG"
```

## 2. Build ứng dụng

```powershell
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH
cmake --build "d:\Dohoa\DHMT2\DHMT2\DHMT\build-mingw-o1" --config Release
```

## 3. Copy exe ra thư mục gốc + copy DLL (lần đầu)

```powershell
Copy-Item "d:\Dohoa\DHMT2\DHMT2\DHMT\build-mingw-o1\aeon_parking.exe" `
          "d:\Dohoa\DHMT2\DHMT2\DHMT\aeon_parking_mingw.exe" -Force

# Copy các DLL cần thiết (chỉ cần làm 1 lần)
Copy-Item "C:\msys64\mingw64\bin\glew32.dll" "d:\Dohoa\DHMT2\DHMT2\DHMT\" -Force
Copy-Item "C:\msys64\mingw64\bin\glfw3.dll" "d:\Dohoa\DHMT2\DHMT2\DHMT\" -Force
Copy-Item "C:\msys64\mingw64\bin\libgcc_s_seh-1.dll" "d:\Dohoa\DHMT2\DHMT2\DHMT\" -Force
Copy-Item "C:\msys64\mingw64\bin\libstdc++-6.dll" "d:\Dohoa\DHMT2\DHMT2\DHMT\" -Force
Copy-Item "C:\msys64\mingw64\bin\libwinpthread-1.dll" "d:\Dohoa\DHMT2\DHMT2\DHMT\" -Force
```

## 4. Chạy ứng dụng

```powershell
cd "d:\Dohoa\DHMT2\DHMT2\DHMT"
.\aeon_parking_mingw.exe
```

> Chạy bằng PowerShell/CMD **bên ngoài TRAE** để tránh bị TRAE Sandbox chặn truy cập driver NVIDIA.

---

## Build & chạy test (không cần OpenGL)

```powershell
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH
cmake --build "d:\Dohoa\DHMT2\DHMT2\DHMT\build-mingw-o1" --config Release
& "d:\Dohoa\DHMT2\DHMT2\DHMT\build-mingw-o1\parking_world_tests.exe"
```

---

## Bản Debug (-O0) — dùng khi cần debug

```powershell
# Cấu hình
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH
cmake -S "d:\Dohoa\DHMT2\DHMT2\DHMT" `
      -B "d:\Dohoa\DHMT2\DHMT2\DHMT\build-mingw-dbg" `
      -G "MinGW Makefiles" `
      -DCMAKE_BUILD_TYPE=Debug `
      -DCMAKE_C_COMPILER="C:/msys64/mingw64/bin/gcc.exe" `
      -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"

# Build
cmake --build "d:\Dohoa\DHMT2\DHMT2\DHMT\build-mingw-dbg" --config Debug

# Chạy
Copy-Item "d:\Dohoa\DHMT2\DHMT2\DHMT\build-mingw-dbg\aeon_parking.exe" `
          "d:\Dohoa\DHMT2\DHMT2\DHMT\aeon_parking_mingw.exe" -Force
cd "d:\Dohoa\DHMT2\DHMT2\DHMT"
.\aeon_parking_mingw.exe
```
