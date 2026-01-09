XENC_SOURCES = $(wildcard $(SRC_DIR)/xenc/*.c)
XENC_HEADERS = $(wildcard $(SRC_DIR)/xenc/*.h)
XENC_OBJECTS = $(XENC_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
XENC_DEPS = $(XENC_OBJECTS:.o=.d)

XENC_CFLAGS = -I$(SRC_DIR)/xenc
XENC_BIN = $(BIN_DIR)/xenc$(EXT)
ALL_TARGETS += $(XENC_BIN)

-include $(XENC_DEPS)

$(XENC_BIN): $(XENC_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $(XENC_OBJECTS) $(LDFLAGS)
	@echo "Built: $@"

$(OBJ_DIR)/xenc/%.o: $(SRC_DIR)/xenc/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(XENC_CFLAGS) -c $< -o $@
	@echo "Compiled: $<"

.PHONY: xenc-clean

xenc-clean:
	rm -f $(XENC_OBJECTS) $(XENC_DEPS) $(XENC_BIN)
