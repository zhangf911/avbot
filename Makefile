#CXXFLAGS+= -fPIC

all : lib getip

lib:getip.a


getip.a:IPLocation.o
	ar r $@ $^

getip: test/test.cpp
	$(CXX) $^ -o $@ getip.a $(CXXFLAGS)
	
clean:
	$(RM) *.o getip *.lib *.so *.a 