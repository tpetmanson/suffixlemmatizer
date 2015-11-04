##Suffix lemmatizer

THIS LIBRARY IS DEPRECATED AND REPLACED BY https://github.com/estnltk/suffix-lemmatizer

This library/program is a statistical lemmatizer learning most probable
inflected word form -> lemmatized word form suffix replacements.

It is designed mainly from Estonian language perspective, but could potentially
used with any inflective language, where different word forms share a
common prefix.

### Building
To build the shared library and binary, use Scons
system http://www.scons.org/


### Command line usage
usage: suflem model_path [--train=path] [--maxlen=integer] [--flush]
model_path - the path to save the model during training and to load the
             model during lemmatization.
--train=path - if given, start the progam in training mode. All input read
               from the given path
--maxlen=integer - maximal suffix length to store in training phase.
                   default value is 8.
--flush    - if given, flush the output after each processed input line.
             has no effect in training mode.

### Lemmatization mode (default)
Lemmatization mode reads one inflected word per line from standard input.
A previously trained model is read from `model_path` and used to lemmatize
the words. Lemmatized words are written to standard output, one word
per line. In the same order as inflected words were read from standard
input.

### Training mode
To train a new model, the `suflem` program requires input in
following format: each line has three tab-separated fields: the inflected
form, the respective lemma, number of occurrences in training corpus.
```
inflected_1     lemma_1 count_1
inflected_2     lemma_2 count_2
...
inflected_n     lemma_n count_n
```
If same inflected form and lemma occur more than once in the dataset, the
respective counts will be summed.

### Notes
- Beware that max line length in input is 1024 chars
and the error will pass silently, unless tokens could not be parsed.
- The program will expect all input to be in utf-8 encoding.
- Program uses characters '$' and \t internally, so if your strings contain
them, it may lower the classification accuracy or make the program crash.
