CXX         = g++
ASN1C       = asn1c
INCLUDEPATH = -I. -Ilib -Ilogiclayerserialization/src\
                -I/usr/local/include \
                -I/opt/local/include \
                -I/opt/local/include/cc++2 \
                -I/usr/local/include/cc++2 \
		-I/usr/include/cc++2

#DEFINES=-DHAVE_MMAP64

#OPTIMIZATION_FLAGS=-O3 -finline-functions -finline-functions-called-once -fearly-inlining
OPTIMIZATION_FLAGS=-O0

DEBUG_FLAGS=-ggdb -D DEBUG
#DEBUG=

LIB_NAMES=gtkmm-2.4 gthread-2.0 cairomm-1.0 libglademm-2.4 glibmm-2.4 libconfig++


CXXFLAGS=$(DEBUG_FLAGS) -Wall $(OPTIMIZATION_FLAGS) -fPIC \
	$(DEFINES) $(INCLUDEPATH) \
	`pkg-config --cflags $(LIB_NAMES)` \
	`Magick-config --cppflags --cflags` \
	`freetype-config --cflags`

LIBS=-lstdc++ -lc -lpthread -ldl \
	`Wand-config --ldflags --libs` \
	`freetype-config --libs` \
	`pkg-config --print-errors --libs $(LIB_NAMES)`

TEST_OBJS=QuadTreeTest.o
OBJS= Rectangle.o

all: test

libcheck:
	@pkg-config --print-errors $(LIB_NAMES) || (echo "Error: unknown libs."; exit 1)
	@Magick-config --cppflags --cflags || (echo "You need libmagick (+libs +header)"; exit 1)
#	@freetype-config -cflags || (echo "You need libfreetype + libs +headers"; exit 1)

# 
test: $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -rdynamic $(LIBS) -lcppunit -o test $(OBJS) $(TEST_OBJS) main.cc

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.c.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean: 
	-rm *.o
