cppver = 17
files = main.cc

a.out: main.cc
	g++ --std=c++$(cppver) $(files) -o a.out

dbg.out: main.cc
	g++ -g --std=c++$(cppver) $(files) -o dbg.out

dbg: ./dbg.out

run: ./a.out
	./a.out
