#include "sums.h"
#include <algorithm>
#include <vector>
#include <utility>
#include <iostream> //DEBUG

std::vector<int> cands[46][10];

void initialize();

static struct Initializer {
    Initializer() { initialize(); }
} initializer;

void collect(int sum = 0, int cnt = 0, int cand = 0, int x = 0)
{
    if(x < 9)
    {
        collect(sum, cnt, cand, x + 1);
        collect(sum + x + 1, cnt + 1, cand|(1<<x), x + 1);
    }
    else
    {
        cands[sum][cnt].push_back(cand);
    }
}

void initialize()
{
    collect();
}

int candidatesForSum(int sum, int size, int given)
{
    if(sum < 0 || sum > 45 || size < 0 || size > 9)
        return 0;

    int r = 0;
    for( std::vector<int>::const_iterator i = cands[sum][size].begin();
         i != cands[sum][size].end(); ++i )
    {
        if((*i&given) == given)
            r |= *i;
    }
    return r^given;
}
