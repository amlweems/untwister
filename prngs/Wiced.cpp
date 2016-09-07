
#include "Wiced.h"
#include "../ConsoleColors.h"
#include <climits>

Wiced::Wiced()
{
    next = 0;
}

Wiced::~Wiced() {}

const std::string Wiced::getName()
{
    return WICED;
}

void Wiced::seed(int64_t value)
{
    next = (uint32_t)value;
}

int64_t Wiced::getSeed()
{
    return next;
}

uint32_t Wiced::random(void)
{
    /* This multiplier was obtained from Knuth, D.E., "The Art of
     Computer Programming," Vol 2, Seminumerical Algorithms, Third
     Edition, Addison-Wesley, 1998, p. 106 (line 26) & p. 108 */
    next = next * 6364136223846793005LL + 1;
    uint32_t ret = (int)((next >> 32) & RAND_MAX);
    return (uint8_t) ret;
}

uint32_t Wiced::getStateSize(void)
{
    return Wiced_STATE_SIZE;
}

void Wiced::setState(std::vector<uint32_t> inState)
{
    next = inState[0] << 32;
    next += inState[1];
}

std::vector<uint32_t> Wiced::getState(void)
{
    uint32_t hi, low;
    hi = next >> 32;
    low = next & 0xffffffff;
    std::vector<uint32_t> ret = { hi, low };
    return ret;
}

std::vector<uint32_t> Wiced::predictForward(uint32_t)
{
    std::vector<uint32_t> ret;
    //TODO
    return ret;
}

std::vector<uint32_t> Wiced::predictBackward(uint32_t)
{
    std::vector<uint32_t> ret;
    //TODO
    return ret;
}


bool Wiced::reverseToSeed(int64_t *outSeed, uint32_t depth)
{
    //TODO
    return false;
}

void Wiced::tune(std::vector<uint32_t> evidenceForward, std::vector<uint32_t> evidenceBackward)
{
    //TODO
}

void Wiced::setEvidence(std::vector<uint32_t>)
{

}

void Wiced::setBounds(uint32_t min, uint32_t max)
{
    //Setting bounds is unsupported in C++ rand, so do nothing here. In fact, this should not get called.
}

int64_t Wiced::getMinSeed()
{
    return 0;
}

int64_t Wiced::getMaxSeed()
{
    return UINT_MAX;
}
