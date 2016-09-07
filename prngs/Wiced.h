#ifndef WICED_H_
#define WICED_H_

#include <random>
#include "PRNG.h"

static const std::string WICED = "wiced";
static const uint32_t Wiced_STATE_SIZE = 2;

class Wiced: public PRNG
{
public:
    Wiced();
    virtual ~Wiced();

    const std::string getName(void);
    void seed(int64_t value);
    int64_t getSeed(void);
    uint32_t random(void);

    uint32_t getStateSize(void);
    void setState(std::vector<uint32_t>);
    std::vector<uint32_t> getState(void);

    void setEvidence(std::vector<uint32_t>);

    std::vector<uint32_t> predictForward(uint32_t);
    std::vector<uint32_t> predictBackward(uint32_t);
    void tune(std::vector<uint32_t>, std::vector<uint32_t>);

    bool reverseToSeed(int64_t *, uint32_t);
    void setBounds(uint32_t, uint32_t);

    int64_t getMinSeed();
    int64_t getMaxSeed();

private:
    uint64_t next;
};

#endif /* NEWLIB_H_ */
