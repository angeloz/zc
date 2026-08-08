CC ?= cc
COSMOCC ?= cosmocc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?=
LDFLAGS ?=
KILO_CFLAGS ?= -Wall -W -pedantic -std=c99
ZC_CPPFLAGS = $(CPPFLAGS) -D_DEFAULT_SOURCE -D_DARWIN_C_SOURCE

TARGET = zc
BUNDLED_KILO = zc-kilo
COSMO_TARGET = $(TARGET).com
COSMO_BUNDLED_KILO = $(BUNDLED_KILO).com
SRC = src/zc.c
KILO_DIR = third_party/kilo
DIST_DIR = dist
BUNDLE_DIR = $(DIST_DIR)/zc-bundle
COSMO_BUNDLE_DIR = $(DIST_DIR)/zc-cosmo-bundle

.PHONY: all clean cosmo bundle bundle-cosmo

all: $(TARGET) $(BUNDLED_KILO)

$(TARGET): $(SRC)
	$(CC) $(ZC_CPPFLAGS) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

$(BUNDLED_KILO): $(KILO_DIR)/kilo.c $(KILO_DIR)/Makefile
	$(MAKE) -C $(KILO_DIR) CC="$(CC)"
	cp $(KILO_DIR)/kilo $@

$(COSMO_TARGET): $(SRC)
	$(COSMOCC) $(ZC_CPPFLAGS) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

$(COSMO_BUNDLED_KILO): $(KILO_DIR)/kilo.c
	$(COSMOCC) $(CPPFLAGS) $(KILO_CFLAGS) -o $@ $(KILO_DIR)/kilo.c $(LDFLAGS)

cosmo: $(COSMO_TARGET) $(COSMO_BUNDLED_KILO)

bundle: all
	mkdir -p $(BUNDLE_DIR)/tools
	cp $(TARGET) $(BUNDLED_KILO) $(BUNDLE_DIR)/

bundle-cosmo: cosmo
	mkdir -p $(COSMO_BUNDLE_DIR)/tools
	cp $(COSMO_TARGET) $(COSMO_BUNDLED_KILO) $(COSMO_BUNDLE_DIR)/

clean:
	rm -f $(TARGET) $(COSMO_TARGET) $(BUNDLED_KILO) $(COSMO_BUNDLED_KILO)
	rm -rf $(DIST_DIR)
	-$(MAKE) -C $(KILO_DIR) clean
