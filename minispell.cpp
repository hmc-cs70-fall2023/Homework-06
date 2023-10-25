#include "treestringset.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <algorithm>
#include <stdexcept>
#include <system_error>
#include <limits>
#include <cerrno>
#include <chrono>
#include <random>
#include <cstddef>

/**
 * \brief Fill a std::vector of words using content from a file.
 * \param words The vector to fill.
 * \param filename The file to read.
 * \param maxwords Maximum number of words to read
 */
void readWords(std::vector<std::string>& words, std::string filename,
               size_t maxwords) {
    std::cerr << "Reading words from " << filename << "...";
    try {
        std::ifstream in;
        in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        in.open(filename);
        in.exceptions(std::ifstream::badbit);
        std::string word;
        for (size_t i = 0; i < maxwords; ++i) {
            in >> word;
            if (!in.good()) {
                break;
            }
            words.push_back(word);
        }
        std::cerr << " done!\n";
    } catch (std::system_error& e) {
        // The error exceptions thrown by our standard library aren't very
        // meaningful, so we catch them and rethrow them with more information.
        throw std::system_error(
            std::make_error_code(std::errc(errno)),
            "Error reading '" + filename + "' (" + e.code().message() + +")");
    }
}

/**
 * \brief Fill a TreeStringSet of words using content from a vector of words.
 *        The order that the words are inserted is exactly the order in the
 *        vector.  The vector is emptied of words as part of this process.
 * \param dict The TreeStringSet to insert into.
 * \param words The vector from which the words will be taken.
 */
void insertAsRead(TreeStringSet& dict, std::vector<std::string>& words) {
    for (const auto& word : words) {
        dict.insert(word);
    }
    words.clear();
}

/**
 * \brief Fill a TreeStringSet of words using content from a vector of words.
 *        The words are inserted in a random order.  The vector is emptied of
 *        words as part of this process.
 * \param dict The TreeStringSet to insert into.
 * \param words The vector from which the words will be taken.
 */
void insertShuffled(TreeStringSet& dict, std::vector<std::string>& words) {
    std::random_device rdev;
    std::mt19937 prng{rdev()};  // This is only a 32-bit seed (weak!), but meh.
    std::shuffle(words.begin(), words.end(), prng);
    insertAsRead(dict, words);
}

/**
 * \brief This is a helper function, you can ignore it.  Bit it inserts the
 *        middle element, and the recurses to insert all the nodes to the left
 *        and to the right.  It'll build a balanced tree.
 */
void insertBalancedHelper(TreeStringSet& dict, std::vector<std::string>& words,
                          size_t start, size_t pastEnd) {
    if (start >= pastEnd) {
        return;
    }
    size_t size = pastEnd - start;
    size_t mid = start + size / 2;
    dict.insert(words[mid]);
    insertBalancedHelper(dict, words, start, mid);
    insertBalancedHelper(dict, words, mid + 1, pastEnd);
}

/**
 * \brief Fill a TreeStringSet of words using content from a vector of words.
 *        It builds a very balanced tree because it firsts sorts the data,
 *        recursively puts the mittle element at the root.
 * \param dict The TreeStringSet to insert into.
 * \param words The vector from which the words will be taken.
 */
void insertBalanced(TreeStringSet& dict, std::vector<std::string>& words) {
    std::sort(words.begin(), words.end());
    insertBalancedHelper(dict, words, 0, words.size());
    words.clear();
}

constexpr const char* DICT_FILE = "/home/student/data/smalldict.words";
constexpr const char* CHECK_FILE = "/home/student/data/ispell.words";


/**
 * \brief Print usage information for this program.
 * \param progname The name of the program.
 */
void usage(const char* progname) {
    std::cerr << "Usage: " << progname << " [options] [file-to-check ...]\n"
              << "Options:\n"
              << "  -h, --help             Print this message and exit.\n"
              << "  -f, --file-order       Insert words in the order they "
                 "appear (default).\n"
              << "  -s, --shuffled-order   Insert words in a random order.\n"
              << "  -b, --balanced-order   Insert words in a balanced order\n"
              << "  -n, --num-dict-words   Number of words to read from the "
                 "dictionary.\n"
              << "  -m, --num-check-words  Number of words to check for "
                 "spelling.\n"
              << "  -d, --dict-file        Use a different dictionary file.\n";
    std::cerr << "\nDefault dictionary file: " << DICT_FILE << std::endl;
    std::cerr << "Default file to check:   " << CHECK_FILE << std::endl;

}

/**
 * \brief Main program,
 */
int main(int argc, const char** argv) {
    // Defaults
    enum { AS_READ, SHUFFLED, BALANCED } insertionOrder = AS_READ;
    std::string dictFile = DICT_FILE;
    std::string fileToCheck = CHECK_FILE;

    size_t maxDictWords = std::numeric_limits<size_t>::max();
    size_t maxCheckWords = std::numeric_limits<size_t>::max();

    // Process Options and command-line arguments
    std::list<std::string> args(argv + 1, argv + argc);
    while (!args.empty() && args.front()[0] == '-') {
        const std::string& option = args.front();
        if (option == "-f" || option == "--file-order") {
            insertionOrder = AS_READ;
        } else if (option == "-s" || option == "--shuffled-order") {
            insertionOrder = SHUFFLED;
        } else if (option == "-b" || option == "--balanced-order") {
            insertionOrder = BALANCED;
        } else if (option == "-d" || option == "--dict-file") {
            args.pop_front();
            if (args.empty()) {
                std::cerr << "-d expects a filename\n";
                return 1;
            }
            dictFile = args.front();
        } else if (option == "-n" || option == "--num-dict-words"
                  || option == "-m" || option == "--num-check-words") {
            args.pop_front();
            if (args.empty()) {
                std::cerr << option << " expects a number\n";
                return 1;
            }
            try {
                size_t num = std::stoul(args.front());
                if (option == "-n" || option == "--num-dict-words") {
                    maxDictWords = num;
                } else {
                    maxCheckWords = num;
                }
            } catch (std::invalid_argument& e) {
                std::cerr << option << " expects a number\n";
                return 1;
            }
        } else if (option == "-h" || option == "--help") {
            usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << option << "\n";
            usage(argv[0]);
            return 1;
        }
        args.pop_front();
    }
    if (!args.empty()) {
        fileToCheck = args.front();
        args.pop_front();
        if (!args.empty()) {
            std::cerr << "extra argument(s), " << args.front() << std::endl;
            return 1;
        }
    }

    // Read the dictionary into a vector
    std::vector<std::string> words;
    readWords(words, dictFile, maxDictWords);

    // Create our search tree (and time how long it all takes)
    std::cerr << "Inserting into dictionary ";
    auto startTime = std::chrono::high_resolution_clock::now();

    TreeStringSet dict;
    if (insertionOrder == AS_READ) {
        std::cerr << "(in order read)...";
        insertAsRead(dict, words);
    } else if (insertionOrder == SHUFFLED) {
        std::cerr << "(in shuffled order)...";
        insertShuffled(dict, words);
    } else if (insertionOrder == BALANCED) {
        std::cerr << "(in perfect-balance order)...";
        insertBalanced(dict, words);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> secs = endTime - startTime;
    std::cerr << " done!\n";

    // Print some stats about the process

    std::cout << " - insertion took " << secs.count() << " seconds\n - ";
    dict.showStatistics(std::cout);
    auto iter = dict.begin();
    std::advance(iter, dict.size() / 2);
    std::cout << " - median word in dictionary: '" << *iter << "'\n\n";

    // Read some words to check against our dictionary (and time it)

    readWords(words, fileToCheck, maxCheckWords);
    std::cerr << "Looking up these words in the dictionary...";
    size_t inDict = 0;
    startTime = std::chrono::high_resolution_clock::now();
    for (const auto& word : words) {
        if (dict.exists(word)) {
            ++inDict;
        }
    }

    endTime = std::chrono::high_resolution_clock::now();
    secs = endTime - startTime;
    std::cerr << " done!\n";

    // Show some stats

    std::cout << " - looking up took " << secs.count() << " seconds\n - ";
    std::cout << words.size() << " words read, " << inDict
              << " in dictionary\n\n";

    return 0;
}
