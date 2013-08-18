CXXFLAGS=-g -O2

project-euler-424: project-euler-424.cpp
	$(CXX) $(CXXFLAGS) -o $@ -I .. $< ../Puzzle.o ../Puzzle_solver.o ../sums.o

clean:
	rm -f project-euler-424
