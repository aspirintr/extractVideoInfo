SQLITE_LIBS=	sqlite3				\

CXXFLAGS =	-g -Wall -fmessage-length=0 $(shell pkg-config --cflags $(SQLITE_LIBS))

OBJS =		vcal.o vcal_test.o

LIBS = $(shell pkg-config --libs  $(SQLITE_LIBS))


TARGET =	vcal_test

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
