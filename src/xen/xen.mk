XEN_SOURCES = $(wildcard $(SRC_DIR)/xen/*.c) \
              $(wildcard $(SRC_DIR)/xen/builtin/*.c)

XEN_HEADERS = $(wildcard $(SRC_DIR)/xen/*.h) \
              $(wildcard $(SRC_DIR)/xen/builtin/*.h)

XEN_OBJECTS = $(XEN_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
XEN_DEPS = $(XEN_OBJECTS:.o=.d)

XEN_CFLAGS = -I$(SRC_DIR)/xen \
             -I$(SRC_DIR)/xen/builtin
XEN_BIN = $(BIN_DIR)/xen$(EXT)
ALL_TARGETS += $(XEN_BIN)

-include $(XEN_DEPS)

$(XEN_BIN): $(XEN_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $(XEN_OBJECTS) $(LDFLAGS)
	@echo "Built: $@"

$(OBJ_DIR)/xen/%.o: $(SRC_DIR)/xen/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(XEN_CFLAGS) -c $< -o $@
	@echo "Compiled: $<"

.PHONY: xen-clean

xen-clean:
	rm -f $(XEN_OBJECTS) $(XEN_DEPS) $(XEN_BIN)
