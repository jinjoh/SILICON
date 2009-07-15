CXX         = g++
INCLUDEPATH = -I. -Ilib -Ilogiclayerserialization/src\
                -I/usr/local/include \
                -I/usr/local/include/cc++2 \
		-I/usr/include/cc++2


DEBUG_FLAGS=-ggdb -D DEBUG
#DEBUG=


CXXFLAGS=$(DEBUG_FLAGS) -Wall $(OPTIMIZATION_FLAGS) -fPIC \
	$(DEFINES) $(INCLUDEPATH)

LIBPATH=-L/opt/local/lib

LIBS=-lstdc++ -lc -lpthread -ldl

TEST_OBJS=QuadTreeTest.o
OBJS=BoundingBox.o Rectangle.o Point.o Circle.o

all: clean test

 
test: $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -rdynamic $(LIBPATH) $(LIBS) -lcppunit -o test $(OBJS) $(TEST_OBJS) main.cc

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.c.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean: 
	-rm *.o
