#aCXXFLAGS+=-D_WIN32
getip.lib:IPLocation.o
	$(CXX) $^ -o $@  -static -shared
