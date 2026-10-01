# ---- compiler dan opsi ----
CC      = cc
CFLAGS  = -std=c99 -Wall -Wextra -g
LDLIBS  = -lsqlite3

# ---- nama file ----
# NAMA_DB di parkir.c nilainya harus sama dengan DB di bawah
NAMA    = parkir
SUMBER  = parkir.c
DB      = parkir.db

# -----------------------------
# Perintah:  make          -> compile
#            make run      -> compile lalu jalan
#            make clean    -> hapus hasil compile
# -----------------------------

all: $(NAMA)

$(NAMA): $(SUMBER)
	$(CC) $(CFLAGS) $(SUMBER) -o $@ $(LDLIBS)

run: $(NAMA)
	./$(NAMA)

clean:
	rm -rf $(NAMA) $(NAMA).dSYM $(DB)

.PHONY: all run clean
