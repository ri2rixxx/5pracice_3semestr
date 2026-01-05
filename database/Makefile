CXX = g++
CXXFLAGS = -std=c++17 -I./include -I./src -pthread
TARGET = server

SRCS = \
    src/main.cpp \
    src/server.cpp \
    src/command_processor.cpp \
    src/simple_commands.cpp \
    src/database_extended.cpp \
    src/structures/stack.cpp \
    src/structures/set.cpp \
    src/structures/hash_table.cpp \
    src/structures/queue.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run
