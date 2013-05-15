/* suffixlemmatizer - simple statistical suffix replacer for lemmatization.
    Copyright (C) 2013  Timo Petmanson

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

#include "Model.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>

using namespace std;
using namespace suflem;

static const char* usage =
"usage: suflem model_path [--train=path] [--maxlen=integer] [--flush]\n"
"model_path - the path to save the model during training and to load the\n"
"             model during lemmatization.\n"
"--train=path - if given, start the progam in training mode. All input read\n"
"               from the given path\n"
"--maxlen=integer - maximal suffix length to store in training phase.\n"
"                   default value is 8.\n"
"--flush    - if given, flush the output after each processed input line.\n"
"             has no effect in training mode.\n"
"\n"
"LEMMATIZATION MODE (default):\n"
"Lemmatization mode reads one inflected word per line from standard input.\n"
"A previously trained model is read from `model_path` and used to lemmatize\n"
"the words. Lemmatized words are written to standard output, one word\n"
"per line. In the same order as inflected words were read from standard\n"
"input.\n"
"\n"
"TRAINING MODE:\n"
"To train a new model, the `suflem` program requires input in\n"
"following format: each line has three tab-separated fields: the inflected\n"
"form, the respective lemma, number of occurrences in training corpus.\n"
"\n"
"inflected_1\tlemma_1\tcount_1\n"
"inflected_2\tlemma_2\tcount_2\n"
"...\n"
"inflected_n\tlemma_n\tcount_n\n"
"\n"
"If same inflected form and lemma occur more than once in the dataset, the\n"
"respective counts will be summed.\n"
"\n"
"NOTES:\n"
"- Beware that max line length in input is 1024 chars\n"
"and the error will pass silently, unless tokens could not be parsed.\n"
"- The program will expect all input to be in utf-8 encoding.\n"
"- Program uses characters '$' and \\t internally, so if your strings contain\n"
"them, it may lower the classification accuracy or make the program crash.\n"
"\n";

static void print_usage() {
    fprintf(stderr, usage);
}

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
            s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

void train_model(std::string const& model_path, std::string const& train_path,
                 long const max_suffix_size)
{
    fprintf(stderr, "Training model from dataset %s.\n", train_path.c_str());
    Model model = Model::train(train_path, max_suffix_size);
    fprintf(stderr, "Trimming model.\n");
    model.trim();
    fprintf(stderr, "Saving model to %s\n", model_path.c_str());
    Model::save(model, model_path);
    fprintf(stderr, "Done!\n");
}

void lemmatize_input(std::string const& model_path, bool flush_lines) {
    fprintf(stderr, "Loading model from %s.\n", model_path.c_str());
    Model model = Model::load(model_path);
    fprintf(stderr, "Loading model done!\n");

    char buffer[1099];
    std::string input;
    while (scanf("%1024s", buffer) == 1) {
        input = buffer;
        input = trim(input);
        printf("%s\n", model.lemmatize(input).c_str());
        if (flush_lines) {
            fflush(stdout);
        }
    }
    fflush(stdout);
}

int main(int argc, char** argv) {
    std::string model_path = "";
    std::string train_path = "";
    bool train_mode  = false;
    bool flush_lines = false;
    long maxlen = 8;

    const std::string TRAIN_FLAG = "--train=";
    const std::string FLUSH_FLAG = "--flush";
    const std::string HELP_FLAG  = "-h";
    const std::string HELP_FLAG2 = "--help";

    // try to parse arguments
    for (int i=1 ; i<argc ; ++i) {
        std::string s(argv[i]);
        if (s == FLUSH_FLAG) {
            flush_lines = true;
        } else if (s == HELP_FLAG || s == HELP_FLAG2) {
            print_usage();
            exit(0);
        } else if (s.substr(0, TRAIN_FLAG.size()) == TRAIN_FLAG) {
            train_path = s.substr(TRAIN_FLAG.size());
            train_mode = true;
            fprintf(stderr, "train path: %s\n", train_path.c_str());
        } else if (sscanf(argv[i], "--maxlen=%ld", &maxlen) == 1) {
            fprintf(stderr, "Max suffix size will be %ld\n", maxlen);
        } else if (i == 1) {
            model_path = s;
        } else {
            fprintf(stderr, ("Invalid argument: " + s + '\n').c_str());
            exit(-1);
        }
    }
    if (model_path.size() == 0) {
        fprintf(stderr, "model_path not given!\n");
        exit(-1);
    }

    try {
        if (train_mode) {
            train_model(model_path, train_path, maxlen);
        } else {
            lemmatize_input(model_path, flush_lines);
        }
    } catch (std::exception& e) {
        fprintf(stderr, "exception: %s\n", e.what());
    } catch (const char* s) {
        fprintf(stderr, "const char*: %s\n", s);
    } catch (...) {
        fprintf(stderr, "Unkown exception\n");
    }

    return EXIT_SUCCESS;
}
