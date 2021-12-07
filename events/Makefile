CC := g++
CFLAGS := -g -O3 -I build
BUILD := build
PROTO_FILES := $(wildcard *.proto)
OBJS := $(patsubst %.proto, build/%.pb.o, $(PROTO_FILES))


default: all
.PHONY: all

all: build/events.a

$(BUILD):
	mkdir -p $@

$(BUILD)/%.pb.cc $(BUILD)/%.pb.h: %.proto | $(BUILD)
	protoc $(PROTOC_FLAGS) --cpp_out=$(BUILD) $<

$(BUILD)/%.pb.o: $(BUILD)/%.pb.cc $(BUILD)/%.pb.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/events.a: $(OBJS)
	ar rcs $@ $^

build/%.d: %.proto | $(BUILD)
	protoc --cpp_out=$(BUILD) --dependency_out=$@ $^
	@echo "" >> $@   # Because protoc isn't observing Unix text file conventions.

build/depend: $(PROTO_FILES:%.proto=build/%.d)
	cat $^ > $@

depend: build/depend
.PHONY: depend

clean:
	rm -rf $(BUILD)
.PHONY: clean

debug:
	@echo PROTO_FILES $(PROTO_FILES)
	@echo OBJS $(OBJS)
.PHONY: debug

-include build/depend
