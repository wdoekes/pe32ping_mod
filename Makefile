# Use a symlink from pe32ping_example.ino to pe32ping_example.cc (and not
# to the more common .cpp) because g++ and make will correctly guess
# their type, while the Arduino IDE does not open the .cpp file as well
# (it already has this file open as the ino file).
HEADERS = $(wildcard *.h bogoduino/*.h)
OBJECTS = pe32ping_mod.o PingMon.o PingMonUtil.o \
	  bogoduino/Arduino.o bogoduino/WString.o

# --- Test mode ---
CXX = g++
CPPFLAGS = -DTEST_BUILD -g -I./bogoduino
CXXFLAGS = -Wall -Os -fdata-sections -ffunction-sections
LDFLAGS = -Wl,--gc-sections # -s(trip)

test: ./pe32ping_mod.example
	./pe32ping_mod.example

clean:
	$(RM) $(OBJECTS) ./pe32ping_mod.example

$(OBJECTS): $(HEADERS)

pe32ping_mod.example: $(OBJECTS)
	$(LINK.cc) -o $@ $^
