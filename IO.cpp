#include <fstream>
#include <iostream>
#include <sstream>

#include "IO.h"

using namespace std;

IO::IO(const std::string& _name)
    : name(_name) {}

bool IO::write(byte* data, const unsigned& suffix, unsigned size)
{
    std::stringstream ss;
    ss << name << suffix;
    fstream file(ss.str(), ios::out | ios::binary);
    if(!file.is_open())
        return false;
    file.write(data, size);
    file.close();
    return true;
}

bool IO::read(byte* data, const unsigned& suffix, unsigned size)
{
    std::stringstream ss;
    ss << name << suffix;
    fstream file(ss.str(), ios::in | ios::binary);
    if(!file.is_open())
        return false;
    file.read(data, size);
    file.close();
    return true;
}
