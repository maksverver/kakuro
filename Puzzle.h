#ifndef PUZZLE_H
#define PUZZLE_H

#include <memory>
#include <vector>
#include <string>

inline bool single(int cand) { return (cand&(cand-1)) == 0; }
inline bool digit(int v) { return v && single(v); }
int log2(int cand);

struct Grid
{
    int height, width;
    std::vector<int> vars;

    void crop();
    void enlarge(int top = 1, int bottom = 1, int left = 1, int right = 1);

    inline int *operator[] (int r) { return &vars[width*r]; }
    inline const int *operator[] (int r) const { return &vars[width*r]; }

    bool operator==(const Grid &g) const {
        return height == g.height && width == g.width && vars == g.vars;
    }

    std::string toString() const;
};

struct State
{
    inline State() : vars(0), grps(0) { };

    int vars, grps;

    std::vector<int> hgrp;               // grp[x] is the horizontal group of x
    std::vector<int> vgrp;               // vgrp[x] is the vertical group of x
    std::vector<int> cand;               // bits in cand[x] are candidates for variable x
    std::vector<int> sum;                // sum[g] is sum for group g
    std::vector<std::vector<int> > mem;  // group[g][m] is member m of group g

    void clearDigits();
    void clearCandidates();
    void clearSums();
    void calculateCandidates();
    void calculateSums();

    int solve(std::vector<int> *solution);

    std::string grpsToString() const;
    std::string varsToString(bool candidates) const;

    int givens(int grp, int without) const;
    int cands(int grp, int without) const;

    bool operator==(const State &s) const {
        return hgrp == s.hgrp && vgrp == s.vgrp && cand == s.cand && sum == s.sum && mem == s.mem;
    }

protected:
    int search(std::vector<int> *solution);
};

struct Puzzle
{
    enum ExportOptions { exportGrid, exportGridSums,
                         exportGridSumsDigits, exportGridSumsCandidates };

    Grid    grid;
    State   state;

    static std::unique_ptr<Puzzle> fromString(const std::string &str);
    std::string toString(ExportOptions options = exportGridSumsCandidates) const;

    bool operator==(const Puzzle &p) const { return grid == p.grid && state == p.state; }
    bool operator!=(const Puzzle &p) const { return !(*this == p); }
};

#endif /* nddef PUZZLE_H */
