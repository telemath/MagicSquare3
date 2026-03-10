# imageconverter - A command-line application for converting between different image types.
# Requires eigen3.
# Everything in this project, except for main.cc and this makefile, comes from libstitch.

TARGET=MagicSquare

INCS = \
    BigInteger.hpp \
    BigIntegerAlgorithms.hpp \
    BigIntegerUtils.hpp \
    BigUnsigned.hpp \
    BigUnsignedInABase.hpp

SRCS = \
    BigInteger.cpp \
    BigIntegerAlgorithms.cpp \
    BigIntegerUtils.cpp \
    BigUnsigned.cpp \
    BigUnsignedInABase.cpp \
    MagicSquare3.cpp

OBJS=$(SRCS:.cpp=.o)

CC = gcc
CFLAGS = -O3 -std=c++11 -fPIC -pthread -Wno-deprecated -static
INCLUDE = -I/usr/include
LIBS = -lm -lpthread -lstdc++

%.o: %.cpp $(INCS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

make: $(OBJS)
	$(CC) $(CFLAGS) -s -o $(TARGET) $(OBJS) $(INCLUDE) $(LIBS)

.PHONY:
	clean

clean:
	rm -f $(OBJS) $(TARGET)

cleanobjs:
	rm -f $(OBJS)

help:
	@echo "make           - makes imageconverter"
	@echo "make cleanobjs - cleans object files only"
	@echo "make clean     - cleans object files and executable"
