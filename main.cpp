#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <valarray> // This include is needed for std::declval
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <boost/math/distributions/chi_squared.hpp>

class HashFunctionTester {
private:
    std::vector<std::string> words;
    const std::string DICTIONARY_PATH = "./words.txt";
    const int HISTOGRAM_WIDTH = 70;
    const int HISTOGRAM_HEIGHT = 10;

    // Helper function to convert signed char to unsigned
    uint16_t sanitizeChar(char c) {
        return static_cast<uint16_t>(static_cast<unsigned char>(c));
    }

    // Compute chi-square statistic
    float computeChiSquare(const std::vector<int>& hashes) {
        int totalWords = words.size();
        float expected = static_cast<float>(totalWords) / 65536.0;
        
        float chiSquare = 0.0;
        for (int count : hashes) {
            chiSquare += std::pow(count - expected, 2) / expected;
        }
        
        return chiSquare;
    }

    // Compute p-value using Boost chi-squared distribution
    float computePValue(float chiSquare) {
        boost::math::chi_squared c2d(65535.0);
        return boost::math::cdf(c2d, chiSquare);
    }

    void loadDictionary() {
        std::ifstream dictFile(DICTIONARY_PATH);
        if (!dictFile) {
            throw std::runtime_error("Could not open dictionary file");
        }
        
        std::string word;
        while (std::getline(dictFile, word)) {
            words.push_back(word);
        }
    }

    void printHorizontalLine(int length = -1) {
        if (length == -1) {
            length = HISTOGRAM_WIDTH;
        }
        std::cout << std::string(length - 1, '-') << std::endl;
    }
    void printHistogram(const std::vector<int>& hashes) {
        // Find max and min values
        int maxCount = *std::max_element(hashes.begin(), hashes.end());
        int minCount = *std::min_element(hashes.begin(), hashes.end());

        // Divide hash into 16 segments
        std::vector<int> segmentMaxCounts(16, 0);
        for (size_t i = 0; i < 16; ++i) {
            size_t start = i * 4096;
            size_t end = std::min((i + 1) * 4096, hashes.size());
            
            auto segmentMax = *std::max_element(
                hashes.begin() + start, 
                hashes.begin() + end
            );
            segmentMaxCounts[i] = segmentMax;
        }

        // Normalize segment max counts
        std::vector<int> normalizedSegments(16);
        for (size_t i = 0; i < 16; ++i) {
            normalizedSegments[i] = std::round(
                ((segmentMaxCounts[i] - minCount) / 
                 static_cast<double>(maxCount - minCount)) * 
                (HISTOGRAM_HEIGHT - 1)
            );
        }

        // Print histogram
        std::cout << "Histogram (Hashes Distribution):" << std::endl;
        printHorizontalLine(HISTOGRAM_WIDTH);
        
        // Print vertically
        for (int row = HISTOGRAM_HEIGHT - 1; row >= 0; --row) {
            std::cout << "|";
            for (size_t col = 0; col < 16; ++col) {
                std::cout << (normalizedSegments[col] >= row ? "   #" : "    ");
            }
            std::cout << "   |" << std::endl;
        }

        // Bottom line
       printHorizontalLine(HISTOGRAM_WIDTH);
        
        // X-axis labels
        std::cout << " ";
        for (int i = 0; i < 16; ++i) {
            std::cout << std::setw(4) << i;
        }
        std::cout << std::endl;
    }

public:
    HashFunctionTester() {
        loadDictionary();
    }

    // Hash function tests with histogram printing
    void testHashFunction(const std::string& name, 
        const std::function<uint16_t(const std::string&)>& hashFunc) {
        std::vector<int> hashes(65536, 0);
        
        for (const auto& word : words) {
            uint16_t h = hashFunc(word);
            hashes[h]++;
        }
        
        float chiSquare = computeChiSquare(hashes);
        float pValue = computePValue(chiSquare);
        printHorizontalLine(HISTOGRAM_WIDTH);
        std::cout << name << " Hash:" << std::endl;
        printHorizontalLine(HISTOGRAM_WIDTH / 2);
        std::cout << "Chi-Square: " << chiSquare << std::endl;
        std::cout << "P-Value: " << pValue << std::endl;   
        printHistogram(hashes);
    }

    void runAllTests() {
        // String Length Hash
        testHashFunction("String Length", [](const std::string& word) {
            return word.length() % 65536;
        });

        // First Character Hash
        testHashFunction("First Character", [this](const std::string& word) {
            return word.empty() ? 0 : sanitizeChar(word[0]) % 65536;
        });

        // Additive Checksum Hash
        testHashFunction("Additive Checksum", [this](const std::string& word) {
            uint16_t h = 0;
            for (char c : word) {
                h = (h + sanitizeChar(c)) % 65536;
            }
            return h;
        });

        // Remainder Hash
        testHashFunction("Remainder", [this](const std::string& word) {
            const uint16_t m = 65413;
            uint16_t h = 0;
            for (char c : word) {
                h = (h * 31 + sanitizeChar(c)) % m;
            }
            return h;
        });

        // Multiplicative Hash
        testHashFunction("Multiplicative", [this](const std::string& word) {
            double h = 0.0;
            for (char c : word) {
                h = fmod(h * 0.6180339887 + sanitizeChar(c), 1.0);
            }
            return static_cast<uint16_t>(h * 65536);
        });

        // Standard Library Hash
        testHashFunction("Standard Library", [](const std::string& word) {
            return std::hash<std::string>{}(word) % 65536;
        });
    }
};

int main() {
    try {
        HashFunctionTester tester;
        tester.runAllTests();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}