#ifndef _IO_H_
#define _IO_H_

typedef char byte;

class IO
{
private:
    const std::string name;

public:
    IO(const std::string& _name);
    bool write(byte* data, const unsigned& suffix, unsigned size);
    bool read(byte* data, const unsigned& suffix, unsigned size);
};

#endif // _IO_H_
