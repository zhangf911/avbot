#aCXXFLAGS+=-D_WIN32
getip.exe: GetIP.o IPLocation.o
	$(CXX) $^ -o $@  -static -lws2_32
