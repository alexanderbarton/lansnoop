CC := g++
INCLUDES := -I ../events/build
CFLAGS := -g -Wall -O3 $(INCLUDES)
BUILD := build
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, build/%.o, $(wildcard *.cpp))


default: all
.PHONY: all

all: $(BUILD)/common.a

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %.cpp ../events/build/*.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/common.a: $(OBJS)
	ar rcs $@ $^

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
	@echo SRCS $(SRCS)
.PHONY: debug

-include build/depend
