#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <bitset>
#include <array>
#include <sstream>

#define NUMBER_OF_LETTERS 26
#define WORD_LENGTH 5
#define MAX_GUESSES 10

#define FEEDBACK_NOT_IN_WORD 0
#define FEEDBACK_IN_WORD 1
#define FEEDBACK_IN_POSITION 2
#define MAX_FEEDBACK_ID (             \
    (FEEDBACK_IN_POSITION << 0 * 2) | \
    (FEEDBACK_IN_POSITION << 1 * 2) | \
    (FEEDBACK_IN_POSITION << 2 * 2) | \
    (FEEDBACK_IN_POSITION << 3 * 2) | \
    (FEEDBACK_IN_POSITION << 4 * 2))
typedef uint8_t Feedback;
typedef char Letter;

ssize_t indexForLetter(Letter letter)
{
    if (!('a' <= letter && letter <= 'z'))
    {
        std::cerr << "Invalid letter " << std::hex << static_cast<int>(letter) << std::endl;
        throw std::runtime_error("Invalid letter");
    }
    return static_cast<ssize_t>(letter - 'a');
}

class Word
{
public:
    Word(std::string wordString);
    ~Word();
    bool containsLetter(const Letter letter);
    const Letter &operator[](std::size_t index) const;
    std::string toString() const;
    friend std::ostream &operator<<(std::ostream &stream, const Word &word);

private:
    std::array<Letter, WORD_LENGTH> letters;
    std::array<std::bitset<WORD_LENGTH>, NUMBER_OF_LETTERS> letterPositionMap;
};

Word::Word(std::string wordString)
{
    if (wordString.size() != WORD_LENGTH)
    {
        throw std::runtime_error("Invalid word length");
    }
    for (int32_t i = 0; i < WORD_LENGTH; i++)
    {
        char letter = wordString[i];
        letters[i] = letter;
        letterPositionMap[indexForLetter(letter)][i] = true;
    }
}

Word::~Word()
{
}

std::string Word::toString() const
{
    std::stringstream result;
    for (int32_t i = 0; i < WORD_LENGTH; i++)
    {
        result << letters[i];
    }
    return result.str();
}

bool Word::containsLetter(Letter letter)
{
    return letterPositionMap[indexForLetter(letter)].any();
}

const Letter &Word::operator[](std::size_t index) const
{
    return letters[index];
}

std::ostream &operator<<(std::ostream &stream, const Word &word)
{
    return stream << word.toString();
}

struct GuessFeedback
{
    char guess[WORD_LENGTH];
    Feedback feedback[WORD_LENGTH];
    GuessFeedback(Word guessString, int32_t feedbackId)
    {
        for (int32_t i = 0; i < WORD_LENGTH; i++)
        {
            guess[i] = guessString[i];
            feedback[i] = (feedbackId >> (2 * i)) & 0x3;
        }
    }
    GuessFeedback(std::string guessString, std::string feedbackString)
    {
        if (guessString.size() != WORD_LENGTH)
        {
            throw std::runtime_error("Invalid guess length");
        }
        if (feedbackString.size() != WORD_LENGTH)
        {
            throw std::runtime_error("Invalid feedback length");
        }

        for (int32_t i = 0; i < WORD_LENGTH; i++)
        {
            guess[i] = guessString[i];
            switch (feedbackString[i])
            {
            case 'r':
                feedback[i] = FEEDBACK_NOT_IN_WORD;
                break;
            case 'y':
                feedback[i] = FEEDBACK_IN_WORD;
                break;
            case 'g':
                feedback[i] = FEEDBACK_IN_POSITION;
                break;
            default:
                throw std::runtime_error("Invalid feedback code");
                break;
            }
        }
    }

    bool isConsistentWith(Word word)
    {
        for (int32_t i = 0; i < WORD_LENGTH; i++)
        {
            switch (feedback[i])
            {
            case FEEDBACK_NOT_IN_WORD:
                if (word.containsLetter(guess[i]))
                {
                    return false;
                }
                break;
            case FEEDBACK_IN_WORD:
                if (!word.containsLetter(guess[i]))
                {
                    return false;
                }
                if (word[i] == guess[i])
                {
                    return false;
                }
                break;
            case FEEDBACK_IN_POSITION:
                if (word[i] != guess[i])
                {
                    return false;
                }
                break;
            }
        }
        return true;
    }
};

class WordleGame
{

public:
    WordleGame(
        std::unique_ptr<std::vector<Word> > guessWords,
        std::unique_ptr<std::vector<Word> > answerWords);
    ~WordleGame();

    bool isPossibleAnswer(Word word);
    Word getGuess();
    void pushFeedback(GuessFeedback guessFeedback);
    void popFeedback();
    static GuessFeedback computeFeedback(Word guess, Word solution);
    static int32_t computeFeedbackId(Word guess, Word solution);

private:
    const std::unique_ptr<std::vector<Word> > guessWords;
    const std::unique_ptr<std::vector<Word> > answerWords;
    std::vector<GuessFeedback> feedbacks;
};

WordleGame::WordleGame(
    std::unique_ptr<std::vector<Word> > guessWordList,
    std::unique_ptr<std::vector<Word> > answerWordList) : guessWords(std::move(guessWordList)),
                                                          answerWords(std::move(answerWordList))
{
    guessWords->insert(guessWords->end(), answerWords->begin(), answerWords->end());
    feedbacks.reserve(MAX_GUESSES);
}

WordleGame::~WordleGame()
{
}

int32_t WordleGame::computeFeedbackId(Word guess, Word solution)
{
    int32_t result = 0;
    for (int32_t i = 0; i < WORD_LENGTH; i++)
    {
        Feedback code;
        if (solution[i] == guess[i])
        {
            code = FEEDBACK_IN_POSITION;
        }
        else if (solution.containsLetter(guess[i]))
        {
            code = FEEDBACK_IN_WORD;
        }
        else
        {
            code = FEEDBACK_NOT_IN_WORD;
        }
        result |= code << (2 * i);
    }
    return result;
}

GuessFeedback WordleGame::computeFeedback(Word guess, Word solution)
{
    return GuessFeedback(guess, computeFeedbackId(guess, solution));
}

Word WordleGame::getGuess()
{
    if (feedbacks.size() == 0)
    {
        // pre-computed known best first guess.
        return Word("roate");
    }
    int32_t leastPossibleSolutions = -1;
    Word bestGuess = (*guessWords)[0];
    std::vector<int32_t> feedbackIdCounts;

    int32_t numPossibleSolutions = 0;
    for (auto possibleSolution : *answerWords)
    {
        if (isPossibleAnswer(possibleSolution))
        {
            numPossibleSolutions++;
        }
    }
    if (numPossibleSolutions < 100)
    {
        std::cout << "POSSIBLE SOLUTIONS: " << std::endl;
        for (auto possibleSolution : *answerWords)
        {
            if (isPossibleAnswer(possibleSolution))
            {
                std::cout << possibleSolution << std::endl;
            }
        }
    }
    if (numPossibleSolutions <= 2)
    {
        for (auto possibleSolution : *answerWords)
        {
            if (isPossibleAnswer(possibleSolution))
            {
                return possibleSolution;
            }
        }
    }

    for (auto potentialGuess : *guessWords)
    {
        feedbackIdCounts.clear();
        feedbackIdCounts.resize(MAX_FEEDBACK_ID + 1, 0);
        int32_t numberRemainingPossibleSolutions = 0;

        for (auto presumedSolution : *answerWords)
        {
            if (!isPossibleAnswer(presumedSolution))
                continue;
            auto feedbackId = computeFeedbackId(potentialGuess, presumedSolution);
            feedbackIdCounts[feedbackId]++;
        }
        for (int32_t feedbackId = 0; feedbackId <= MAX_FEEDBACK_ID; feedbackId++)
        {
            if (feedbackIdCounts[feedbackId] == 0)
                continue;
            pushFeedback(GuessFeedback(potentialGuess, feedbackId));
            for (auto possibleSolution : *answerWords)
            {
                if (isPossibleAnswer(possibleSolution))
                {
                    numberRemainingPossibleSolutions += feedbackIdCounts[feedbackId];
                }
            }
            popFeedback();
        }

        if (numberRemainingPossibleSolutions < leastPossibleSolutions || leastPossibleSolutions < 0)
        {
            leastPossibleSolutions = numberRemainingPossibleSolutions;
            bestGuess = potentialGuess;
        }
    }
    return bestGuess;
}

void WordleGame::pushFeedback(GuessFeedback guessFeedback)
{
    feedbacks.push_back(guessFeedback);
}

void WordleGame::popFeedback()
{
    feedbacks.pop_back();
}

bool WordleGame::isPossibleAnswer(Word word)
{
    for (auto guessFeedback : feedbacks)
    {
        if (!guessFeedback.isConsistentWith(word))
        {
            return false;
        }
    }
    return true;
}

std::unique_ptr<std::vector<Word> > readFileLines(std::istream &stream)
{
    std::unique_ptr<std::vector<Word> > lines(new std::vector<Word>);

    std::string line;
    while (std::getline(stream, line))
    {
        lines->push_back(Word(line));
    }

    return lines;
}

int main(int argc, char **argv)
{
    std::ifstream answersFile("answers.txt");
    if (!answersFile.is_open())
    {
        std::cerr << "Unable to read answers file" << std::endl;
        exit(1);
    }
    auto answerWords = readFileLines(answersFile);
    std::ifstream guessesFile("guesses.txt");
    if (!guessesFile.is_open())
    {
        std::cerr << "Unable to read guesses file" << std::endl;
        exit(1);
    }
    auto guessWords = readFileLines(guessesFile);

    WordleGame game(std::move(guessWords), std::move(answerWords));
    std::string line;
    while (true)
    {
        Word guess = game.getGuess();
        std::cout << "guess: " << guess << std::endl;
        bool gotFeedback = false;
        do
        {
            std::cout << "feedback: " << std::flush;

            if (std::getline(std::cin, line))
            {
                try
                {
                    game.pushFeedback(GuessFeedback(guess.toString(), line));
                    gotFeedback = true;
                }
                catch (const std::runtime_error &)
                {
                    std::cout << "Invalid feedback!" << std::endl;
                }
            }
        } while (!gotFeedback);
    }

    return 0;
}