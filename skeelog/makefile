skeelog:
	x86_64-w64-mingw32-gcc -O3 -Wall -Wextra -Wno-unused-parameter -c -fmessage-length=0 -o obj/main.o "source/skeelog.cpp"
	x86_64-w64-mingw32-gcc -static -m64 -std=c11 -pedantic-errors -o bin/skeelog.exe obj/main.o -lws2_32 -lmswsock -ladvapi32 -lshlwapi

clean:
	rm -f bin/*.exe
	rm -f obj/*.o
