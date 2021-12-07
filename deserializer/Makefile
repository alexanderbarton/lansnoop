CC := g++
INCLUDES := -I ../events/build -I ../common
CFLAGS := -g -std=c++17 -Wall -O3 $(INCLUDES)
LIBS := ../common/build/common.a ../events/build/events.a
BUILD := build
LFLAGS := -lprotobuf
SRCS := $(wildcard *.cpp)
OBJS:=$(patsubst %.cpp, build/%.o, $(wildcard *.cpp))

# Protobuffers 3.6.0 or 3.6.1 may have broken something so the std::system_error is thrown before main().
# See https://github.com/protocolbuffers/protobuf/issues/4958
# Here's a workaround:
#
LFLAGS += -pthread -Wl,--no-as-needed


default: all
.PHONY: all

all: $(BUILD)/deserializer

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %.cpp | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/deserializer: $(OBJS) $(LIBS)
	$(CC) $^ $(LFLAGS) -o $@

build/%.d: %.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MM -MT $(patsubst %.cpp,build/%.o, $<) -MF $@ $<

build/depend: $(SRCS:%.cpp=build/%.d)
	cat $^ > $@

depend: build/depend
.PHONY: depend

clean:
	rm -rf $(BUILD)
.PHONY: clean

debug:
	@echo OBJS $(OBJS)
.PHONY: debug

-include build/depend
