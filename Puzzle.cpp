#include "Puzzle.h"
#include "sums.h"
#include <memory>
#include <algorithm>

int log2(int cand)
{
    int r = 0;
    while(cand >>= 1)
        ++r;
    return r;
}

void Grid::crop()
{
    int minr = 999999999, minc = 999999999, maxr = -1, maxc = -1;
    for(int r = 0; r < height; ++r)
        for(int c = 0; c < width; ++c)
            if(vars[r*width + c] >= 0)
            {
                minr = std::min(minr, r);
                minc = std::min(minc, c);
                maxr = std::max(maxr, r);
                maxc = std::max(maxc, c);
            }

    if(maxr < 0)
        return; // No empty cells (we could crop, though)

    --minc, --minr;
    int new_height = maxr - minr + 1, new_width = maxc - minc + 1;
    std::vector<int> new_vars;
    new_vars.reserve(new_width*new_height);
    for(int r = 0; r < new_height; ++r)
        for(int c = 0; c < new_width; ++c)
            new_vars.push_back(vars[(r + minr)*width + (c + minc)]);

    height = new_height, width = new_width;
    vars.swap(new_vars);
}

void Grid::enlarge(int top, int bottom, int left, int right)
{
    if(top < 0) top = 0;
    if(bottom < 0) bottom = 0;
    if(left < 0) left = 0;
    if(right < 0) right = 0;

    int new_height = height + top + bottom, new_width = width + left + right;
    std::vector<int> new_vars(new_width*new_height, - 1);
    for(int r = 0; r < height; ++r)
        for(int c = 0; c < width; ++c)
            new_vars[(r + top)*new_width + (c + left)] = vars[r*width + c];
    height = new_height, width = new_width;
    vars.swap(new_vars);
}

void State::clearDigits()
{
    for(int v = 0; v < vars; ++v)
        if(cand[v] && single(cand[v]))
            cand[v] = 0;
}

void State::clearCandidates()
{
    for(int v = 0; v < vars; ++v)
        if(cand[v] && !single(cand[v]))
            cand[v] = 0;
}

void State::clearSums()
{
    for(int g = 0; g < grps; ++g)
        sum[g] = 0;
}

int State::givens(int grp, int without) const
{
    int r = 0;
    for( std::vector<int>::const_iterator i = mem[grp].begin();
         i != mem[grp].end(); ++i )
    {
        if(*i != without && digit(cand[*i]))
            r |= cand[*i];
    }
    return r;
}

int State::cands(int grp, int without) const
{
    if(sum[grp])
        return candidatesForSum(sum[grp], mem[grp].size(), givens(grp, without));
    else
        return ((1<<9)-1) & ~givens(grp, without);
}

void State::calculateCandidates()
{
    std::vector<bool> fixed(vars, false);
    for(int v = 0; v < vars; ++v)
        if(digit(cand[v]))
            fixed[v] = true;
        else
            cand[v] = (1<<9)-1;

    bool modified;
    do {
        modified = false;
        for(int v = 0; v < vars; ++v)
            if(!fixed[v])
            {
                int c = cands(hgrp[v], v) & ~givens(hgrp[v],v) &
                        cands(vgrp[v], v) & ~givens(vgrp[v],v);
                if((cand[v]&c) != cand[v])
                {
                    modified = true;
                    cand[v] &= c;
                }
            }
    } while(modified);
}

void State::calculateSums()
{
    for(int g = 0; g < grps; ++g)
    {
        if(sum[g] != 0)
            continue;

        for( std::vector<int>::const_iterator i = mem[g].begin();
             i != mem[g].end(); ++i )
        {
            if(digit(cand[*i]))
            {
                sum[g] += 1 + log2(cand[*i]);
            }
            else
            {
                sum[g] = 0;
                break;
            }
        }
    }
}
