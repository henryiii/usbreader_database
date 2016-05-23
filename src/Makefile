CXX    := clang++
GXX    := CC
CFLAGS := -g -O3 -std=c++11 -I. -I/usr/local/include -I/usr/include/root
LFLAGS := -g -L. -L/usr/local/lib -L/usr/lib/i386-linux-gnu -L/physics/ROOT/v5-34-18/lib -lstdc++ -lz -lusb-1.0 -lusb -lm -lc -lGui -lCore -lCint -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -pthread -lm -ldl -rdynamic 
PFLAGS := -lpqxx -lpq

all: setup_daq receive_one receive_one_from_db select analyze convert_data_to_root show_db viewer analyze_data asyn

setup_daq: setup_daq.o Word.o Event.o Packet.o Signal.o Level.o
	$(GXX) -o $@ $^ $(LFLAGS)

receive_one: receive_one.o Word.o Event.o Packet.o Level.o Signal.o
	$(GXX) -o $@ $^ $(LFLAGS) $(PFLAGS)

receive_one_from_db: receive_one_from_db.o Word.o Event.o Packet.o Level.o Signal.o
	$(GXX) -o $@ $^ $(LFLAGS) $(PFLAGS)

select: select.o Word.o Event.o Packet.o Level.o Signal.o
	$(GXX) -o $@ $^ $(LFLAGS) $(PFLAGS)

analyze: analyze.o Signal.o Level.o Packet.o Event.o Word.o
	$(GXX) -o $@ $^ $(LFLAGS) $(PFLAGS)

convert_data_to_root: convert_data_to_root.o Signal.o Level.o Packet.o Event.o Word.o
	$(GXX) -o $@ $^ $(LFLAGS) $(PFLAGS)

show_db: show_db.o Signal.o Level.o Packet.o Event.o Word.o
	$(GXX) -o $@ $^ $(LFLAGS) $(PFLAGS)

viewer: viewer.o Signal.o Level.o Packet.o Event.o Word.o
	$(GXX) -o $@ $^ $(LFLAGS) $(PFLAGS)

analyze_data: analyze_data.o Signal.o Level.o Packet.o Event.o Word.o
	$(GXX) -o $@ $^ $(LFLAGS) $(PFLAGS)

asyn: asyn.o Signal.o Level.o Packet.o Event.o Word.o
	$(GXX) -o $@ $^ $(LFLAGS) $(PFLAGS)

analyze.o: analyze.cxx Signal.h Level.h Packet.h Event.h Word.h Signal.cxx Level.cxx Packet.cxx Event.cxx Word.cxx
	$(GXX) $(CFLAGS) -c analyze.cxx	

convert_data_to_root.o: convert_data_to_root.cxx Signal.h Level.h Packet.h Event.h Word.h Signal.cxx Level.cxx Packet.cxx Event.cxx Word.cxx
	$(GXX) $(CFLAGS) -c convert_data_to_root.cxx	

show_db.o: show_db.cxx Signal.h Level.h Packet.h Event.h Word.h Signal.cxx Level.cxx Packet.cxx Event.cxx Word.cxx
	$(GXX) $(CFLAGS) -c show_db.cxx	

viewer.o: viewer.cxx Signal.h Level.h Packet.h Event.h Word.h Signal.cxx Level.cxx Packet.cxx Event.cxx Word.cxx
	$(GXX) $(CFLAGS) -c viewer.cxx	

analyze_data.o: analyze_data.cxx Signal.h Level.h Packet.h Event.h Word.h Signal.cxx Level.cxx Packet.cxx Event.cxx Word.cxx
	$(GXX) $(CFLAGS) -c analyze_data.cxx	

receive_one.o: receive_one.cxx special_functions_usb.h Word.h Event.h Level.h Packet.h Signal.h Signal.cxx Packet.cxx Event.cxx Level.cxx Word.cxx
	$(GXX) $(CFLAGS) -c receive_one.cxx

receive_one_from_db.o: receive_one_from_db.cxx special_functions_usb.h Word.h Event.h Level.h Packet.h Signal.h Signal.cxx Packet.cxx Event.cxx Level.cxx Word.cxx
	$(GXX) $(CFLAGS) -c receive_one_from_db.cxx

select.o: select.cxx special_functions_usb.h Word.h Event.h Level.h Packet.h Signal.h Signal.cxx Packet.cxx Event.cxx Level.cxx Word.cxx
	$(GXX) $(CFLAGS) -c select.cxx

setup_daq.o: setup_daq.cxx special_functions_usb.h usbfunctions.h functions_usb.h Word.h Event.h Level.h Packet.h Signal.h Signal.cxx Packet.cxx Event.cxx Level.cxx Word.cxx
	$(GXX) $(CFLAGS) -c setup_daq.cxx

asyn.o: asyn.cxx special_functions_usb.h usbfunctions.h functions_usb.h Word.h Event.h Level.h Packet.h Signal.h Signal.cxx Packet.cxx Event.cxx Level.cxx Word.cxx
	$(GXX) $(CFLAGS) -c asyn.cxx

Word.o: Word.h Word.cxx
	$(GXX) $(CFLAGS) -c Word.cxx

Event.o: Event.h Event.cxx Word.h Word.cxx
	$(GXX) $(CFLAGS) -c Event.cxx

Level.o: Level.h Level.cxx Event.h Packet.h Signal.h Event.cxx Packet.cxx Signal.cxx
	$(GXX) $(CFLAGS) -c Level.cxx

Signal.o: Signal.h Signal.cxx
	$(GXX) $(CFLAGS) -c Signal.cxx

Packet.o: Packet.h Packet.cxx Word.h Word.cxx Event.h Event.cxx
	$(GXX) $(CFLAGS) -c Packet.cxx
clean:
	rm -rf *.o setup_daq receive_one receive_one_from_db select analyze convert_data_to_root show_db viewer analyze_data 
