CXX = g++
CXXFLAGS = -std=c++17
TARGET = statistics_service

SRCS = main.cpp statistics_service.cpp db_client.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
