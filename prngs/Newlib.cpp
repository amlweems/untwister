
#include "Newlib.h"
#include "../ConsoleColors.h"
#include <climits>

Newlib::Newlib()
{
    next = 0;
}

Newlib::~Newlib() {}

const std::string Newlib::getName()
{
    return NEWLIB;
}

void Newlib::seed(int64_t value)
{
    next = (uint32_t)value;
}

int64_t Newlib::getSeed()
{
    return next;
}

uint32_t Newlib::random(void)
{
    /* This multiplier was obtained from Knuth, D.E., "The Art of
     Computer Programming," Vol 2, Seminumerical Algorithms, Third
     Edition, Addison-Wesley, 1998, p. 106 (line 26) & p. 108 */
    next = next * 6364136223846793005LL + 1;
    return (int)((next >> 32) & RAND_MAX);
}

uint32_t Newlib::getStateSize(void)
{
    return Newlib_STATE_SIZE;
}

void Newlib::setState(std::vector<uint32_t> inState)
{
    next = inState[0] << 32;
    next += inState[1];
}

std::vector<uint32_t> Newlib::getState(void)
{
    uint32_t hi, low;
    hi = next >> 32;
    low = next & 0xffffffff;
    std::vector<uint32_t> ret = { hi, low };
    return ret;
}

std::vector<uint32_t> Newlib::predictForward(uint32_t)
{
    std::vector<uint32_t> ret;
    //TODO
    return ret;
}

std::vector<uint32_t> Newlib::predictBackward(uint32_t)
{
    std::vector<uint32_t> ret;
    //TODO
    return ret;
}


bool Newlib::reverseToSeed(int64_t *outSeed, uint32_t depth)
{
    //TODO
    return false;
}

void Newlib::tune(std::vector<uint32_t> evidenceForward, std::vector<uint32_t> evidenceBackward)
{
    //TODO
}

void Newlib::setEvidence(std::vector<uint32_t>)
{

}

void Newlib::setBounds(uint32_t min, uint32_t max)
{
    //Setting bounds is unsupported in C++ rand, so do nothing here. In fact, this should not get called.
}

int64_t Newlib::getMinSeed()
{
    return 0;
}

int64_t Newlib::getMaxSeed()
{
    return UINT_MAX;
}
