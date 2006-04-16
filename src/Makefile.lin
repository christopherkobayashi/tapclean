# Project: tapclean
# gcc main.c crc32.c clean.c tap2audio.c loader_id.c scanners/*.c -o tapclean -lm

CPP  = g++
CC   = gcc
INCS =  
CXXINCS = 
CFLAGS = $(INCS)  
CXXFLAGS = $(CXXINCS)  
LIBS = -lm  
BIN  = tapclean

OBJ  = main.o clean.o crc32.o batchscan.o filesearch.o loader_id.o scanners/wildload.o scanners/aces.o scanners/anirog.o scanners/atlantis.o scanners/audiogenic.o scanners/bleep_f1.o scanners/bleep_f2.o scanners/burner.o scanners/c64tape.o scanners/chr.o scanners/cyberload_f1.o scanners/cyberload_f2.o scanners/cyberload_f3.o scanners/cyberload_f4.o scanners/enigma.o scanners/firebird.o scanners/flashload.o scanners/freeload.o scanners/hitec.o scanners/hitload.o scanners/ikloader.o scanners/jetload.o scanners/microload.o scanners/nova_f1.o scanners/nova_f2.o scanners/ocean.o scanners/oceannew1t1.o scanners/oceannew1t2.o scanners/oceannew2.o scanners/palace_f1.o scanners/palace_f2.o scanners/pause.o scanners/pavloda.o scanners/rackit.o scanners/rasterload.o scanners/seuck.o scanners/snakeload50t1.o scanners/snakeload50t2.o scanners/snakeload51.o scanners/superpav.o scanners/supertape.o scanners/tdi.o scanners/turbotape.o scanners/turrican.o scanners/usgold.o scanners/virgin.o scanners/visiload.o tap2audio.o $(RES)

LINKOBJ  = main.o clean.o crc32.o batchscan.o filesearch.o loader_id.o scanners/wildload.o scanners/aces.o scanners/anirog.o scanners/atlantis.o scanners/audiogenic.o scanners/bleep_f1.o scanners/bleep_f2.o scanners/burner.o scanners/c64tape.o scanners/chr.o scanners/cyberload_f1.o scanners/cyberload_f2.o scanners/cyberload_f3.o scanners/cyberload_f4.o scanners/enigma.o scanners/firebird.o scanners/flashload.o scanners/freeload.o scanners/hitec.o scanners/hitload.o scanners/ikloader.o scanners/jetload.o scanners/microload.o scanners/nova_f1.o scanners/nova_f2.o scanners/ocean.o scanners/oceannew1t1.o scanners/oceannew1t2.o scanners/oceannew2.o scanners/palace_f1.o scanners/palace_f2.o scanners/pause.o scanners/pavloda.o scanners/rackit.o scanners/rasterload.o scanners/seuck.o scanners/snakeload50t1.o scanners/snakeload50t2.o scanners/snakeload51.o scanners/superpav.o scanners/supertape.o scanners/tdi.o scanners/turbotape.o scanners/turrican.o scanners/usgold.o scanners/virgin.o scanners/visiload.o tap2audio.o $(RES)

all: tapclean

$(BIN): $(OBJ)
	$(CC) $(LINKOBJ) -o tapclean $(LIBS)

