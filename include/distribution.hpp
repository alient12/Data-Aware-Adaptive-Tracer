#ifndef DISTRIBUTION_HPP
#define DISTRIBUTION_HPP

#include <iostream>
#include <map>
#include <tuple>
#include <vector>
#include <cmath>
#include <limits>

class DistributionCalculator {
public:
    void computeDistribution(const std::map<std::tuple<int, std::string, int, long long>, int>& argCount);
    void printDistributions() const;
    std::string generateRareArgCondition(int sampler_id, const std::string& func_name, int arg_number) const;

private:
    std::map<std::tuple<int, std::string, int>, std::vector<std::pair<std::pair<long long, long long>, int>>> distributions;
    int calculateNumBins(const std::vector<long long>& values);
};

#endif // DISTRIBUTION_HPP