  # the compiler: gcc for C program, define as g++ for C++
CC = gcc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  #  -Wall turns on most, but not all, compiler warnings

CFLAGS  = -g -Wall

  # the build target executable:
TARGET = server

all: $(TARGET)

#no dependency between server and deliver, so no need to link them together
server: server.c
	gcc server.c -g -o server
clean:
	$(RM) $(TARGET)
