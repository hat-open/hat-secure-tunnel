.POSIX:
.SUFFIXES:

include config.mk

FORMAT_STYLE = {BasedOnStyle: llvm,\
	            IndentWidth: 4,\
	            MaxEmptyLinesToKeep: 2,\
	            TabWidth: 4}

all: $(BIN) $(MAN)

install: $(BIN) $(MAN)
	mkdir -p $(DESTDIR)$(bindir)
	mkdir -p $(DESTDIR)$(mandir)/man1
	cp $(BIN) $(DESTDIR)$(bindir)
	gzip <$(MAN) >$(DESTDIR)$(mandir)/man1/$(name).1.gz

format:
	clang-format -i -style="$(FORMAT_STYLE)" $(SOURCES) $(HEADERS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all install format clean

$(BIN): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(LDFLAGS_ALL) -o $(BIN) $(OBJECTS) $(LIBS_ALL)

$(MAN): $(DOC_MANUAL) $(DOC_JSON_SCHEMA)
	@mkdir -p $(BUILD_DIR)
	m4 -D _NAME=$(name) \
	   -D _TITLE=$$(echo $(name) | tr '[:lower:]' '[:upper:]') \
	   -D _JSON_SCHEMA_PATH=$(DOC_JSON_SCHEMA) \
	   $(DOC_MANUAL) >$(MAN)

include rules.mk
