CC := g++
INCLUDES := -I ../events/build -I ../common -I include
CFLAGS := -g -std=c++17 -Wall -O3 $(INCLUDES)
LIBS := ../common/build/common.a ../events/build/events.a
BUILD := build
# LFLAGS := -lglfw -lGL -lX11 -lpthread -lXrandr -ldl
LFLAGS := -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lprotobuf
SRCS := $(wildcard *.cpp)
OBJS:=$(patsubst %.cpp, build/%.o, $(wildcard *.cpp)) build/glad.o


default: all
.PHONY: all

all: $(BUILD)/viewer

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %.cpp | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/viewer: $(OBJS) $(LIBS)
	$(CC) $^ $(LFLAGS) -o $@

$(BUILD)/glad.o: src/glad.c include/KHR/khrplatform.h include/glad/glad.h
	gcc -g -Wall -O3 $(INCLUDES) -c $< -o $@

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
