CXX         = g++
ASN1C       = asn1c
INCLUDEPATH = -I. -Ilib -Ilogiclayerserialization/src\
                -I/usr/local/include \
                -I/opt/local/include \
                -I/opt/local/include/cc++2 \
                -I/usr/local/include/cc++2 \
		-I/usr/include/cc++2

# DEFINES=-DHAVE_MMAP64

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

LIB_OBJS=lib/grid.o \
	lib/port_color_manager.o \
	lib/debug.o \
	lib/plugins.o \
	lib/alignment_marker.o \
	lib/memory_map.o \
	lib/graphics.o \
	lib/quadtree.o \
	lib/logic_model.o \
	lib/img_algorithms.o \
	lib/renderer.o \
	lib/project.o \
	lib/scaling_manager.o

GUI_OBJS=gui/SplashWin.o \
	gui/NewProjectWin.o \
	gui/GridConfigWin.o \
	gui/GateConfigWin.o \
	gui/GateListWin.o \
	gui/PortColorsWin.o \
	gui/GateSelectWin.o \
	gui/PortSelectWin.o \
	gui/ImageWin.o \
	gui/InProgressWin.o \
	gui/ObjectMatchingWin.o \
	gui/SetNameWin.o \
	gui/SetThresholdWin.o \
	gui/SetOrientationWin.o \
	gui/ProjectSettingsWin.o \
	gui/TemplateMatchingParamsWin.o \
	gui/ConnectionInspectorWin.o \
	gui/MainWin.o \
	gui/main.o

PLUGIN_TEMPLATE_OBJS=plugins/template.o

all: libcheck asn1_lib degate plugins

libcheck:
	@pkg-config --print-errors $(LIB_NAMES) || (echo "Error: unknown libs."; exit 1)
	@Magick-config --cppflags --cflags || (echo "You need libmagick (+libs +header)"; exit 1)
#	@freetype-config -cflags || (echo "You need libfreetype + libs +headers"; exit 1)

# 
degate: $(LIB_OBJS) $(GUI_OBJS)
	$(CXX) $(CXXFLAGS) -rdynamic $(LIBS) -o degate \
		$(LIB_OBJS) $(GUI_OBJS) logiclayerserialization/logiclayerserialization.a

plugins: plugin_clean plugin_template

plugin_template: $(PLUGIN_TEMPLATE_OBJS)
	$(CXX) -fPIC $(CXXFLAGS) -shared -o plugins/template.so $(PLUGIN_TEMPLATE_OBJS)

check: $(LIB_OBJS)
	for i in test/*.c; do \
		$(CXX) $(CXXFLAGS) $(LIBS) -o $$i.test $$i $(LIB_OBJS) && $$i.test; \
		done

asn1_gen_stub_code:
	-mkdir logiclayerserialization/src
	cd logiclayerserialization/src && $(ASN1C) -fskeletons-copy -fcompound-names ../protocol.asn1

asn1_lib:
	cd logiclayerserialization &&  make

asn1_test: asn1_lib logiclayerserialization/test.o
	$(CXX) $(CXXFLAGS) -o asn1test \
		logiclayerserialization/test.o \
		logiclayerserialization/logiclayerserialization.a

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.c.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

asn1_clean:
	-rm \
		logiclayerserialization/src/*.o \
		logiclayerserialization/*~ \
		logiclayerserialization/src/*~ \
		logiclayerserialization/src/Makefile.am.sample \
		logiclayerserialization/logiclayerserialization.a

plugin_clean:
	-rm \
		plugins/*.o plugins/*.so plugins/*~ plugins/*.rpo \

clean: plugin_clean asn1_clean
	-rm \
		gui/*.o gui/*.rpo gui/*~ gui/*.core \
		lib/*.o lib/*.rpo lib/*~ \
		test/*.test
	-rm -rf doc/api

documentation:
	doxygen Doxyfile

run:
	export DEGATE_HOME=.;export DEGATE_PLUGINS=plugins; \
		gdb -x gdb_commands -q ./degate

run2:
	export DEGATE_HOME=.;export DEGATE_PLUGINS=plugins; \
		./degate aaa06

stats:
	wc -l lib/*.[ch] gui/*.cc gui/*.h plugins/*.cc

opt:
	make OPTIMIZATION_FLAGS="-O3 -finline-functions -finline-functions-called-once -fearly-inlining"
