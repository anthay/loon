# makefile for loon-cpp
# see http://loonfile.info

TARGET = loontest
LIBS = 
CC = clang++
CFLAGS = -std=c++11 -stdlib=libc++ -Wall
SRC_DIR  = ../../src
TEST_DIR = ../../test
INCLUDES = -I$(SRC_DIR)

OBJECTS = test.o var.o loon_reader.o loon_writer.o
HEADERS = 

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@



all: $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm $(OBJECTS)
	rm $(TARGET)



$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

test.o: $(TEST_DIR)/test.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

var.o: $(TEST_DIR)/var.cpp $(TEST_DIR)/var.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

loon_reader.o: $(SRC_DIR)/loon_reader.cpp $(SRC_DIR)/loon_reader.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

loon_writer.o: $(SRC_DIR)/loon_writer.cpp $(SRC_DIR)/loon_writer.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

