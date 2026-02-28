def:
	g++ Seq.cpp -o S && ./S

A:
	g++ -mavx2 -std=c++17 A.cpp -o A && ./A

B:
	g++ -fopenmp B.cpp -o B && ./B

C:
	g++ -fopenmp C.cpp -o C && ./C

D:
	g++ -fopenmp -mavx2 D.cpp -o D && ./D

all:
	g++ Seq.cpp -o S && ./S
	g++ -mavx2 -std=c++17 A.cpp -o A && ./A
	g++ -fopenmp B.cpp -o B && ./B
	g++ -fopenmp C.cpp -o C && ./C
	g++ -fopenmp -mavx2 D.cpp -o D && ./D

