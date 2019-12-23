# Default Target
testnic: main.o TestNIC.o
	g++ -I.. -o testnic main.o TestNIC.o

main.o:	main.cpp
	g++ -I.. -c main.cpp

TestNIC.o: TestNIC.h TestNIC.cpp
	g++ -I.. -c TestNIC.cpp

