/*
 * ==============================================================================
 *                         SISTEM PARKIR KAMPUS
 * ==============================================================================
 * Deskripsi:
 *   Program simulasi manajemen parkir motor dan mobil sederhana menggunakan
 *   bahasa C tanpa database eksternal (semua data disimpan di memori/variabel).
 *
 * Cara Kompilasi & Menjalankan:
 *   1. Kompilasi: clang -std=c99 -Wall -Wextra parkir.c -o parkir
 *                 (atau gcc -std=c99 -Wall -Wextra parkir.c -o parkir)
 *   2. Jalankan  : ./parkir
 * ==============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ==============================================================================
 * 1. KONSTANTA PROGRAM
 * ==============================================================================
 * Menggunakan #define agar angka-angka penting mudah diubah di satu tempat
 * dan kode lebih mudah dibaca (menghindari "magic numbers").
 */

/* Kapasitas Slot Parkir */
#define SLOT_MOTOR          4                           /* Slot 1 s/d 4 untuk motor */
#define SLOT_MOBIL          8                           /* Slot 5 s/d 12 untuk mobil */
#define TOTAL_SLOT          (SLOT_MOTOR + SLOT_MOBIL)   /* Total 12 slot */

/* Kode Jenis Kendaraan */
#define KENDARAAN_MOTOR     0
#define KENDARAAN_MOBIL     1

/* Tarif Parkir (dalam Rupiah) */
#define TARIF_MOTOR_PER_JAM     2000
#define TARIF_MOBIL_PER_JAM     5000
#define MAKS_MOTOR_PER_HARI     20000
#define MAKS_MOBIL_PER_HARI     50000

/* Kapasitas Maksimal Riwayat Transaksi */
#define KAPASITAS_RIWAYAT   50

/* Panjang Maksimal Teks */
#define PANJANG_PLAT        16   /* 15 karakter plat + 1 karakter penutup '\0' */
#define PANJANG_WAKTU       24   /* Penampung format tanggal & jam */


/* ==============================================================================
 * 2. VARIABEL GLOBAL (PENYIMPANAN DATA / DATABASE SEMENTARA)
 * ==============================================================================
 * Data disimpan dalam array paralel. Nomor indeks array mewakili nomor slot.
 * Indeks 0 sampai (SLOT_MOTOR - 1)       -> Slot parkir motor
 * Indeks SLOT_MOTOR sampai (TOTAL_SLOT - 1) -> Slot parkir mobil
 */

/* Data Kendaraan yang Sedang Terparkir */
char platParkir[TOTAL_SLOT][PANJANG_PLAT];   /* Plat nomor; jika kosong bernilai "" */
int  jenisParkir[TOTAL_SLOT];                /* 0 = Motor, 1 = Mobil */
long menitMasukParkir[TOTAL_SLOT];           /* Waktu masuk (dalam satuan menit) */

/* Data Riwayat Transaksi yang Sudah Keluar */
char platRiwayat[KAPASITAS_RIWAYAT][PANJANG_PLAT];
int  jenisRiwayat[KAPASITAS_RIWAYAT];
int  durasiRiwayat[KAPASITAS_RIWAYAT];       /* Durasi parkir (dalam menit) */
int  bayarRiwayat[KAPASITAS_RIWAYAT];        /* Total biaya yang dibayar */
int  jumlahRiwayat = 0;                      /* Menghitung berapa transaksi tersimpan */

/* Data Keuangan */
long totalPendapatan = 0;                    /* Akumulasi uang dari kendaraan keluar */

/* Variabel Pembantu Simulasi Waktu */
long offsetMenit = 0;                        /* Menit tambahan untuk mempercepat waktu */


/* ==============================================================================
 * 3. FUNGSI WAKTU & SIMULASI
 * ==============================================================================
 */

/*
 * menitSekarang()
 * Menghitung waktu saat ini dalam satuan menit sejak 1 Januari 1970 (Unix Epoch).
 * Ditambah variabel 'offsetMenit' sehingga kita bisa memajukan waktu simulasi.
 */
long menitSekarang(void) {
    long detik = (long)time(NULL);
    long menit = detik / 60;
    return menit + offsetMenit;
}

/*
 * formatWaktu()
 * Mengubah angka menit menjadi teks tanggal dan jam yang mudah dibaca.
 * Contoh output: "08/10 14:30:00"
 */
void formatWaktu(long menit, char *outputTeks) {
    time_t detik = (time_t)menit * 60;
    strftime(outputTeks, PANJANG_WAKTU, "%d/%m %H:%M:%S", localtime(&detik));
}


/* ==============================================================================
 * 4. FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ==============================================================================
 */

/*
 * dapatkanNamaJenis()
 * Mengubah kode angka kendaraan (0/1) menjadi teks ("Motor"/"Mobil").
 */
void dapatkanNamaJenis(int jenis, char *outputTeks) {
    if (jenis == KENDARAAN_MOBIL) {
        strcpy(outputTeks, "Mobil");
    } else {
        strcpy(outputTeks, "Motor");
    }
}

/*
 * bacaPilihanJenis()
 * Menanyakan jenis kendaraan ke pengguna lalu mengembalikan kode kendaraan.
 * Return: 0 (Motor), 1 (Mobil), atau -1 jika input tidak valid.
 */
int bacaPilihanJenis(void) {
    int pilihan;
    printf("  Jenis Kendaraan (1. Motor, 2. Mobil): ");
    if (scanf("%d", &pilihan) != 1) {
        return -1;
    }

    if (pilihan == 1) return KENDARAAN_MOTOR;
    if (pilihan == 2) return KENDARAAN_MOBIL;

    return -1; /* Pilihan angka selain 1 atau 2 */
}

/*
 * jedaAntarMenu()
 * Memberi jeda spasi baris agar tampilan layar lebih rapi dan nyaman dibaca.
 */
void jedaAntarMenu(void) {
    printf("\n");
}


/* ==============================================================================
 * 5. LOGIKA PERHITUNGAN TARIF
 * ==============================================================================
 */

/*
 * hitungTarif()
 * Menghitung total biaya parkir berdasarkan jenis kendaraan dan lama parkir (menit).
 *
 * Aturan Tarif:
 * 1. Dihitung per jam. Kelebihan menit dibulatkan ke atas menjadi 1 jam penuh.
 *    (Contoh: parkir 65 menit dihitung 2 jam).
 * 2. Minimal durasi bayar adalah 1 jam.
 * 3. Ada batas biaya maksimal per hari (24 jam) agar biaya tidak membengkak.
 */
int hitungTarif(int jenis, int durasiMenit) {
    int tarifPerJam;
    int batasMaksimalHarian;

    /* Tentukan tarif berdasarkan jenis kendaraan */
    if (jenis == KENDARAAN_MOBIL) {
        tarifPerJam         = TARIF_MOBIL_PER_JAM;
        batasMaksimalHarian = MAKS_MOBIL_PER_HARI;
    } else {
        tarifPerJam         = TARIF_MOTOR_PER_JAM;
        batasMaksimalHarian = MAKS_MOTOR_PER_HARI;
    }

    /* Pembulatan menit ke jam (misal 61 menit -> 2 jam) */
    int totalJam = durasiMenit / 60;
    if (durasiMenit % 60 > 0) {
        totalJam = totalJam + 1;
    }

    /* Minimal dihitung 1 jam */
    if (totalJam < 1) {
        totalJam = 1;
    }

    /* Hitung jumlah hari penuh dan sisa jam */
    int jumlahHari = totalJam / 24;
    int sisaJam    = totalJam % 24;

    /* Biaya sisa jam tidak boleh melebihi batas tarif 1 hari penuh */
    int biayaHari = jumlahHari * batasMaksimalHarian;
    int biayaSisa = sisaJam * tarifPerJam;
    if (biayaSisa > batasMaksimalHarian) {
        biayaSisa = batasMaksimalHarian;
    }

    return biayaHari + biayaSisa;
}


/* ==============================================================================
 * 6. PENGELOLAAN DATA SLOT & TRANSAKSI
 * ==============================================================================
 */

/*
 * apakahSlotKosong()
 * Mengecek apakah suatu slot kosong.
 * Suatu slot kosong jika karakter pertamanya adalah '\0' (string kosong).
 * Return: 1 jika kosong, 0 jika terisi.
 */
int apakahSlotKosong(int nomorSlot) {
    if (platParkir[nomorSlot][0] == '\0') {
        return 1;
    }
    return 0;
}

/*
 * hitungSlotTerpakai()
 * Menghitung berapa banyak slot yang saat ini terisi untuk jenis kendaraan tertentu.
 */
int hitungSlotTerpakai(int jenis) {
    int terpakai = 0;
    for (int s = 0; s < TOTAL_SLOT; s++) {
        if (apakahSlotKosong(s) == 0 && jenisParkir[s] == jenis) {
            terpakai++;
        }
    }
    return terpakai;
}

/*
 * cariSlotKosong()
 * Mencari indeks slot kosong pertama yang sesuai untuk jenis kendaraan.
 * Return: Indeks slot (0 s/d 11), atau -1 jika penuh.
 */
int cariSlotKosong(int jenis) {
    int slotAwal;
    int slotAkhir;

    if (jenis == KENDARAAN_MOBIL) {
        slotAwal  = SLOT_MOTOR; /* Mobil menempati slot indeks 4 s/d 11 */
        slotAkhir = TOTAL_SLOT;
    } else {
        slotAwal  = 0;          /* Motor menempati slot indeks 0 s/d 3 */
        slotAkhir = SLOT_MOTOR;
    }

    for (int s = slotAwal; s < slotAkhir; s++) {
        if (apakahSlotKosong(s) == 1) {
            return s; /* Ditemukan slot kosong */
        }
    }

    return -1; /* Area parkir penuh */
}

/*
 * cariSlotBerdasarkanPlat()
 * Mencari di slot mana kendaraan dengan plat tertentu sedang parkir.
 * Return: Indeks slot jika ditemukan, atau -1 jika tidak ada.
 */
int cariSlotBerdasarkanPlat(const char *platDicari) {
    for (int s = 0; s < TOTAL_SLOT; s++) {
        /* strcmp mengembalikan 0 jika kedua teks persis sama */
        if (strcmp(platParkir[s], platDicari) == 0) {
            return s;
        }
    }
    return -1;
}

/*
 * simpanKeRiwayat()
 * Mencatat transaksi parkir yang telah selesai ke dalam array riwayat.
 */
void simpanKeRiwayat(char *plat, int jenis, int durasi, int bayar) {
    if (jumlahRiwayat >= KAPASITAS_RIWAYAT) {
        printf("  [Peringatan] Kapasitas riwayat transaksi sudah penuh!\n");
        return;
    }

    strcpy(platRiwayat[jumlahRiwayat], plat);
    jenisRiwayat[jumlahRiwayat]  = jenis;
    durasiRiwayat[jumlahRiwayat] = durasi;
    bayarRiwayat[jumlahRiwayat]  = bayar;

    jumlahRiwayat++;
}


/* ==============================================================================
 * 7. FITUR MENU SISTEM
 * ==============================================================================
 */

/*
 * tampilkanMenuUtama()
 * Menampilkan ringkasan status parkir dan daftar opsi menu.
 */
void tampilkanMenuUtama(void) {
    char waktuTeks[PANJANG_WAKTU];
    formatWaktu(menitSekarang(), waktuTeks);

    int motorTerpakai = hitungSlotTerpakai(KENDARAAN_MOTOR);
    int mobilTerpakai = hitungSlotTerpakai(KENDARAAN_MOBIL);

    printf("\n======================================================\n");
    printf("                SISTEM PARKIR KAMPUS                  \n");
    printf("======================================================\n");
    printf(" Waktu Sekarang   : %s\n", waktuTeks);
    printf(" Ketersediaan Slot: Motor [%d/%d]  |  Mobil [%d/%d]\n",
           motorTerpakai, SLOT_MOTOR, mobilTerpakai, SLOT_MOBIL);
    printf(" Total Pendapatan : Rp%ld\n", totalPendapatan);
    printf("------------------------------------------------------\n");
    printf(" 1. Kendaraan Masuk\n");
    printf(" 2. Kendaraan Keluar & Pembayaran\n");
    printf(" 3. Peta / Denah Slot Parkir\n");
    printf(" 4. Riwayat Transaksi\n");
    printf(" 5. Simulasi Waktu (Majukan Menit)\n");
    printf(" 0. Keluar dari Program\n");
    printf("======================================================\n");
}

/*
 * menuKendaraanMasuk()
 * Menangani proses saat kendaraan baru masuk ke area parkir.
 */
void menuKendaraanMasuk(void) {
    char plat[PANJANG_PLAT];
    char waktuTeks[PANJANG_WAKTU];
    char namaKendaraan[10];

    printf("\n--- PENCATATAN KENDARAAN MASUK ---\n");

    /* Langkah 1: Input plat nomor */
    printf("  Masukkan Plat Nomor: ");
    scanf("%15s", plat);

    /* Langkah 2: Cek apakah kendaraan ini sudah tercatat sedang parkir */
    if (cariSlotBerdasarkanPlat(plat) >= 0) {
        printf("  [Gagal] Kendaraan dengan plat %s sudah ada di dalam area parkir!\n", plat);
        return;
    }

    /* Langkah 3: Pilih jenis kendaraan */
    int jenis = bacaPilihanJenis();
    if (jenis < 0) {
        printf("  [Gagal] Pilihan jenis kendaraan tidak valid!\n");
        return;
    }
    dapatkanNamaJenis(jenis, namaKendaraan);

    /* Langkah 4: Cari slot yang masih kosong */
    int slot = cariSlotKosong(jenis);
    if (slot < 0) {
        printf("  [Gagal] Mohon maaf, slot parkir untuk %s sudah penuh!\n", namaKendaraan);
        return;
    }

    /* Langkah 5: Simpan data kendaraan ke dalam slot tersebut */
    strcpy(platParkir[slot], plat);
    jenisParkir[slot]      = jenis;
    menitMasukParkir[slot] = menitSekarang();

    /* Langkah 6: Tampilkan konfirmasi keberhasilan */
    formatWaktu(menitMasukParkir[slot], waktuTeks);
    printf("  [Sukses] %s (%s) berhasil diparkir di Slot #%d pada %s\n",
           plat, namaKendaraan, slot + 1, waktuTeks);
}

/*
 * menuKendaraanKeluar()
 * Menangani proses kendaraan keluar, menghitung durasi & tarif, serta mencetak struk.
 */
void menuKendaraanKeluar(void) {
    char plat[PANJANG_PLAT];
    char waktuMasukTeks[PANJANG_WAKTU];
    char waktuKeluarTeks[PANJANG_WAKTU];
    char namaKendaraan[10];

    printf("\n--- PENCATATAN KENDARAAN KELUAR ---\n");

    /* Langkah 1: Masukkan plat nomor yang akan keluar */
    printf("  Masukkan Plat Nomor: ");
    scanf("%15s", plat);

    /* Langkah 2: Cari kendaraan di dalam slot */
    int slot = cariSlotBerdasarkanPlat(plat);
    if (slot < 0) {
        printf("  [Gagal] Kendaraan dengan plat %s tidak ditemukan di area parkir!\n", plat);
        return;
    }

    /* Langkah 3: Hitung lama durasi parkir */
    long waktuKeluar = menitSekarang();
    long selisih     = waktuKeluar - menitMasukParkir[slot];
    if (selisih < 0) {
        selisih = 0;
    }
    int durasiMenit = (int)selisih;

    /* Langkah 4: Hitung biaya tarif parkir */
    int jenis = jenisParkir[slot];
    int biaya = hitungTarif(jenis, durasiMenit);
    dapatkanNamaJenis(jenis, namaKendaraan);

    /* Konversi durasi menit menjadi tampilan jam dan sisa menit */
    int totalJam  = durasiMenit / 60;
    int sisaMenit = durasiMenit % 60;

    formatWaktu(menitMasukParkir[slot], waktuMasukTeks);
    formatWaktu(waktuKeluar, waktuKeluarTeks);

    /* Langkah 5: Cetak struk pembayaran */
    printf("\n  ================ STRUK PARKIR ================\n");
    printf("  Plat Nomor   : %s\n", plat);
    printf("  Jenis        : %s\n", namaKendaraan);
    printf("  Lokasi Parkir: Slot #%d\n", slot + 1);
    printf("  Waktu Masuk  : %s\n", waktuMasukTeks);
    printf("  Waktu Keluar : %s\n", waktuKeluarTeks);
    printf("  Durasi       : %d Jam %d Menit (Total %d Menit)\n", totalJam, sisaMenit, durasiMenit);
    printf("  Total Biaya  : Rp%d\n", biaya);
    printf("  ==============================================\n");

    /* Langkah 6: Simpan ke riwayat transaksi dan tambahkan pendapatan */
    simpanKeRiwayat(plat, jenis, durasiMenit, biaya);
    totalPendapatan += biaya;

    /* Langkah 7: Kosongkan slot parkir (kembalikan string ke kosong) */
    platParkir[slot][0] = '\0';
    printf("  [Sukses] Slot #%d kini telah kembali kosong.\n", slot + 1);
}

/*
 * menuPetaSlot()
 * Menampilkan status ketersediaan setiap slot parkir secara visual.
 */
void menuPetaSlot(void) {
    printf("\n=================== DENAH SLOT PARKIR ===================\n");

    /* Bagian Slot Motor */
    printf(" [ Area Motor: Slot 1 - %d ]\n", SLOT_MOTOR);
    for (int s = 0; s < SLOT_MOTOR; s++) {
        if (apakahSlotKosong(s)) {
            printf("   Slot %2d : [ KOSONG ]\n", s + 1);
        } else {
            printf("   Slot %2d : [ TERISI ] -> Plat: %s\n", s + 1, platParkir[s]);
        }
    }

    /* Bagian Slot Mobil */
    printf("\n [ Area Mobil: Slot %d - %d ]\n", SLOT_MOTOR + 1, TOTAL_SLOT);
    for (int s = SLOT_MOTOR; s < TOTAL_SLOT; s++) {
        if (apakahSlotKosong(s)) {
            printf("   Slot %2d : [ KOSONG ]\n", s + 1);
        } else {
            printf("   Slot %2d : [ TERISI ] -> Plat: %s\n", s + 1, platParkir[s]);
        }
    }
    printf("=========================================================\n");
}

/*
 * menuRiwayatTransaksi()
 * Menampilkan daftar transaksi kendaraan yang sudah keluar dan membayar.
 */
void menuRiwayatTransaksi(void) {
    printf("\n=================== RIWAYAT TRANSAKSI ===================\n");

    if (jumlahRiwayat == 0) {
        printf("  Belum ada transaksi kendaraan keluar.\n");
        printf("=========================================================\n");
        return;
    }

    for (int i = 0; i < jumlahRiwayat; i++) {
        char namaKendaraan[10];
        dapatkanNamaJenis(jenisRiwayat[i], namaKendaraan);

        int jam  = durasiRiwayat[i] / 60;
        int sisa = durasiRiwayat[i] % 60;

        printf("  Transaksi #%d\n", i + 1);
        printf("    Plat Nomor : %s (%s)\n", platRiwayat[i], namaKendaraan);
        printf("    Durasi     : %d Jam %d Menit\n", jam, sisa);
        printf("    Biaya      : Rp%d\n", bayarRiwayat[i]);
        printf("  -------------------------------------------------------\n");
    }
}

/*
 * menuSimulasiWaktu()
 * Membantu simulasi pengetesan tarif tanpa perlu menunggu waktu nyata.
 */
void menuSimulasiWaktu(void) {
    char waktuTeks[PANJANG_WAKTU];

    printf("\n--- SIMULASI PENYESUAIAN WAKTU ---\n");
    if (offsetMenit == 0) {
        printf("  Status: Waktu berjalan normal sesuai jam komputer.\n");
    } else {
        printf("  Status: Waktu telah dimajukan +%ld menit dari waktu asli.\n", offsetMenit);
    }

    int menitTambahan;
    printf("  Tambahkan berapa menit? (Ketik 0 untuk reset ke waktu asli): ");
    if (scanf("%d", &menitTambahan) != 1) {
        printf("  [Gagal] Input tidak valid!\n");
        return;
    }

    if (menitTambahan == 0) {
        offsetMenit = 0;
        printf("  Waktu berhasil dikembalikan ke waktu komputer asli.\n");
    } else {
        offsetMenit += menitTambahan;
        printf("  Waktu simulasi berhasil dimajukan sebanyak %d menit.\n", menitTambahan);
    }

    formatWaktu(menitSekarang(), waktuTeks);
    printf("  Waktu sekarang setelah simulasi: %s\n", waktuTeks);
}


/* ==============================================================================
 * 8. FUNGSI UTAMA (MAIN)
 * ==============================================================================
 */

int main(void) {
    int pilihanMenu;

    do {
        tampilkanMenuUtama();
        printf("  Pilih Menu [0-5]: ");

        /* Validasi input menu */
        if (scanf("%d", &pilihanMenu) != 1) {
            printf("\n  [Error] Input harus berupa angka!\n");
            /* Bersihkan buffer input agar tidak terjadi infinite loop */
            while (getchar() != '\n');
            continue;
        }

        switch (pilihanMenu) {
            case 1:
                menuKendaraanMasuk();
                break;
            case 2:
                menuKendaraanKeluar();
                break;
            case 3:
                menuPetaSlot();
                break;
            case 4:
                menuRiwayatTransaksi();
                break;
            case 5:
                menuSimulasiWaktu();
                break;
            case 0:
                printf("\n  Terima kasih telah menggunakan Sistem Parkir Kampus. Sampai jumpa!\n\n");
                break;
            default:
                printf("\n  [Peringatan] Pilihan menu tidak valid. Silakan pilih 0 sampai 5.\n");
                break;
        }

        /* Beri jeda jarak jika belum keluar dari program */
        if (pilihanMenu != 0) {
            jedaAntarMenu();
        }

    } while (pilihanMenu != 0);

    return 0;
}
