/*
 * ==============================================================================
 *                         SISTEM PARKIR KAMPUS
 * ==============================================================================
 * Deskripsi:
 *   Program parkir kampus sederhana yang dirancang khusus agar sangat mudah
 *   dipahami oleh pemula:
 *   - Tanpa database eksternal & tanpa struct (hanya array dan variabel dasar).
 *   - Posisi fungsi main() berada di atas agar alur program langsung terlihat.
 *   - Data slot dibedakan murni dari nomor indeks array (Slot 1-4 Motor, 5-12 Mobil).
 *   - Menghitung biaya langsung dari durasi jam parkir (tanpa library time.h yang rumit).
 *
 * Cara Kompilasi & Menjalankan:
 *   clang -std=c99 -Wall -Wextra parkir.c -o parkir
 *   ./parkir
 * ==============================================================================
 */

#include <stdio.h>
#include <string.h>

/* ==============================================================================
 * 1. KONSTANTA PROGRAM
 * ==============================================================================
 */
#define SLOT_MOTOR      4                          /* Slot 1 sampai 4 untuk motor */
#define SLOT_MOBIL      8                          /* Slot 5 sampai 12 untuk mobil */
#define TOTAL_SLOT      (SLOT_MOTOR + SLOT_MOBIL)  /* Total ada 12 slot */

#define TARIF_MOTOR     2000                       /* Rp 2.000 per jam */
#define TARIF_MOBIL     5000                       /* Rp 5.000 per jam */


/* ==============================================================================
 * 2. VARIABEL GLOBAL (PENYIMPANAN DATA DENGAN ARRAY)
 * ==============================================================================
 * Pembeda slot parkir berdasarkan indeks array (0 sampai 11):
 * - Indeks 0 sampai 3  : Slot Motor (Slot 1 s/d 4)
 * - Indeks 4 sampai 11 : Slot Mobil (Slot 5 s/d 12)
 */

/* Data Kendaraan yang Sedang Parkir */
char platParkir[TOTAL_SLOT][16];    /* Menyimpan plat nomor. Jika kosong bernilai "" */
int  jenisParkir[TOTAL_SLOT];       /* 1 = Motor, 2 = Mobil */

/* Data Riwayat Transaksi (Kendaraan yang sudah keluar) */
char platRiwayat[50][16];           /* Catatan plat */
int  jenisRiwayat[50];              /* 1 = Motor, 2 = Mobil */
int  durasiRiwayat[50];             /* Lama parkir dalam jam */
int  bayarRiwayat[50];              /* Total rupiah yang dibayar */
int  jumlahRiwayat = 0;             /* Menghitung banyaknya transaksi */

/* Total Uang Pemasukan */
int  totalPendapatan = 0;


/* ==============================================================================
 * 3. DEKLARASI FUNGSI (PROTOTIPE)
 * ==============================================================================
 * Ditulis di atas agar compiler mengenali fungsi sebelum dipanggil di main().
 */
void tampilkanMenu(void);
void kendaraanMasuk(void);
void kendaraanKeluar(void);
void cekSlotParkir(void);
void lihatRiwayat(void);


/* ==============================================================================
 * 4. FUNGSI UTAMA (MAIN) - BERADA DI POSISI ATAS
 * ==============================================================================
 */
int main(void) {
    int pilihan;

    do {
        tampilkanMenu();
        printf("  Pilih Menu [0-4]: ");

        /* Mencegah error jika pengguna salah mengetik huruf */
        if (scanf("%d", &pilihan) != 1) {
            printf("\n  [Error] Masukkan angka 0 sampai 4!\n");
            while (getchar() != '\n'); /* Bersihkan sisa ketikan di buffer */
            continue;
        }

        switch (pilihan) {
            case 1:
                kendaraanMasuk();
                break;
            case 2:
                kendaraanKeluar();
                break;
            case 3:
                cekSlotParkir();
                break;
            case 4:
                lihatRiwayat();
                break;
            case 0:
                printf("\n  Terima kasih telah menggunakan Sistem Parkir Kampus. Selesai!\n\n");
                break;
            default:
                printf("\n  [Peringatan] Pilihan tidak valid! Silakan pilih 0-4.\n");
                break;
        }

        if (pilihan != 0) {
            printf("\n"); /* Spasi pemisah antar menu */
        }

    } while (pilihan != 0);

    return 0;
}


/* ==============================================================================
 * 5. IMPLEMENTASI FITUR-FITUR
 * ==============================================================================
 */

/*
 * tampilkanMenu()
 * Menampilkan ringkasan ketersediaan slot parkir dan opsi menu.
 */
void tampilkanMenu(void) {
    /* 1. Hitung berapa motor yang sedang parkir (indeks 0 sampai 3) */
    int motorTerisi = 0;
    for (int i = 0; i < SLOT_MOTOR; i++) {
        if (platParkir[i][0] != '\0') {
            motorTerisi++;
        }
    }

    /* 2. Hitung berapa mobil yang sedang parkir (indeks 4 sampai 11) */
    int mobilTerisi = 0;
    for (int i = SLOT_MOTOR; i < TOTAL_SLOT; i++) {
        if (platParkir[i][0] != '\0') {
            mobilTerisi++;
        }
    }

    /* 3. Tampilkan informasi ke layar */
    printf("======================================================\n");
    printf("                SISTEM PARKIR KAMPUS                  \n");
    printf("======================================================\n");
    printf(" Slot Terisi: Motor [%d/%d]  |  Mobil [%d/%d]\n",
           motorTerisi, SLOT_MOTOR, mobilTerisi, SLOT_MOBIL);
    printf(" Total Pendapatan: Rp%d\n", totalPendapatan);
    printf("------------------------------------------------------\n");
    printf(" 1. Kendaraan Masuk\n");
    printf(" 2. Kendaraan Keluar & Bayar\n");
    printf(" 3. Cek Status Slot Parkir\n");
    printf(" 4. Lihat Riwayat Transaksi\n");
    printf(" 0. Keluar\n");
    printf("======================================================\n");
}

/*
 * kendaraanMasuk()
 * Mencatat kendaraan baru yang masuk ke slot parkir.
 */
void kendaraanMasuk(void) {
    char plat[16];
    int  jenis;

    printf("\n--- PENCATATAN KENDARAAN MASUK ---\n");

    /* 1. Input plat nomor */
    printf("  Masukkan Plat Nomor: ");
    scanf("%15s", plat);

    /* 2. Cek apakah kendaraan dengan plat ini sudah ada di dalam */
    for (int i = 0; i < TOTAL_SLOT; i++) {
        if (platParkir[i][0] != '\0' && strcmp(platParkir[i], plat) == 0) {
            printf("  [Gagal] Kendaraan %s sudah ada di dalam area parkir!\n", plat);
            return;
        }
    }

    /* 3. Input jenis kendaraan */
    printf("  Jenis Kendaraan (1. Motor, 2. Mobil): ");
    if (scanf("%d", &jenis) != 1 || (jenis != 1 && jenis != 2)) {
        printf("  [Gagal] Pilihan jenis kendaraan tidak valid!\n");
        return;
    }

    /* 4. Tentukan batas slot yang dicari berdasarkan jenis kendaraan */
    int awalSlot, akhirSlot;
    if (jenis == 1) {
        awalSlot  = 0;          /* Motor: slot indeks 0 s/d 3 */
        akhirSlot = SLOT_MOTOR;
    } else {
        awalSlot  = SLOT_MOTOR; /* Mobil: slot indeks 4 s/d 11 */
        akhirSlot = TOTAL_SLOT;
    }

    /* 5. Cari slot yang masih kosong */
    int slotKosong = -1;
    for (int i = awalSlot; i < akhirSlot; i++) {
        if (platParkir[i][0] == '\0') {
            slotKosong = i;
            break;
        }
    }

    /* 6. Jika tidak ada slot kosong */
    if (slotKosong == -1) {
        if (jenis == 1) {
            printf("  [Gagal] Parkir Motor sudah penuh!\n");
        } else {
            printf("  [Gagal] Parkir Mobil sudah penuh!\n");
        }
        return;
    }

    /* 7. Simpan data ke array pada slot yang ditemukan */
    strcpy(platParkir[slotKosong], plat);
    jenisParkir[slotKosong] = jenis;

    printf("  [Sukses] %s (%s) berhasil diparkir di Slot #%d\n",
           plat, (jenis == 1 ? "Motor" : "Mobil"), slotKosong + 1);
}

/*
 * kendaraanKeluar()
 * Mencatat kendaraan keluar, menghitung biaya parkir, dan mencetak struk.
 */
void kendaraanKeluar(void) {
    char plat[16];
    int  jamParkir;

    printf("\n--- PENCATATAN KENDARAAN KELUAR ---\n");

    /* 1. Input plat nomor yang ingin keluar */
    printf("  Masukkan Plat Nomor: ");
    scanf("%15s", plat);

    /* 2. Cari di slot mana kendaraan tersebut berada */
    int slotKetemu = -1;
    for (int i = 0; i < TOTAL_SLOT; i++) {
        if (platParkir[i][0] != '\0' && strcmp(platParkir[i], plat) == 0) {
            slotKetemu = i;
            break;
        }
    }

    /* 3. Jika tidak ditemukan */
    if (slotKetemu == -1) {
        printf("  [Gagal] Kendaraan %s tidak ditemukan di area parkir!\n", plat);
        return;
    }

    /* 4. Masukkan lama parkir (dalam jam) */
    printf("  Lama Parkir (berapa jam): ");
    if (scanf("%d", &jamParkir) != 1 || jamParkir < 1) {
        jamParkir = 1; /* Minimal parkir dihitung 1 jam */
    }

    /* 5. Hitung tarif berdasarkan jenis kendaraan */
    int jenis = jenisParkir[slotKetemu];
    int totalBayar;
    if (jenis == 1) {
        totalBayar = jamParkir * TARIF_MOTOR;
    } else {
        totalBayar = jamParkir * TARIF_MOBIL;
    }

    /* 6. Cetak struk pembayaran */
    printf("\n  ================ STRUK PARKIR ================\n");
    printf("  Plat Nomor   : %s\n", plat);
    printf("  Jenis        : %s\n", (jenis == 1 ? "Motor" : "Mobil"));
    printf("  Lokasi       : Slot #%d\n", slotKetemu + 1);
    printf("  Lama Parkir  : %d Jam\n", jamParkir);
    printf("  Total Bayar  : Rp%d\n", totalBayar);
    printf("  ==============================================\n");

    /* 7. Simpan ke riwayat transaksi jika kapasitas masih ada */
    if (jumlahRiwayat < 50) {
        strcpy(platRiwayat[jumlahRiwayat], plat);
        jenisRiwayat[jumlahRiwayat]  = jenis;
        durasiRiwayat[jumlahRiwayat] = jamParkir;
        bayarRiwayat[jumlahRiwayat]  = totalBayar;
        jumlahRiwayat++;
    }

    /* 8. Tambahkan uang ke total pendapatan */
    totalPendapatan += totalBayar;

    /* 9. Kosongkan slot parkir (kembalikan string jadi "") */
    platParkir[slotKetemu][0] = '\0';
    printf("  [Sukses] Slot #%d sekarang sudah kembali kosong.\n", slotKetemu + 1);
}

/*
 * cekSlotParkir()
 * Menampilkan status setiap slot parkir apakah terisi atau kosong.
 */
void cekSlotParkir(void) {
    printf("\n=================== STATUS SLOT PARKIR ===================\n");

    /* Area Motor: Slot 1 sampai 4 */
    printf(" [ Area Motor: Slot 1 - %d ]\n", SLOT_MOTOR);
    for (int i = 0; i < SLOT_MOTOR; i++) {
        if (platParkir[i][0] == '\0') {
            printf("   Slot %2d : [ KOSONG ]\n", i + 1);
        } else {
            printf("   Slot %2d : [ TERISI ] -> Plat: %s\n", i + 1, platParkir[i]);
        }
    }

    /* Area Mobil: Slot 5 sampai 12 */
    printf("\n [ Area Mobil: Slot %d - %d ]\n", SLOT_MOTOR + 1, TOTAL_SLOT);
    for (int i = SLOT_MOTOR; i < TOTAL_SLOT; i++) {
        if (platParkir[i][0] == '\0') {
            printf("   Slot %2d : [ KOSONG ]\n", i + 1);
        } else {
            printf("   Slot %2d : [ TERISI ] -> Plat: %s\n", i + 1, platParkir[i]);
        }
    }
    printf("==========================================================\n");
}

/*
 * lihatRiwayat()
 * Menampilkan catatan transaksi kendaraan yang sudah keluar.
 */
void lihatRiwayat(void) {
    printf("\n=================== RIWAYAT TRANSAKSI ===================\n");

    if (jumlahRiwayat == 0) {
        printf("  Belum ada transaksi kendaraan keluar.\n");
        printf("=========================================================\n");
        return;
    }

    for (int i = 0; i < jumlahRiwayat; i++) {
        printf("  Transaksi #%d\n", i + 1);
        printf("    Plat Nomor  : %s (%s)\n",
               platRiwayat[i], (jenisRiwayat[i] == 1 ? "Motor" : "Mobil"));
        printf("    Lama Parkir : %d Jam\n", durasiRiwayat[i]);
        printf("    Total Bayar : Rp%d\n", bayarRiwayat[i]);
        printf("  -------------------------------------------------------\n");
    }
}
