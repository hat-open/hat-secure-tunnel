.POSIX:
.SUFFIXES:

include config.mk

FORMAT_STYLE = {BasedOnStyle: llvm,\
	            IndentWidth: 4,\
	            MaxEmptyLinesToKeep: 2,\
	            TabWidth: 4}

all: $(BIN)

install: $(BIN)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	cp $(BIN) $(DESTDIR)$(PREFIX)/bin
	gzip <man/hat-iec62351-tunnel.1 >$(DESTDIR)$(PREFIX)/share/man/man1/hat-iec62351-tunnel.1.gz

format:
	clang-format -i -style="$(FORMAT_STYLE)" $(SOURCES) $(HEADERS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all install format clean

include rules.mk
