all: master bin_adder

master: master.cpp
	g++ -o master master.cpp

bin_adder: bin_adder.cpp
	g++ -o bin_adder bin_adder.cpp

clean:
	-rm master bin_adder
