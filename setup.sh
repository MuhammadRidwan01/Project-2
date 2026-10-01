#!/usr/bin/env bash
#
# ======================================================================
#  setup.sh  -  "npm install" untuk proyek C
# ======================================================================
#
#  Fungsi file ini:
#    1. Mengecek operating system yang dipakai
#    2. Memasang compiler C kalau belum ada
#    3. Memasang library SQLite kalau belum ada
#    4. Menjalankan make
#    5. Menjalankan programnya
#
#  Cara pakai:
#       ./setup.sh          compile lalu langsung jalan
#       ./setup.sh --build  hanya compile
#       ./setup.sh --clean  hapus hasil compile, lalu compile ulang
#
#  Di Windows pakai setup.bat (perintah nyaris sama).
# ======================================================================

# --- pengaturan warna terminal -------------------------------------
hijau='\033[0;32m'
kuning='\033[0;33m'
merah='\033[0;31m'
biru='\033[0;34m'
putih='\033[0m'

ok()    { printf "${hijau}  OK${putih}   %s\n" "$1"; }
info()  { printf "${biru}  ..${putih}   %s\n" "$1"; }
warn()  { printf "${kuning}  !!!!${putih}  %s\n" "$1"; }
gagal() { printf "${merah}  GAGAL${putih} %s\n" "$1"; }
lompat() { printf "\n${biru}%s${putih}\n" "$1"; }

# --- argument -------------------------------------------------------
JALANKAN=1        # 1 = setelah compile, langsung jalankan program
BERSIHKAN=0      # 1 = hapus hasil compile yang lama dulu
for arg in "$@"; do
    case "$arg" in
        --build) JALANKAN=0 ;;
        --clean) BERSIHKAN=1 ;;
        *) ;;
    esac
done

lompat "1. Mengecek sistem operasi"

OS="$(uname -s)"
if   [ "$OS" = "Darwin" ]; then JENIS_OS="macos"
elif [ "$OS" = "Linux"  ]; then JENIS_OS="linux"
else
    gagal "Sistem '$OS' tidak dikenali. Pakai setup.bat untuk Windows."
    exit 1
fi
ok "$OS"

# --- 2. Memasang compiler C ----------------------------------------
lompat "2. Mengecek compiler C"

CEK_CC=0
if command -v cc >/dev/null 2>&1; then
    ok "cc sudah ada ($(cc --version 2>/dev/null | head -1 | cut -c1-40))"
    CEK_CC=1
elif command -v gcc >/dev/null 2>&1; then
    ok "gcc sudah ada"
    CEK_CC=1
elif command -v clang >/dev/null 2>&1; then
    ok "clang sudah ada"
    CEK_CC=1
fi

if [ "$CEK_CC" -eq 0 ]; then
    warn "compiler C belum ada, sekarang dipasang..."
    if   [ "$JENIS_OS" = "macos" ]; then
        info "membuka Xcode Command Line Tools, tunggu sampai selesai"
        xcode-select --install 2>/dev/null
        printf "    Selesai? buka lagi terminal lalu jalankan ./setup.sh\n"
        exit 1
    else
        if command -v apt-get >/dev/null 2>&1; then
            sudo apt-get update && sudo apt-get install -y build-essential
        elif command -v dnf >/dev/null 2>&1; then
            sudo dnf install -y gcc
        elif command -v pacman >/dev/null 2>&1; then
            sudo pacman -S --noconfirm gcc
        else
            gagal "Package manager tidak dikenali. Pasang gcc manual."
            exit 1
        fi
    fi
fi

# --- 3. Memasang library SQLite ------------------------------------
lompat "3. Mengecek library SQLite"

# Cara ceknya: coba compile program kecil yang pakai sqlite3.
CEK_SQLITE=0
cat > /tmp/cek_sqlite_parkir.c <<'EOF'
#include <sqlite3.h>
int main(void) { return sqlite3_libversion_number() > 0 ? 0 : 1; }
EOF

if cc /tmp/cek_sqlite_parkir.c -o /tmp/cek_sqlite_parkir -lsqlite3 2>/dev/null; then
    ok "sqlite3 sudah ada"
    CEK_SQLITE=1
else
    warn "sqlite3 belum ada, sekarang dipasang..."
    if [ "$JENIS_OS" = "macos" ]; then
        ok "tidak perlu dipasang, macOS sudah punya sqlite3 bawaan"
        CEK_SQLITE=1
    else
        if command -v apt-get >/dev/null 2>&1; then
            sudo apt-get install -y libsqlite3-dev
        elif command -v dnf >/dev/null 2>&1; then
            sudo dnf install -y sqlite-devel
        elif command -v pacman >/dev/null 2>&1; then
            sudo pacman -S --noconfirm sqlite
        elif command -v zypper >/dev/null 2>&1; then
            sudo zypper install -y sqlite3-devel
        else
            gagal "Package manager tidak dikenali. Pasang libsqlite3-dev manual."
            exit 1
        fi
        CEK_SQLITE=1
    fi
fi
rm -f /tmp/cek_sqlite_parkir.c /tmp/cek_sqlite_parkir

# --- 4. Compile -----------------------------------------------------
lompat "4. Compile"

if [ "$BERSIHKAN" -eq 1 ]; then
    info "membersihkan hasil compile yang lama"
    make clean
fi

if make; then
    ok "compile selesai -> ./parkir"
else
    gagal "compile gagal. Baca pesan error di atas."
    exit 1
fi

if [ "$JALANKAN" -eq 0 ]; then
    lompat "Selesai. Jalankan ./parkir untuk mulai."
    exit 0
fi

# --- 5. Jalankan ----------------------------------------------------
lompat "5. Menjalankan program"
lompat "   (untuk keluar, pilih menu 0 atau tekan Ctrl-D)"
echo
./parkir
