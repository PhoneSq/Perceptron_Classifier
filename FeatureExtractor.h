#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cctype>

// turn a text file into a feature vector: counts per keyword
inline std::vector<double> extractFeatures(const std::string& filename,
    const std::vector<std::string>& keywords)
{
    std::ifstream file(filename);
    std::vector<double> features(keywords.size(), 0.0);

    if (!file.is_open())   // ✅ safer check
        return features;

    std::string token;
    while (file >> token)
    {
        // lowercase safely (handles numbers & punctuation fine)
        std::transform(token.begin(), token.end(), token.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        // strip trailing punctuation
        while (!token.empty() && std::ispunct(static_cast<unsigned char>(token.back())))
            token.pop_back();

        // check against keywords
        for (size_t i = 0; i < keywords.size(); ++i)
        {
            if (!keywords[i].empty() && token.find(keywords[i]) != std::string::npos)
                features[i] += 1.0;
        }
    }

    return features;
}