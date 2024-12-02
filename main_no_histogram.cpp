#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <valarray> // This include is needed for declval
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <boost/math/distributions/chi_squared.hpp>
using namespace std;

class HashFunctionTester {
private:
    // Create an empty vector to store each word<string> in wordlist
    vector<string> words;

    // Define constant string that stores the relative file path to "words.txt".
    const string DICTIONARY_PATH = "./words.txt";

    // Define constant integer for table width
    const int TABLE_WIDTH = 70;

    // Helper function to convert signed char to unsigned
    uint16_t sanitizeChar(char c) {
        return static_cast<uint16_t>(static_cast<unsigned char>(c));
    }

   // Compute chi-square statistic for a given set of hashes
    float computeChiSquare(const vector<int>& hashes) {

        // Define integer to store total number of words
        int totalWords = words.size();

        // Calculate the expected value by dividing totalWords by 65536.0
        float expected = static_cast<float>(totalWords) / 65536.0;

        // Initialize chi-square statistic to 0
        float chiSquare = 0.0;

        // Loop through each count in the hashes vector
        for (int count : hashes) {
            // Update chi-square value by adding the contribution of the current count
            chiSquare += pow(count - expected, 2) / expected;
        }

        // Return the computed chi-square statistic as a float
        return chiSquare;
    }

    // Compute p-value based on the chi-square statistic
    float computePValue(float chiSquare) {
        
        // Create a chi-squared distribution with 65535 degrees of freedom using Boost library
        boost::math::chi_squared c2d(65535.0);

        // Return the cumulative distribution function (CDF) value for the given chi-square statistic
        return boost::math::cdf(c2d, chiSquare);
    }

    // Load dictionary from a file and store words in the 'words' vector
    void loadDictionary() {
        
        // Open the dictionary file for reading
        ifstream dictFile(DICTIONARY_PATH);

        // If the file could not be opened, throw an error
        if (!dictFile) {
            throw runtime_error("Could not open dictionary file");
        }
        
        // Define string variable to store each word from the dictionary file temporarily
        string word;

        // Read each line (word) from the dictionary file and add to the 'words' vector
        while (getline(dictFile, word)) {
            words.push_back(word);
        }
    }

    // Print a horizontal line of dashes of a specified length
    void printHorizontalLine(int length = -1) {

        // If length is not specified, use default value TABLE_WIDTH
        if (length == -1) {
            length = TABLE_WIDTH;
        }

        // Print horizontal line to console
        cout << string(length - 1, '-') << endl;


    }

// Public constructor for the HashFunctionTester class
public:


    // Initializes an object of HashFunctionTester
    HashFunctionTester() {

        // Calls the loadDictionary function to populate the 'words' vector with words
        loadDictionary(); 
    }

    // Function to test a hash function and print a histogram of hash results
    // Accepts the name of the hash function and the hash function itself as arguments
    void testHashFunction(const string& name, 
        const function<uint16_t(const string&)>& hashFunc) {

        // Create a vector to store the hash results, initialized to 0 with a size of 65536
        vector<int> hashes(65536, 0);

        // Iterate through each word in the 'words' vector
        for (const auto& word : words) {

            // Define a 16-bit unsigned integer for the resulting hash of the current word
            uint16_t h = hashFunc(word);

            // Increment the corresponding hash bucket in the 'hashes' vector
            hashes[h]++;
        }

        // Compute the chi-square statistic based on the hash distribution
        float chiSquare = computeChiSquare(hashes);

        // Compute the p-value from the chi-square statistic
        float pValue = computePValue(chiSquare);

        // Print a horizontal line as a divider between the tests
        printHorizontalLine(TABLE_WIDTH);

        // Print the hash function's name
        cout << name << " Hash:" << endl;

        // Print another horizontal line of half the width
        printHorizontalLine(TABLE_WIDTH / 2);

        // Print the chi-square value
        cout << "Chi-Square: " << chiSquare << endl;

        // Print the p-value
        cout << "P-Value: " << pValue << endl;   

    }

    // Function to run all hash function tests
    void runAllTests() {

        // String Length Hash Test
        // This test hashes a string based on its length modulo 65536
        testHashFunction("String Length", [](const string& word) {
            return word.length() % 65536;  // Return the length of the string modulo 65536
        });

        // First Character Hash Test
        // This test hashes a string based on the first character (sanitized) modulo 65536
        testHashFunction("First Character", [this](const string& word) {
            return word.empty() ? 0 : sanitizeChar(word[0]) % 65536;  // If empty, return 0, else sanitize and hash the first character
        });

        // Additive Checksum Hash Test
        // This test computes a checksum by adding sanitized character values modulo 65536
        testHashFunction("Additive Checksum", [this](const string& word) {
            uint16_t h = 0;  // Initialize checksum value
            for (char c : word) {
                h = (h + sanitizeChar(c)) % 65536;  // Add sanitized char to checksum, taking modulo 65536
            }
            return h;  // Return the checksum value
        });

        // Remainder Hash Test
        // This test computes a hash by multiplying the current hash by 31, adding the sanitized character, and taking the remainder modulo 65413
        testHashFunction("Remainder", [this](const string& word) {
            const uint16_t m = 65413;  // Define modulus value
            uint16_t h = 0;  // Initialize hash value
            for (char c : word) {
                h = (h * 31 + sanitizeChar(c)) % m;  // Update hash by multiplying by 31 and adding sanitized character
            }
            return h;  // Return the final hash value
        });

        // Multiplicative Hash Test
        // This test uses a multiplicative approach with a constant factor (0.6180339887) to generate the hash
        testHashFunction("Multiplicative", [this](const string& word) {
            double h = 0.0;  // Initialize hash value as a floating-point number
            for (char c : word) {
                h = fmod(h * 0.6180339887 + sanitizeChar(c), 1.0);  // Update hash using floating-point multiplication and sanitize char
            }
            return static_cast<uint16_t>(h * 65536);  // Scale the result to a uint16_t and return
        });

        // Standard Library Hash Test
        // This test uses the standard C++ hash function to hash the string and takes modulo 65536
        testHashFunction("Standard Library", [](const string& word) {
            return hash<string>{}(word) % 65536;  // Use the standard C++ hash function and return modulo 65536
        });
    }

};


// Main function
int main() {
    try {
        // Create a HashFunctionTester object and run all hash function tests
        HashFunctionTester tester;
        tester.runAllTests();
    }
    catch (const exception& e) {
        // If an exception occurs, print the error message
        cerr << "Error: " << e.what() << endl;
        return 1;  // Return 1 to indicate an error
    }

    return 0;  // Return 0 to indicate successful execution
}
