CXX := g++
CXXFLAGS := -std=c++17 -g -O2 -Wall -Wextra -pedantic -MD -MP

SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
DEPFILES := $(OBJECTS:.o=.d)


all: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o wordle $(LIBS)

-include $(DEPFILES)

run: all
	time ./wordle solution_words.txt

$(OBJECTS): %.o: %.cpp

clean:
	rm -f *.o *.d wordle
