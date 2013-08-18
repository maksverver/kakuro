#include "Puzzle.h"

static int bit_count[512] = { };

int State::search(std::vector<int> *solution)
{
    calculateCandidates();

    int min_bits = 10, best = -1;
    for(int v = 0; v < vars; ++v)
    {
        int bits = bit_count[cand[v]];
        if(bits == 0)
            return 0;
        if(bits > 1 && bits < min_bits)
        {
            min_bits = bits;
            best     = v;
        }
    }

    if(best == -1)
    {
        if(solution)
            *solution = cand;
        return 1;
    }

    // Backtrack on 'best' variable
    std::vector<int> old_cand = cand;
    int r = 0;
    for(int x = 1; x != (1<<9); x<<=1)
        if(old_cand[best]&x)
        {
            cand[best] = x;
            r += search(solution);
            cand = old_cand;
            if(r > 1)
                break;
        }

    return r;
}

int State::solve(std::vector<int> *solution)
{
    if(bit_count[1] == 0)
    {
        // Initialize bit_count array
        for(int n = 1; n < 512; ++n)
        {
            int x = n;
            do { ++bit_count[n]; } while(x &= x- 1);
        }
    }

    // Check validity of filled-in digits
    for(int g = 0; g < grps; ++g)
    {
        int used = 0, s = 0;
        for( std::vector<int>::const_iterator i = mem[g].begin();
            i != mem[g].end(); ++i )
        {
            if(digit(cand[*i]))
            {
                if(used & cand[*i])
                    return -1;
                s += 1 + log2(cand[*i]);
                used |= cand[*i];
            }
        }
        if(sum[g] && bit_count[used] == int(mem[g].size()) && s != sum[g])
            return -1;
    }

    // Try to solve the grid, but keep the current candidates
    std::vector<int> backup = cand;
    //clearDigits();
    int n = search(solution);
    cand.swap(backup);
    return n;
}
