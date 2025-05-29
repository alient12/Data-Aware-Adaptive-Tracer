#include "distribution.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>

// Compute the number of bins using Sturges' formula
int DistributionCalculator::calculateNumBins(const std::vector<long long>& values) {
    int numBins = std::log2(values.size()) + 1;
    return std::max(numBins, 1); // Ensure at least one bin
}

void DistributionCalculator::computeDistribution(const std::map<std::tuple<int, std::string, int, long long>, int>& argCount) {
    std::map<std::tuple<int, std::string, int>, std::vector<long long>> groupedValues;

    // Group values by (sampler_id, function_name, arg_number), considering repetition count
    for (const auto& entry : argCount) {
        std::tuple<int, std::string, int> groupKey = { std::get<0>(entry.first), std::get<1>(entry.first), std::get<2>(entry.first) };
        // Add the value entry.second times
        for (int i = 0; i < entry.second; ++i) {
            groupedValues[groupKey].push_back(std::get<3>(entry.first));
        }
    }

    // Compute bins for each group using Sturges' formula
    for (const auto& group : groupedValues) {
        int numBins = calculateNumBins(group.second);
        long long minValue = *std::min_element(group.second.begin(), group.second.end());
        long long maxValue = *std::max_element(group.second.begin(), group.second.end());

        double binWidth = static_cast<double>(maxValue - minValue) / numBins;

        // Generate bin boundaries
        std::vector<std::pair<long long, long long>> bins;
        for (int i = 0; i < numBins; ++i) {
            long long start = minValue + i * binWidth;
            long long end = (i == numBins - 1) ? maxValue : start + binWidth;
            bins.emplace_back(start, end);
        }

        // Assign values to bins, ensuring last bin includes max value
        std::map<std::pair<long long, long long>, int> binCounts;
        for (const auto& value : group.second) {
            for (size_t i = 0; i < bins.size(); ++i) {
                const auto& bin = bins[i];
                if ((i < bins.size() - 1 && value >= bin.first && value < bin.second) ||
                    (i == bins.size() - 1 && value >= bin.first && value <= bin.second)) {
                    binCounts[bin] += 1;
                    break;
                }
            }
        }

        // Store results in distribution map
        std::vector<std::pair<std::pair<long long, long long>, int>> binResults;
        for (const auto& binEntry : binCounts) {
            binResults.push_back({ binEntry.first, binEntry.second });
        }

        distributions[group.first] = binResults;
    }
}

void DistributionCalculator::printDistributions() const {
    for (const auto& group : distributions) {
        std::cout << "sampler" << std::get<0>(group.first) << "_" << std::get<1>(group.first)
                  << "_arg" << std::get<2>(group.first) << " Distribution:\n";

        for (const auto& binData : group.second) {
            std::cout << "  (" << binData.first.first << ", " << binData.first.second << "): " << binData.second << "\n";
        }
        std::cout << std::endl;
    }
}

std::string DistributionCalculator::generateRareArgCondition(int sampler_id, const std::string& func_name, int arg_number) const {
    std::tuple<int, std::string, int> key = { sampler_id, func_name, arg_number };

    // Check if we have distributions for this key
    auto it = distributions.find(key);
    if (it == distributions.end() || it->second.empty()) {
        return ""; // No data available
    }

    const auto& binResults = it->second;
    int totalBins = binResults.size();

    // Determine how many bins are rare (10%, rounded up)
    int numRareBins = std::ceil(totalBins * 0.1);
    if (numRareBins == 0) numRareBins = 1;

    // Sort bins by population (count) ascending
    std::vector<std::pair<std::pair<long long, long long>, int>> sortedBins = binResults;
    std::sort(sortedBins.begin(), sortedBins.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
    });

    std::ostringstream condition;
    std::string argVarName = "arg" + std::to_string(arg_number);

    // Collect conditions from the rarest bins
    for (int i = 0; i < numRareBins; ++i) {
        long long start = sortedBins[i].first.first;
        long long end = sortedBins[i].first.second;

        if (start == end) {
            condition << "(" << argVarName << " == " << start << ")";
        } else {
            condition << "(" << argVarName << " >= " << start << " && " << argVarName << " <= " << end << ")";
        }

        if (i < numRareBins - 1) {
            condition << " || ";
        }
    }

    return condition.str();
}