
include MakefileSTD/MakefileSTD

AR = ar
CXX = g++
CXXFLAGS = -flto -pipe -std=c++2a -pedantic -Wall -IuSockets/src -lpthread
LIBS=
LIBFILE=libusocketwrapper.a
USOCKETSFLAGS=

ifeq ($(DEBUG),1)
	CXXFLAGS += -ggdb -g -pg
else
	CXXFLAGS += -O4 -s
endif

ifeq ($(platform),win)
	LIBS += -lzlib
	LIBS += -lgcc_s_seh-1 -lmfc100
	LIBS += -lmfc100u -lmsvcp100 -lmsvcr100 -lmsvcr100_clr0400
else
	LIBS += -L/usr/lib -ldl -lz
	CXXFLAGS += -fPIC
	CXXFLAGS += -I/usr/include
	LIBS += -luv
	USOCKETSFLAGS += USE_LIBUV=1
endif
LIBS += -lpthread -lssl -lcrypto

INCLUDE = -I./src/ -I./concurrent -I./uSockets/src
CXXFLAGS += $(INCLUDE)
OBJECTS += bin/Buffer.o bin/Socket.o
OBJECTS += bin/Context.o bin/Loop.o
OBJECTS += bin/Event.o bin/Debug.o

$(LIBFILE): $(OBJECTS) uSockets/uSockets.a
	cp uSockets/uSockets.a $@
	$(AR) rvs $@ $(OBJECTS)


EXAMPLE_FILES = examples/networking_test.exe examples/chat_client.exe examples/chat_server.exe

test: $(EXAMPLE_FILES)
	examples/networking_test.exe

examples/%.exe: examples/%.cpp $(LIBFILE)
	$(CXX) -o $@ $^ $(CXXFLAGS)


# uSockets:

uSockets/uSockets.a:
	cd uSockets ; make $(MFLAGS) $(USOCKETSFLAGS)


# objects:
bin/tests/%.o: tests/%.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)
#bin/networking/%.o: src/networking/%.cpp src/networking/%.hpp
#	$(CXX) -c -o $@ $< $(CXXFLAGS)
bin/%.o: src/%.cpp src/%.hpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

.PHONY: clean
clean:
	(cd uSockets ; make clean)
	$(RM) $(OBJECTS) $(LIBFILE) $(TESTS) bin/tests/*

