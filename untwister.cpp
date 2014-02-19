/*
    Copyright Dan Petro, 2014

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <limits.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <atomic>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "ConsoleColors.h"
#include "Generator.h"

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::steady_clock;

//Pair of <seed, quality of fit>
typedef std::pair<uint32_t, double> Seed;
static std::vector<uint32_t> observedOutputs;
static const unsigned int ONE_YEAR = 31536000;

void Usage()
{
    std::cout << BOLD << "Untwister" << RESET << " - Recover PRNG seeds from observed values." << std::endl;
    std::cout << "\t-i <input_file> [-d <depth> ] [-r <rng_alg>] [-g <seed>] [-t <threads>]\n" << std::endl;
    std::cout << "\t-i <input_file>\n\t\tPath to file input file containing observed results of your RNG. The contents" << std::endl;
    std::cout << "\t\tare expected to be newline separated 32-bit integers. See test_input.txt for" << std::endl;
    std::cout << "\t\tan example." << std::endl;
    std::cout << "\t-d <depth>\n\t\tThe depth (default 1000) to inspect for each seed value when brute forcing." << std::endl;
    std::cout << "\t\tChoosing a higher depth value will make brute forcing take longer (linearly), but is " << std::endl;
    std::cout << "\t\trequired for cases where the generator has been used many times already." << std::endl;
    std::cout << "\t-r <rng_alg>\n\t\tThe RNG algorithm to use. Supported RNG algorithms:" << std::endl;
    std::cout << "\t\t\tmt19937 (default)" << std::endl;
    std::cout << "\t\t\tglibc-rand" << std::endl;
    std::cout << "\t-u\n\t\tUse bruteforce, but only for unix timestamp values within a range of +/- 1 " << std::endl;
    std::cout << "\t\tyear from the current time." << std::endl;
    std::cout << "\t-g <seed>\n\t\tGenerate a test set of random numbers from the given seed (at a random depth)" << std::endl;
    std::cout << "\t-t <threads>\n\t\tSpawn this many threads (default is 2)" << std::endl;
    std::cout << "" << std::endl;

}


/* Yeah lots of parameters, but such is the life of a thread */
void BruteForce(const unsigned int id, std::mutex& workingMutex, bool& isCompleted, std::vector<Seed>* answers,
        std::vector<uint32_t>* status, uint32_t startingSeed, uint32_t endingSeed, uint32_t depth, std::string rng)
{
    Generator generator(rng);

    for (uint32_t seedIndex = startingSeed; seedIndex <= endingSeed; ++seedIndex)
    {
        generator.Seed(seedIndex);

        uint32_t matchesFound = 0;
        for (uint32_t index = 0; index < depth; index++)
        {
            uint32_t nextRand = generator.Random();
            uint32_t observed = observedOutputs[matchesFound];

            if (observed == nextRand)
            {
                matchesFound++;
                if(matchesFound == observedOutputs.size())
                {
                    break;  // This seed is a winner if we get to the end
                }
            }
        }
        if (isCompleted)
        {
            break;  // Some other thread found the seed
        }
        status->at(id) = seedIndex - startingSeed;
        if (matchesFound == observedOutputs.size())
        {
            Seed seed = {seedIndex, 100};
            answers->push_back(seed);
            isCompleted = true;
        }
    }
}

void GenerateSample(uint32_t seed, uint32_t depth, std::string rng)
{
    Generator generator(rng);
    generator.Seed(seed);
    Generator distance_gen(rng);
    distance_gen.Seed(time(NULL));
    uint32_t distance = distance_gen.Random() % (depth - 10);

    //Burn a bunch of random numbers
    for (uint32_t i = 0; i < distance; i++)
    {
        generator.Random();
    }

    for (uint32_t i = 0; i < 10; i++)
    {
        std::cout << generator.Random() << std::endl;
    }
}

void StatusThread(std::vector<std::thread>& pool, bool& isCompleted, std::mutex& workingMutex,
        uint32_t totalWork, std::vector<uint32_t>* status)
{
    double percent = 0;
    steady_clock::time_point start = steady_clock::now();
    while (!isCompleted)
    {
        unsigned int sum = 0;
        for (unsigned int index = 0; index < status->size(); ++index)
        {
            sum += status->at(index);
        }
        percent = ((double) (sum) / (double) totalWork) * 100.0;
        isCompleted = (100.0 <= percent);
        printf("\r%s%sProgress: %3.2f%c", CLEAR.c_str(), DEBUG.c_str(), percent, 37);
        printf(" (%d seconds)", (int) duration_cast<seconds>(steady_clock::now() - start).count());
        std::cout.flush();
        std::this_thread::sleep_for(milliseconds(150));
    }
    printf("\r%s", CLEAR.c_str());
}

/* Divide X number of seeds among Y number of threads */
std::vector <uint32_t> DivisionOfLabor(uint32_t sizeOfWork, uint32_t numberOfWorkers)
{
    uint32_t work = sizeOfWork / numberOfWorkers;
    uint32_t leftover = sizeOfWork % numberOfWorkers;
    std::vector<uint32_t> labor(numberOfWorkers);
    for (uint32_t index = 0; index < numberOfWorkers; ++index)
    {
        if (0 < leftover)
        {
            labor.at(index) = work + 1;
            --leftover;
        }
        else
        {
            labor.at(index) = work;
        }
    }
    return labor;
}

void SpawnThreads(const unsigned int threads, std::vector <Seed>* answers, uint32_t lowerBoundSeed,
        uint32_t upperBoundSeed, uint32_t depth, std::string rng)
{
    std::mutex workingMutex;
    bool isCompleted = false;  // Flag to tell threads to stop working
    std::cout << INFO << "Spawning " << threads << " worker thread(s) ..." << std::endl;

    std::vector<std::thread> pool(threads);
    std::vector<uint32_t>* status = new std::vector<uint32_t>(threads);
    std::vector<uint32_t> labor = DivisionOfLabor(upperBoundSeed - lowerBoundSeed, threads);
    uint32_t startAt = lowerBoundSeed;
    for (unsigned int id = 0; id < threads; ++id)
    {
        uint32_t endAt = startAt + labor.at(id);
        pool[id] = std::thread(BruteForce, id, std::ref(workingMutex), std::ref(isCompleted), answers, status, startAt, endAt, depth, rng);
        startAt += labor.at(id);
    }
    StatusThread(std::ref(pool), std::ref(isCompleted), std::ref(workingMutex), upperBoundSeed - lowerBoundSeed, status);
    for (unsigned int index = 0; index < pool.size(); ++index)
    {
        pool[index].join();
    }
}

int main(int argc, char **argv)
{
    int c;
    unsigned int threads = 2;
    uint32_t lowerBoundSeed = 0;
    uint32_t upperBoundSeed = ULONG_MAX;
    uint32_t depth = 1000;
    std::string rng = "mt19937";
    uint32_t seed = 0;

    while ((c = getopt(argc, argv, "d:i:g:t:r:u")) != -1)
    {
        switch (c)
        {
            case 'g':
            {
                seed = strtoul(optarg, NULL, 10);
                break;
            }
            case 'u':
            {
                lowerBoundSeed = time(NULL) - ONE_YEAR;
                upperBoundSeed = time(NULL) + ONE_YEAR;
                break;
            }
            case 'r':
            {
                rng = optarg;
                break;
            }
            case 'd':
            {
                depth = strtoul(optarg, NULL, 10);
                if (depth == 0)
                {
                    std::cerr << WARN << "ERROR: Please enter a valid depth > 1" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }
            case 'i':
            {
                std::ifstream infile(optarg);
                if (!infile)
                {
                    std::cerr << WARN << "ERROR: File \"" << optarg << "\" not found" << std::endl;
                }
                std::string line;
                while (std::getline(infile, line))
                {
                    observedOutputs.push_back(strtoul(line.c_str(), NULL, 10));
                }
                break;
            }
            case 't':
            {
                threads = strtoul(optarg, NULL, 10);
                if (threads == 0)
                {
                    std::cerr << WARN << "ERROR: Please enter a valid number of threads > 0" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }
            case '?':
            {
                if (optopt == 'd')
                   fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                   fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                   fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                Usage();
                return EXIT_FAILURE;
            }
            default:
            {
                Usage();
                return EXIT_FAILURE;
            }
        }
    }

    if (seed != 0)
    {
        GenerateSample(seed, depth, rng);
        return EXIT_SUCCESS;
    }

    if (observedOutputs.empty())
    {
        Usage();
        std::cerr << WARN << "ERROR: No input numbers provided. Use -i <file> to provide a file" << std::endl;
        return EXIT_FAILURE;
    }

    std::vector <Seed>* answers = new std::vector <Seed>;
    steady_clock::time_point elapsed = steady_clock::now();
    SpawnThreads(threads, answers, lowerBoundSeed, upperBoundSeed, depth, rng);
    std::cout << INFO << "Completed in " << duration_cast<seconds>(steady_clock::now() - elapsed).count() << " second(s)" << std::endl;
    for (unsigned int index = 0; index < answers->size(); ++index)
    {
        std::cout << SUCCESS << "Seed is " << answers->at(index).first;
        std::cout << " with a confidence of " << answers->at(index).second << "%" << std::endl;
    }
    delete answers;
    return EXIT_SUCCESS;
}
