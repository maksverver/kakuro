/* Project Euler problem 424 
   see http://projecteuler.net/problem=424 */

#include "Puzzle.h"
#include "assert.h"
#include <algorithm>
#include <iostream>
#include <string>

static long long solve(const char *p)
{
    assert(*p > '0' && *p <= '9');
    int size = *p++ - '0';

    std::vector<std::vector<char> > grid(size, std::vector<char>(size, 'X'));
    std::vector<std::vector<int> > vars(size, std::vector<int>(size, -1));
    std::vector<std::vector<std::string> >
        hlabel(size, std::vector<std::string>(size)),
        vlabel(size, std::vector<std::string>(size));

    std::vector<std::string> sum_labels;            // for each sum
    std::vector<std::pair<int,char> > cell_labels;  // for each var

    State state;

    for (int r = 0; r < size; ++r)
    {
        for (int c = 0; c < size; ++c)
        {
            assert(*p == ',');
            ++p;
            if (*p == 'X')
            {
                ++p;  // nothing to do
            }
            else
            if (*p == 'O' || (*p >= 'A' && *p <= 'J'))
            {
                if (*p != 'O')
                {
                    cell_labels.push_back(std::make_pair(state.vars, *p));
                }
                grid[r][c] = *p++;
                vars[r][c] = state.vars++;
            }
            else
            {
                assert(*p == '(');
                ++p;
                for (;;)
                {
                    char dir = *p++;
                    assert(dir == 'h' || dir == 'v');
                    while (*p >= 'A' && *p <= 'J')
                    {
                        (dir == 'h' ? hlabel : vlabel)[r][c] += *p++;
                    }
                    if (*p != ',') break;
                    ++p;
                }
                assert(*p == ')');
                ++p;
            }
        }
    }
    assert(*p == '\0');

    state.hgrp.assign(state.vars, -1);
    state.vgrp.assign(state.vars, -1);

    // Search for horizontal/vertical groups:
    for (int r = 1; r < size; ++r)
    {
        for (int c = 1; c < size; ++c)
        {
            if (grid[r][c] != 'X' && grid[r][c - 1] == 'X')
            {
                int d = c + 1;
                while (d < size && grid[r][d] != 'X') ++d;
                // std::cout << "H: " << r << ',' << c << '-' << d << ' ' << hlabel[r][c-1] << '\n';
                int g = state.grps++;
                sum_labels.push_back(hlabel[r][c-1]);
                state.mem.resize(g + 1);
                for ( ; c < d; ++c)
                {
                    state.hgrp[vars[r][c]] = g;
                    state.mem[g].push_back(vars[r][c]);
                }
            }
        }
    }
    for (int c = 1; c < size; ++c)
    {
        for (int r = 1; r < size; ++r)
        {
            if (grid[r][c] != 'X' && grid[r - 1][c] == 'X')
            {
                int s = r + 1;
                while (s < size && grid[s][c] != 'X') ++s;
                // std::cout << "V: " << r << '-' << s << ',' << c << ' ' << vlabel[r-1][c] << '\n';
                int g = state.grps++;
                sum_labels.push_back(vlabel[r-1][c]);
                state.mem.resize(g + 1);
                for ( ; r < s; ++r)
                {
                    state.vgrp[vars[r][c]] = g;
                    state.mem[g].push_back(vars[r][c]);
                }
            }
        }
    }
    state.sum.resize(state.grps);
    state.cand.assign(state.vars, (1<<9)-1);

    long long result = 0;
    int digits[10] = {0,1,2,3,4,5,6,7,8,9};
    int solutions = 0, n;
    std::vector<int> sol;
    do {
        // Assign fixed cells:
        for (size_t i = 0; i < cell_labels.size(); ++i)
        {
            int d = digits[cell_labels[i].second - 'A'];
            if (d == 0) goto skip;
            state.cand[cell_labels[i].first] = 1 << (d - 1);
        }

        // Assign sums:
        for (int i = 0; i < state.grps; ++i)
        {
            // Leading zeroes in clues are not allowed!
            if (digits[sum_labels[i][0] - 'A'] == 0) goto skip;

            state.sum[i] = 0;
            for ( std::string::const_iterator it = sum_labels[i].begin();
                  it != sum_labels[i].end(); ++it )
            {
                state.sum[i] = 10*state.sum[i] + digits[*it - 'A'];
            }

            // Check that the sum is at least possible for the corresponding
            // number of cells, n:
            int n = int(state.mem[i].size());
            assert(0 < n && n < 10);
            if ( state.sum[i] < n*(n + 1)/2 ||
                 state.sum[i] > 45 - (9 - n)*(10 - n)/2 ) goto skip;
        }

        n = state.solve(&sol);
        if (n > 0)
        {
            solutions += n;
#if 0
            std::cout << "n=" << n << " ";
            for (int i = 0; i < 10; ++i)
                std::cout << digits[i];
            std::cout << '\n';
            for (int r = 0; r < size; ++r)
            {
                for (int c = 0; c < size; ++c)
                {
                    if (grid[r][c] != 'X')
                        std::cout << 1 + log2(sol[vars[r][c]]);
                    else
                        std::cout << '.';
                }
                std::cout << '\n';
            }
#endif
            result = 0;
            for (int i = 0; i < 10; ++i)
            {
                result = 10*result  + digits[i];
            }
            std::cout << result << std::endl;
        }

    skip: ;

    } while (std::next_permutation(&digits[0], &digits[10]));

    assert(solutions == 1);
    return result;
}

int main()
{
    long long total = 0;
    std::string line;
    while (std::getline(std::cin, line))
    {
        total += solve(line.c_str());
    }
    std::cout << total << std::endl;
}
