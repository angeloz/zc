CC ?= cc
COSMOCC ?= cosmocc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?=
LDFLAGS ?=

TARGET = zc
BUNDLED_KILO = zc-kilo
SRC = src/zc.c
KILO_DIR = third_party/kilo

.PHONY: all clean cosmo

all: $(TARGET) $(BUNDLED_KILO)

$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -D_DEFAULT_SOURCE -D_DARWIN_C_SOURCE -o $@ $(SRC) $(LDFLAGS)

$(BUNDLED_KILO): $(KILO_DIR)/kilo.c $(KILO_DIR)/Makefile
	$(MAKE) -C $(KILO_DIR) CC="$(CC)"
	cp $(KILO_DIR)/kilo $@

cosmo: $(SRC)
	$(COSMOCC) $(CPPFLAGS) $(CFLAGS) -o $(TARGET).com $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET).com $(BUNDLED_KILO)
	-$(MAKE) -C $(KILO_DIR) clean
