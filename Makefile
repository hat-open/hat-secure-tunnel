.POSIX:
.SUFFIXES:

include config.mk

FORMAT_STYLE = {BasedOnStyle: llvm,\
	            IndentWidth: 4,\
	            MaxEmptyLinesToKeep: 2,\
	            TabWidth: 4}

all: $(BIN)

install: $(BIN)
	mkdir -p $(DESTDIR)$(bindir)
	mkdir -p $(DESTDIR)$(mandir)/man1
	cp $(BIN) $(DESTDIR)$(bindir)
	gzip <$(srcdir)/man/hat-iec62351-tunnel.1 >$(DESTDIR)$(mandir)/man1/hat-iec62351-tunnel.1.gz

format:
	clang-format -i -style="$(FORMAT_STYLE)" $(SOURCES) $(HEADERS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all install format clean

include rules.mk
