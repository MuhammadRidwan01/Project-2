/*
 * ======================================================================
 *  SISTEM PARKIR KAMPUS 
 * ======================================================================
 *  Cara compile:
 *      clang -std=c99 -Wall -Wextra parkir.c -o parkir -lsqlite3
 *      ./parkir
 * ======================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>
#include <time.h>

#define SLOT_MOTOR  4
#define SLOT_MOBIL  8
#define TOTAL_SLOT  (SLOT_MOTOR + SLOT_MOBIL)
#define NAMA_DB     "parkir.db"

enum { MOTOR = 0, MOBIL = 1 };

/* Global */
sqlite3 *db;
long offsetDetik = 0;

time_t waktuSekarang(void) {
    return time(NULL) + offsetDetik;
}

/* ================================================================
 *  1. HELPER INPUT & TAMPILAN SEDERHANA
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

void rapikanPlat(char *plat) {
    for (int i = 0; plat[i]; i++)
        plat[i] = toupper((unsigned char)plat[i]);
}

void fmtWaktu(time_t t, char *out) {
    if (t < 100000000)
        sprintf(out, "%02d:%02d", (int)(t / 60), (int)(t % 60));
    else
        strftime(out, 20, "%d/%m %H:%M:%S", localtime(&t));
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
    int perJam   = (jenis == MOBIL) ? 5000 : 2000;
    int maksHari = (jenis == MOBIL) ? 50000 : 20000;

    int totalJam = (menit + 59) / 60;
    if (totalJam < 1) totalJam = 1;

    int hari      = totalJam / 24;
    int sisaJam   = totalJam % 24;
    int biayaSisa = sisaJam * perJam;
    if (biayaSisa > maksHari) biayaSisa = maksHari;

    return (hari * maksHari) + biayaSisa;
}

/* ================================================================
 *  3. OPERASI DATABASE SQLITE
 * ================================================================ */

void dbExec(const char *sql) {
    sqlite3_exec(db, sql, NULL, NULL, NULL);
}

long dbAngka(const char *sql, long defVal) {
    char **hasil;
    int baris, kolom;
    long val = defVal;
    if (sqlite3_get_table(db, sql, &hasil, &baris, &kolom, NULL) == SQLITE_OK) {
        if (baris > 0 && hasil[kolom]) val = atol(hasil[kolom]);
        sqlite3_free_table(hasil);
    }
    return val;
}

void dbBuka(void) {
    sqlite3_open(NAMA_DB, &db);
    dbExec("CREATE TABLE IF NOT EXISTS app (id INTEGER PRIMARY KEY, jam INTEGER);");
    dbExec("CREATE TABLE IF NOT EXISTS kendaraan (slot INTEGER PRIMARY KEY, plat TEXT, jenis INTEGER, masuk INTEGER);");
    dbExec("CREATE TABLE IF NOT EXISTS riwayat (id INTEGER PRIMARY KEY AUTOINCREMENT, plat TEXT, jenis INTEGER, masuk INTEGER, keluar INTEGER, bayar INTEGER);");
}

void jamMuat(void) {
    offsetDetik = dbAngka("SELECT jam FROM app WHERE id = 1;", 0);
    if (offsetDetik == 480) offsetDetik = 0;
}

void jamSimpan(void) {
    char sql[64];
    sprintf(sql, "INSERT OR REPLACE INTO app (id, jam) VALUES (1, %ld);", offsetDetik);
    dbExec(sql);
}

int jumlahTerpakai(int jenis) {
    char sql[64];
    sprintf(sql, "SELECT COUNT(*) FROM kendaraan WHERE jenis = %d;", jenis);
    return (int)dbAngka(sql, 0);
}

long totalPendapatan(void) {
    return dbAngka("SELECT COALESCE(SUM(bayar), 0) FROM riwayat;", 0);
}

int cariSlotKosong(int jenis) {
    int mulai = (jenis == MOTOR) ? 0 : SLOT_MOTOR;
    int akhir = (jenis == MOTOR) ? SLOT_MOTOR : TOTAL_SLOT;
    char sql[64];
    for (int s = mulai; s < akhir; s++) {
        sprintf(sql, "SELECT COUNT(*) FROM kendaraan WHERE slot = %d;", s);
        if (dbAngka(sql, 0) == 0) return s;
    }
    return -1;
}

int cariSlotPlat(const char *plat) {
    char sql[128];
    sprintf(sql, "SELECT slot FROM kendaraan WHERE plat = '%s';", plat);
    return (int)dbAngka(sql, -1);
}

/* ================================================================
 *  4. MENU DAN FITUR UTAMA
 * ================================================================ */

void tampilMenu(void) {
    char waktu[24];
    fmtWaktu(waktuSekarang(), waktu);

    printf("\n==============================================\n");
    printf("            SISTEM PARKIR KAMPUS\n");
    printf("==============================================\n");
    printf(" Jam %s | Motor %d/%d | Mobil %d/%d\n",
           waktu, jumlahTerpakai(MOTOR), SLOT_MOTOR,
           jumlahTerpakai(MOBIL), SLOT_MOBIL);
    printf(" Total Pendapatan: Rp%ld\n", totalPendapatan());
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
    char plat[32], waktu[24], sql[256];

    input("Plat nomor: ", plat, sizeof(plat));
    rapikanPlat(plat);
    if (strlen(plat) == 0) {
        puts("  Plat tidak boleh kosong!");
        return;
    }

    if (cariSlotPlat(plat) >= 0) {
        printf("  %s sudah ada di dalam area parkir!\n", plat);
        return;
    }

    int jenis = inputAngka("Jenis (1. Motor, 2. Mobil): ") - 1;
    if (jenis != MOTOR && jenis != MOBIL) {
        puts("  Jenis kendaraan tidak valid!");
        return;
    }

    int slot = cariSlotKosong(jenis);
    if (slot < 0) {
        printf("  Parkir %s penuh!\n", (jenis == MOBIL) ? "Mobil" : "Motor");
        return;
    }

    time_t sekarang = waktuSekarang();
    sprintf(sql, "INSERT INTO kendaraan VALUES (%d, '%s', %d, %ld);",
            slot, plat, jenis, (long)sekarang);
    dbExec(sql);

    fmtWaktu(sekarang, waktu);
    printf("  %s masuk slot %d pada %s.\n", plat, slot + 1, waktu);
}

void fiturKeluar(void) {
    char plat[32], sql[256], wMasuk[24], wKeluar[24];

    input("Plat nomor: ", plat, sizeof(plat));
    rapikanPlat(plat);

    sprintf(sql, "SELECT slot, jenis, masuk FROM kendaraan WHERE plat = '%s';", plat);
    char **hasil;
    int baris, kolom;
    if (sqlite3_get_table(db, sql, &hasil, &baris, &kolom, NULL) != SQLITE_OK || baris == 0) {
        if (hasil) sqlite3_free_table(hasil);
        puts("  Kendaraan tidak ditemukan!");
        return;
    }

    int slot     = atoi(hasil[3]);
    int jenis    = atoi(hasil[4]);
    time_t masuk = atol(hasil[5]);
    sqlite3_free_table(hasil);

    time_t sekarang  = waktuSekarang();
    long durasiDetik = sekarang - masuk;
    if (durasiDetik < 0) durasiDetik = 0;

    int durasiMenit = durasiDetik / 60;
    if (durasiDetik > 0 && durasiMenit == 0) durasiMenit = 1;

    int bayar = hitungTarif(jenis, durasiMenit);

    fmtWaktu(masuk, wMasuk);
    fmtWaktu(sekarang, wKeluar);

    printf("\n  ---------- STRUK PARKIR ----------\n");
    printf("  Plat   : %s\n", plat);
    printf("  Jenis  : %s\n", (jenis == MOBIL) ? "Mobil" : "Motor");
    printf("  Masuk  : %s\n", wMasuk);
    printf("  Keluar : %s\n", wKeluar);
    printf("  Durasi : %d jam %d menit (%ld detik)\n",
           durasiMenit / 60, durasiMenit % 60, durasiDetik % 60);
    printf("  Tarif  : Rp%d\n", bayar);
    printf("  ----------------------------------\n");

    sprintf(sql, "INSERT INTO riwayat (plat, jenis, masuk, keluar, bayar) VALUES ('%s', %d, %ld, %ld, %d);",
            plat, jenis, (long)masuk, (long)sekarang, bayar);
    dbExec(sql);

    sprintf(sql, "DELETE FROM kendaraan WHERE slot = %d;", slot);
    dbExec(sql);
}

void fiturPetaSlot(void) {
    char isi[TOTAL_SLOT][32];
    for (int i = 0; i < TOTAL_SLOT; i++) strcpy(isi[i], "kosong");

    char **hasil;
    int baris, kolom;
    if (sqlite3_get_table(db, "SELECT slot, plat FROM kendaraan;", &hasil, &baris, &kolom, NULL) == SQLITE_OK) {
        for (int i = 1; i <= baris; i++) {
            int s = atoi(hasil[i * 2]);
            if (s >= 0 && s < TOTAL_SLOT) strcpy(isi[s], hasil[i * 2 + 1]);
        }
        sqlite3_free_table(hasil);
    }

    puts("\n  Slot Motor:");
    for (int i = 0; i < SLOT_MOTOR; i++)
        printf("  [%2d] %-11s\n", i + 1, isi[i]);

    puts("\n  Slot Mobil:");
    for (int i = SLOT_MOTOR; i < TOTAL_SLOT; i++)
        printf("  [%2d] %-11s\n", i + 1, isi[i]);
}

void fiturRiwayat(void) {
    char **hasil;
    int baris, kolom;
    if (sqlite3_get_table(db, "SELECT plat, jenis, masuk, keluar, bayar FROM riwayat;", &hasil, &baris, &kolom, NULL) != SQLITE_OK || baris == 0) {
        puts("  Belum ada transaksi.");
        if (hasil) sqlite3_free_table(hasil);
        return;
    }

    printf("\n  %-3s %-11s %-6s %-18s %-18s %s\n", "No", "Plat", "Jenis", "Masuk", "Keluar", "Bayar");
    for (int i = 1; i <= baris; i++) {
        char wMasuk[24], wKeluar[24];
        fmtWaktu(atol(hasil[i * 5 + 2]), wMasuk);
        fmtWaktu(atol(hasil[i * 5 + 3]), wKeluar);

        printf("  %-3d %-11s %-6s %-18s %-18s Rp%s\n",
               i,
               hasil[i * 5 + 0],
               (atoi(hasil[i * 5 + 1]) == MOBIL) ? "Mobil" : "Motor",
               wMasuk, wKeluar, hasil[i * 5 + 4]);
    }
    sqlite3_free_table(hasil);
}

void fiturMajukan(void) {
    char waktu[24];
    printf("  Offset saat ini: +%ld menit.\n", offsetDetik / 60);
    int m = inputAngka("Majukan berapa menit (0 untuk reset ke waktu nyata): ");
    if (m == 0) {
        offsetDetik = 0;
        puts("  Waktu di-reset kembali ke waktu nyata saat ini.");
    } else {
        offsetDetik += (long)m * 60;
        printf("  Waktu berhasil dimajukan %d menit.\n", m);
    }
    jamSimpan();

    fmtWaktu(waktuSekarang(), waktu);
    printf("  Waktu sekarang menjadi: %s\n", waktu);
}

/* ================================================================
 *  5. PROGRAM UTAMA
 * ================================================================ */

int main(void) {
    int pilihan;
    dbBuka();
    jamMuat();

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

    jamSimpan();
    sqlite3_close(db);
    return 0;
}
