#include "Puzzle.h"
#include <memory>

const char * const digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
                            "ghijklmnopqrstuvwxyz0123456789-_";

static char decode(char c)
{
    // Asumes ASCII character set
    if(c >= 'A' && c <= 'Z')
        return c - 'A';
    if(c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if(c >= '0' && c <= '9')
        return c - '0' + 52;
    if(c == '-')
        return 62;
    if(c == '_')
        return 63;
    return char(-1);
}

static void append_int(std::string &str, int i)
{
    int n = 1;
    while(i/n >= 32) n *= 32;
    while(n > 1)
    {
        str += digits[32 | (i/n)];
        i %= n;
        n /= 32;
    }
    str += digits[i];
}

static int read_int(std::vector<char> &bytes, size_t &pos)
{
    int i = 0;
    while(pos < bytes.size())
    {
        int v = bytes[pos++];
        if(v&32)
            i = 32*i + (v&31);
        else
            return 32*i + v;
    }
    return -1;
}

std::string Grid::toString() const
{
    std::string str;
    append_int(str, height > 0 ? height - 1 : 0);
    append_int(str, width > 0  ? width - 1 : 0);
    int bit = 0, value = 0;
    for(int r = 1; r < height; ++r)
        for(int c = 1; c < width; ++c)
        {
            if((*this)[r][c] >= 0)
                value |= 1<<bit;
            if(++bit == 6)
            {
                str += digits[value];
                bit = value = 0;
            }
        }
    if(bit)
        str += digits[value];
    return str;
}

std::string State::grpsToString() const
{
    std::string str;
    str.resize(grps);
    for(int g = 0; g < grps; ++g)
        str[g] = digits[(sum[g] >= 0 && sum[g] < 64) ? sum[g] : 0];
    return str;
}

std::string State::varsToString(bool candidates) const
{
    std::string str;
    str.reserve((vars*3 + 1)/2);
    int bits = 0, data = 0;
    for(int v = 0; v < vars; ++v)
    {
        if(candidates || digit(cand[v]))
            data |= (cand[v]&511) << bits;
        bits += 9;

        while(bits >= 6)
        {
            str += digits[data&63];
            data >>= 6;
            bits -= 6;
        }
    }
    if(bits)
        str += digits[data&63];

    return str;
}

std::string Puzzle::toString(ExportOptions options) const
{
    std::string str = grid.toString();
    if(options >= exportGridSums)
        str += state.grpsToString();
    if(options >= exportGridSumsDigits)
        str += state.varsToString(options >= exportGridSumsCandidates);
    return str;
}

Puzzle* Puzzle::fromString(const std::string &str)
{
    std::vector<char> bytes;
    bytes.reserve(str.size());
    for(std::string::const_iterator i = str.begin(); i != str.end(); ++i)
    {
        char c = decode(*i);
        if(c == char(-1))
            return 0;
        bytes.push_back(c);
    }

    size_t pos = 0;
    std::auto_ptr<Puzzle> puzzle(new Puzzle);
    Grid &grid = puzzle->grid;
    State &state = puzzle->state;

    // Parse size
    grid.height = 1 + read_int(bytes, pos);
    grid.width  = 1 + read_int(bytes, pos);
    if(grid.height <= 0 || grid.width <= 0)
        return 0;

    // Parse grid
    int grid_bytes = ((grid.width - 1)*(grid.height - 1) + 5)/6;
    grid.vars.resize(grid.width * grid.height, - 1);
    if(pos + grid_bytes > bytes.size())
        return 0;
    int r = 1, c = 1, hgrp = -1;
    std::vector<int> vgrp(grid.width, -1);
    while(grid_bytes--)
    {
        int byte = bytes[pos++];
        for(int bit = 0; bit < 6; ++bit)
        {
            if((byte>>bit)&1)
            {
                if(hgrp == -1)
                {
                    hgrp = state.grps++;
                    state.mem.resize(state.grps);
                }
                if(vgrp[c] == -1)
                {
                    vgrp[c] = state.grps++;
                    state.mem.resize(state.grps);
                }
                state.hgrp.push_back(hgrp);
                state.vgrp.push_back(vgrp[c]);
                state.cand.push_back(0);
                state.mem[hgrp].push_back(state.vars);
                state.mem[vgrp[c]].push_back(state.vars);
                grid[r][c] = state.vars++;
            }
            else
            {
                hgrp = vgrp[c] = -1;
            }

            if(++c == grid.width)
            {
                c = 1;
                hgrp = -1;
                if(++r == grid.height)
                    break;
            }
        }
    }

    // Parse sums
    state.sum.resize(state.grps);
    if(pos + state.grps > bytes.size())
        return puzzle.release();
    for(int g = 0; g < state.grps; ++g)
        state.sum[g] = bytes[pos++];

    // Parse candidates
    if(pos + (3*state.vars+1)/2 > bytes.size())
        return puzzle.release();
    for(int v = 0, avail = 0, data = 0; v < state.vars; ++v)
    {
        if(!avail)
        {
            data  = bytes[pos++] <<  0;
            data |= bytes[pos++] <<  6;
            data |= bytes[pos++] << 12;
            avail = 2;
        }
        state.cand[v] = data&511;
        data >>= 9;
        --avail;
    }

    return puzzle.release();
}
