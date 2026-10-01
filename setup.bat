@echo off
REM ======================================================================
REM  setup.bat  -  "npm install" untuk proyek C di Windows
REM ======================================================================
REM
REM  Jalankan:   setup.bat            compile lalu jalan
REM              setup.bat --build    hanya compile
REM              setup.bat --clean    hapus hasil compile, compile ulang
REM ======================================================================

setlocal

set JALANKAN=1
set BERSIHKAN=0
if "%~1"=="--build" set JALANKAN=0
if "%~1"=="--clean" set BERSIHKAN=1

echo.
echo [1/4] Mengecek compiler C
where gcc >nul 2>&1
if errorlevel 1 (
    echo   !!!! gcc belum ada
    echo.
    echo   Pasang MinGW-w64 dengan mengetik:
    echo     winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
    echo.
    echo   Lalu buka JENDELA TERMINAL BARU dan jalankan setup.bat lagi.
    echo.
    pause
    exit /b 1
)
echo   OK   gcc sudah ada

echo.
echo [2/4] Mengecek library SQLite
echo int main(void){return 0;} > "%TEMP%\cek_sqlite.c"
gcc "%TEMP%\cek_sqlite.c" -o "%TEMP%\cek_sqlite.exe" -lsqlite3 >nul 2>&1
if errorlevel 1 (
    echo   !!!! sqlite3 belum ada, sekarang dipasang...
    pacman -S --noconfirm mingw-w64-x86_64-sqlite3 >nul 2>&1
    del "%TEMP%\cek_sqlite.c" >nul 2>&1
    echo   GAGAL sqlite3 tidak bisa dipasang otomatis.
    echo   Pasang MinGW-w64 dulu lalu ulangi langkah di atas.
    exit /b 1
)
echo   OK   sqlite3 sudah ada
del "%TEMP%\cek_sqlite.c" >nul 2>&1
del "%TEMP%\cek_sqlite.exe" >nul 2>&1

echo.
echo [3/4] Compile
if "%BERSIHKAN%"=="1" (
    if exist parkir.exe del parkir.exe
    if exist parkir.db del parkir.db
)
gcc -std=c99 -Wall -Wextra -g parkir.c -o parkir.exe -lsqlite3
if errorlevel 1 (
    echo   GAGAL compile gagal, baca pesan error di atas
    exit /b 1
)
echo   OK   compile selesai

if "%JALANKAN%"=="0" (
    echo.
    echo Selesai. Jalankan exe untuk mulai.
    exit /b 0
)

echo.
echo [4/4] Menjalankan program
echo   (untuk keluar, pilih menu 0 atau tekan Ctrl-Z lalu Enter)
echo.
parkir.exe

endlocal
