/*
 * ======================================================================
 *  SISTEM PARKIR KAMPUS  (tanpa database - semua data pakai variabel)
 * ======================================================================
 *  Cara compile:
 *      clang -std=c99 -Wall -Wextra parkir.c -o parkir
 *      ./parkir
 * ======================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SLOT_MOTOR  4
#define SLOT_MOBIL  8
#define TOTAL_SLOT  (SLOT_MOTOR + SLOT_MOBIL)

enum { MOTOR = 0, MOBIL = 1 };

/* ================================================================
 *  VARIABEL GLOBAL (pengganti database)
 * ================================================================ */

/* Kendaraan yang sedang parkir. Indeks = nomor slot. */
char platParkir[TOTAL_SLOT][16];   /* plat di slot itu, "" = kosong */
int  jenisParkir[TOTAL_SLOT];      /* 0 = motor, 1 = mobil         */
long menitMasukParkir[TOTAL_SLOT]; /* kapan masuk (jumlah menit)    */

/* Riwayat transaksi. Indeks = nomor transaksi. */
char platRiwayat[50][16];
int  jenisRiwayat[50];
int  durasiRiwayat[50];           /* durasi dalam menit */
int  bayarRiwayat[50];
int  jumlahRiwayat = 0;

/* Total uang dari semua transaksi */
long totalPendapatan = 0;

/* Penambahan waktu untuk simulasi (menit) */
long offsetMenit = 0;

/* Waktu sekarang, dalam menit sejak 1 Januari 1970 */
long menitSekarang(void) {
    return (long)(time(NULL) / 60) + offsetMenit;
}

/* ================================================================
 *  1. HELPER
 * ================================================================ */

void input(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    if (fgets(buf, size, stdin)) {
        buf[strcspn(buf, "\n")] = 0;
    } else {
        buf[0] = 0;
    }
}

int inputAngka(const char *prompt) {
    char buf[32];
    input(prompt, buf, sizeof(buf));
    return atoi(buf);
}

/* Ubah jumlah menit menjadi teks tanggal/jam */
void tampilWaktu(long menit, char *out) {
    time_t detik = (time_t)menit * 60;
    strftime(out, 20, "%d/%m %H:%M:%S", localtime(&detik));
}

/* Tampilkan nama jenis kendaraan */
void namaJenis(int jenis, char *out) {
    if (jenis == MOBIL) strcpy(out, "Mobil");
    else               strcpy(out, "Motor");
}

void jeda(void) {
    printf("\nTekan Enter untuk melanjutkan...");
    char buf[8];
    fgets(buf, sizeof(buf), stdin);
}

/* ================================================================
 *  2. PERHITUNGAN TARIF
 * ================================================================ */

int hitungTarif(int jenis, int menit) {
    /* Tarif per jam dan batas maksimum per hari */
    int perJam, maksSehari;
    if (jenis == MOBIL) {
        perJam     = 5000;
        maksSehari = 50000;
    } else {
        perJam     = 2000;
        maksSehari = 20000;
    }

    /* Ubah menit menjadi jam, sisa menit dihitung jadi 1 jam penuh */
    int jam = menit / 60;
    if (menit % 60 > 0) jam = jam + 1;

    /* Minimal 1 jam */
    if (jam < 1) jam = 1;

    /* Pisahkan jumlah hari lengkap dan sisa jam */
    int hari      = jam / 24;
    int sisaJam   = jam % 24;

    /* Biaya = (hari x batas harian) + (sisa jam x tarif per jam) */
    int biayaHari  = hari * maksSehari;
    int biayaSisa  = sisaJam * perJam;

    /* Sisa jam tidak boleh melebihi batas satu hari */
    if (biayaSisa > maksSehari) biayaSisa = maksSehari;

    return biayaHari + biayaSisa;
}

/* ================================================================
 *  3. FUNGSI DATA (cukup array, tanpa query)
 * ================================================================ */

/* Slot kosong berarti plat di slot itu kosong string */
int slotKosong(int s) {
    if (platParkir[s][0] == 0) return 1;
    return 0;
}

/* Berapa slot yang sudah dipakai untuk jenis tertentu */
int jumlahTerpakai(int jenis) {
    int n = 0;
    for (int s = 0; s < TOTAL_SLOT; s++) {
        if (slotKosong(s) == 0 && jenisParkir[s] == jenis) {
            n = n + 1;
        }
    }
    return n;
}

/* Cari nomor slot kosong pertama untuk jenis tertentu.
   Kalau tidak ada, kembalikan -1. */
int cariSlotKosong(int jenis) {
    int mulai, akhir;

    if (jenis == MOBIL) {
        mulai = SLOT_MOTOR;         /* mobil mulai dari slot 5 */
        akhir = TOTAL_SLOT;
    } else {
        mulai = 0;                  /* motor mulai dari slot 1 */
        akhir = SLOT_MOTOR;
    }

    for (int s = mulai; s < akhir; s++) {
        if (slotKosong(s) == 1) return s;
    }

    return -1;
}

/* Cari nomor slot dari sebuah plat.
   Kalau plat tidak ada di parkir, kembalikan -1. */
int cariSlotPlat(const char *plat) {
    for (int s = 0; s < TOTAL_SLOT; s++) {
        if (strcmp(platParkir[s], plat) == 0) return s;
    }
    return -1;
}

/* Simpan satu transaksi ke riwayat.
   Kalau riwayat penuh, buang transaksi paling lama. */
void simpanTransaksi(char *plat, int jenis, int durasi, int bayar) {
    if (jumlahRiwayat == 50) {
        /* geser semua data satu posisi ke depan */
        for (int i = 1; i < 50; i++) {
            strcpy(platRiwayat[i - 1], platRiwayat[i]);
            jenisRiwayat[i - 1] = jenisRiwayat[i];
            durasiRiwayat[i - 1] = durasiRiwayat[i];
            bayarRiwayat[i - 1] = bayarRiwayat[i];
        }
        /* sekarang jumlahRiwayat = 49 */
        jumlahRiwayat = 49;
    }

    /* simpan di baris berikutnya */
    strcpy(platRiwayat[jumlahRiwayat], plat);
    jenisRiwayat[jumlahRiwayat] = jenis;
    durasiRiwayat[jumlahRiwayat] = durasi;
    bayarRiwayat[jumlahRiwayat] = bayar;
    jumlahRiwayat = jumlahRiwayat + 1;
}

/* ================================================================
 *  4. MENU DAN FITUR
 * ================================================================ */

void tampilMenu(void) {
    char waktu[24];
    tampilWaktu(menitSekarang(), waktu);

    printf("\n==============================================\n");
    printf("            SISTEM PARKIR KAMPUS\n");
    printf("==============================================\n");
    printf(" Jam %s | Motor %d/%d | Mobil %d/%d\n",
           waktu, jumlahTerpakai(MOTOR), SLOT_MOTOR,
           jumlahTerpakai(MOBIL), SLOT_MOBIL);
    printf(" Total Pendapatan: Rp%ld\n", totalPendapatan);
    printf("----------------------------------------------\n");
    printf(" 1. Kendaraan masuk\n");
    printf(" 2. Kendaraan keluar\n");
    printf(" 3. Peta slot\n");
    printf(" 4. Riwayat transaksi\n");
    printf(" 5. Majukan waktu simulasi\n");
    printf(" 0. Keluar\n");
    printf("==============================================\n");
}

void fiturMasuk(void) {
    char plat[16], waktu[24], jenisTeks[10];

    /* 1. baca plat */
    input("Plat nomor: ", plat, sizeof(plat));
    if (plat[0] == 0) {
        puts("  Plat tidak boleh kosong!");
        return;
    }

    /* 2. cek plat sudah ada atau belum */
    if (cariSlotPlat(plat) >= 0) {
        printf("  %s sudah ada di dalam area parkir!\n", plat);
        return;
    }

    /* 3. baca jenis kendaraan */
    int jenis = inputAngka("Jenis (1. Motor, 2. Mobil): ") - 1;
    if (jenis != MOTOR && jenis != MOBIL) {
        puts("  Jenis kendaraan tidak valid!");
        return;
    }
    namaJenis(jenis, jenisTeks);

    /* 4. cari slot kosong */
    int slot = cariSlotKosong(jenis);
    if (slot < 0) {
        printf("  Parkir %s penuh!\n", jenisTeks);
        return;
    }

    /* 5. isi data ke slot tersebut */
    strcpy(platParkir[slot], plat);
    jenisParkir[slot] = jenis;
    menitMasukParkir[slot] = menitSekarang();

    /* 6. kabari ke user */
    tampilWaktu(menitMasukParkir[slot], waktu);
    printf("  %s masuk slot %d pada %s.\n", plat, slot + 1, waktu);
}

void fiturKeluar(void) {
    char plat[16], wMasuk[24], wKeluar[24], jenisTeks[10];

    /* 1. baca plat */
    input("Plat nomor: ", plat, sizeof(plat));

    /* 2. cari slotnya */
    int slot = cariSlotPlat(plat);
    if (slot < 0) {
        puts("  Kendaraan tidak ditemukan!");
        return;
    }

    /* 3. hitung durasi (selisih menit keluar - menit masuk) */
    long keluar = menitSekarang();
    int durasi  = (int)(keluar - menitMasukParkir[slot]);
    if (durasi < 0) durasi = 0;

    /* 4. hitung tarif */
    int jenis = jenisParkir[slot];
    int bayar = hitungTarif(jenis, durasi);
    namaJenis(jenis, jenisTeks);

    /* 5. cetak struk */
    tampilWaktu(menitMasukParkir[slot], wMasuk);
    tampilWaktu(keluar, wKeluar);

    printf("\n  ---------- STRUK PARKIR ----------\n");
    printf("  Plat   : %s\n", plat);
    printf("  Jenis  : %s\n", jenisTeks);
    printf("  Masuk  : %s\n", wMasuk);
    printf("  Keluar : %s\n", wKeluar);
    printf("  Durasi : %d jam %d menit\n", durasi / 60, durasi % 60);
    printf("  Tarif  : Rp%d\n", bayar);
    printf("  ----------------------------------\n");

    /* 6. simpan ke riwayat dan tambahkan uang */
    simpanTransaksi(plat, jenis, durasi, bayar);
    totalPendapatan = totalPendapatan + bayar;

    /* 7. kosongkan slotnya */
    platParkir[slot][0] = 0;
}

void fiturPetaSlot(void) {
    puts("\n  Slot Motor:");
    for (int s = 0; s < SLOT_MOTOR; s++) {
        if (slotKosong(s) == 1) printf("  [%2d] kosong\n", s + 1);
        else                   printf("  [%2d] %s\n", s + 1, platParkir[s]);
    }

    puts("\n  Slot Mobil:");
    for (int s = SLOT_MOTOR; s < TOTAL_SLOT; s++) {
        if (slotKosong(s) == 1) printf("  [%2d] kosong\n", s + 1);
        else                   printf("  [%2d] %s\n", s + 1, platParkir[s]);
    }
}

void fiturRiwayat(void) {
    if (jumlahRiwayat == 0) {
        puts("  Belum ada transaksi.");
        return;
    }

    printf("\n  %-3s %-11s %-6s %-14s %s\n",
           "No", "Plat", "Jenis", "Durasi", "Bayar");

    for (int i = 0; i < jumlahRiwayat; i++) {
        char jenisTeks[10], durasiTeks[20];
        namaJenis(jenisRiwayat[i], jenisTeks);

        int jam = durasiRiwayat[i] / 60;
        int sisa = durasiRiwayat[i] % 60;
        sprintf(durasiTeks, "%d jam %d mnt", jam, sisa);

        printf("  %-3d %-11s %-6s %-14s Rp%d\n",
               i + 1, platRiwayat[i], jenisTeks, durasiTeks, bayarRiwayat[i]);
    }
}

void fiturMajukan(void) {
    char waktu[24];

    if (offsetMenit == 0) {
        puts("  Waktu sekarang mengikuti waktu nyata.");
    } else {
        printf("  Waktu dimajukan +%ld menit dari waktu nyata.\n", offsetMenit);
    }

    int m = inputAngka("Majukan berapa menit (0 untuk kembali ke waktu nyata): ");

    if (m == 0) {
        offsetMenit = 0;
        puts("  Waktu kembali mengikuti waktu nyata.");
    } else {
        offsetMenit = offsetMenit + m;
        printf("  Waktu berhasil dimajukan %d menit.\n", m);
    }

    tampilWaktu(menitSekarang(), waktu);
    printf("  Waktu sekarang menjadi: %s\n", waktu);
}

/* ================================================================
 *  5. PROGRAM UTAMA
 * ================================================================ */

int main(void) {
    int pilihan;

    do {
        tampilMenu();
        pilihan = inputAngka("Pilih menu (0-5): ");

        switch (pilihan) {
            case 1: fiturMasuk();    break;
            case 2: fiturKeluar();   break;
            case 3: fiturPetaSlot(); break;
            case 4: fiturRiwayat();  break;
            case 5: fiturMajukan();  break;
            case 0: puts("\nTerima kasih. Program selesai."); break;
            default: puts("  Pilihan tidak valid!"); break;
        }

        if (pilihan != 0) jeda();

    } while (pilihan != 0);

    return 0;
}