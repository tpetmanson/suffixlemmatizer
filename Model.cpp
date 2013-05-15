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
#include <vector>
#include <algorithm>

namespace suflem {

long const MAX_LINE_LENGTH = 1024;

///////////////////////////////////////////////////////////////////////////////
// Miscellaneous functions
///////////////////////////////////////////////////////////////////////////////

// function for detecting utf-8 codepoint begins in string `s`.
// the indices in `s` are stored into `v`.
static inline bool store_codepoints(std::string const& s,
                                    std::vector<long>& v) {
    const long masks[] = {0, 0x6, 0xE, 0x1E, 0x3E, 0x7E};
    const long shift[] = {7, 5, 4, 3, 2, 1};
    const long n = static_cast<long>(s.size());
    v.clear();
    for (long i=0 ; i<n ; ++i) {
        bool found_start = false;
        unsigned char c = static_cast<unsigned char>(s[i]);
        for (int j=0 ; j<6 ; ++j) {
            if (c >> shift[j] == masks[j]) { // we detect a code point
                v.push_back(i);
                found_start = true;
                break;
            }
        }
        if (!found_start) {
            if (c >> 6 != 0x2 || i == 0) {
                // utf-8 decode error
                return false;
            }
        }
    }
    return true;
}

static inline
long common_prefix_size(std::string const& a, std::string const& b,
                        std::vector<long> const& acodepoints,
                        std::vector<long> const& bcodepoints)
{
    long N = std::min(acodepoints.size(), bcodepoints.size());
    long preflen = 0;
    for (long j=0 ; j<N ; ++j) {
        if (acodepoints[j] != bcodepoints[j]) {
            return j;
        }
        for (long i=preflen ; i<acodepoints[j] ; ++i) {
            if (a[i] != b[i]) {
                return j;
            }
        }
        preflen = acodepoints[j];
    }
    return N;
}

// simple function for computing probability of tp and fp counts
static inline double compute_prob(std::pair<long, long> const& p) {
    return static_cast<double>(p.first) / (p.first + p.second);
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

///////////////////////////////////////////////////////////////////////////////
// Main model methods
///////////////////////////////////////////////////////////////////////////////

Model::Model(size_t max_suffix_size) throw(std::runtime_error) :
    _replacements(), _lemcounts(), _infcounts(),
    _max_suffix_size(max_suffix_size), _is_trimmed(false)
{
    if (_max_suffix_size < 1 || _max_suffix_size > 1024) {
        throw std::runtime_error("must be 1 <= max_suffix_size <= 1024");
    }
}

void Model::update_replacement(std::string const& inf,
                               std::string const& lem,
                               long tp,
                               long fp)
{
    std::pair<long, long>& data = _replacements[inf][lem];
    data.first  += tp;
    data.second += fp;
}

void Model::update_inflected(std::string const& inf, long tp, long fp) {
    std::pair<long, long>& data = _infcounts[inf];
    data.first  += tp;
    data.second += fp;
}

void Model::update_lemma(std::string const& lem, long tp, long fp) {
    std::pair<long, long>& data = _lemcounts[lem];
    data.first  += tp;
    data.second += fp;
}

void Model::update(std::string const& inflected, std::string const& lemma,
                   long count) throw(std::runtime_error)
{
    if (is_trimmed()) {
        throw std::runtime_error("Cannot update a trimmed model.");
    }
    // insert special markers to denote string beginning
    std::string inf = '$' + inflected;
    std::string lem = '$' + lemma;
    // vectors to store utf-8 codepoint beginnings
    std::vector<long> infcodepoints;
    std::vector<long> lemcodepoints;
    // inflected and lemmatized suffixes
    std::string infsuf;
    std::string lemsuf;
    // detect characters in utf-8 encoded data
    if (!store_codepoints(inf, infcodepoints) ||
        !store_codepoints(lem, lemcodepoints)) {
        throw std::runtime_error("Utf-8 decode error.");
    }
    // add positions to mark the end of the string
    infcodepoints.push_back(inf.size());
    lemcodepoints.push_back(lem.size());

    // compute some size constants
    const long n = std::max(infcodepoints.size(), lemcodepoints.size());
    const long m = std::max(n-static_cast<long>(_max_suffix_size+1),
                            static_cast<long>(0));
    const long lemsize = lemcodepoints.size();
    const long infsize = infcodepoints.size();
    const long prefixsize = common_prefix_size(inf, lem,
                                               infcodepoints, lemcodepoints);
    //printf("%s\t%s\n", inf.c_str(), lem.c_str());
    for (long i=n-2 ; i>=m ; --i) {
        infsuf = inf.substr(infcodepoints[std::min(infsize-1, i)]);
        lemsuf = lem.substr(lemcodepoints[std::min(lemsize-1, i)]);
        // if replacement is correct, store as true positive replacement,
        // otherwise as false positive replacement
        if (i <= prefixsize-1) {
            //printf("correct ");
            update_replacement(infsuf, lemsuf, count, 0);
        } else {
            update_replacement(infsuf, lemsuf, 0, count);
        }
        // lemma suffix is true positive, inflected suffix is false positive
        update_lemma(lemsuf, count, 0);
        update_lemma(infsuf, 0, count);
        // inflected suffix is true positive, lemma suffix is false positive
        update_inflected(infsuf, count, 0);
        update_inflected(lemsuf, 0, count);
        //printf("%ld\t%s\t%s\n", i, infsuf.c_str(), lemsuf.c_str());
    }
}

std::string Model::lemmatize(std::string const& inflected)
    const throw(std::runtime_error)
{
    std::string inf = '$' + inflected; inf = suflem::trim(inf);
    std::string infsuf;
    std::string lemsuf;
    std::string best_lemma;
    std::vector<long> codepoints;
    double best_prob;
    bool iter_found;
    double prAB, prBA, prA, prB;

    if (!store_codepoints(inf, codepoints)) {
        throw std::runtime_error("Utf-8 decode error!");
    }
    codepoints.push_back(inf.size());
    long n = codepoints.size();

    // start looking for longest suffix replacements
    for (long i=0 ; i<n ; ++i) {
        best_prob  = 0.0;
        iter_found = false;
        // get the probability of inflected suffix
        infsuf = inf.substr(codepoints[i]);
        auto infit = _infcounts.find(infsuf);
        if (infit != _infcounts.end()) {
            prB = compute_prob(infit->second);
        } else {
            // probability is zero
            continue;
        }

        // does this infsuf has possible replacements
        auto repit = _replacements.find(infsuf);
        if (repit == _replacements.end()) {
            continue;
        }

        // check out different replacements
        for (auto k=repit->second.begin() ; k != repit->second.end() ; ++k) {
            lemsuf = k->first;
            // get lemma suffix probability
            auto lemit = _lemcounts.find(lemsuf);
            if (lemit == _lemcounts.end()) {
                // probability will be zero
                continue;
            } else {
                prA = compute_prob(lemit->second);
            }
            // get replacement probability
            prBA = compute_prob(k->second);
            // compute the probability, that lemsuf is the correct replacement
            // for infsuf
            prAB = (prBA * prA) / prB;
            //printf("%s %lf=(%lf * %lf)/%lf\n", (inf.substr(0, codepoints[i]) + lemsuf).c_str(), prAB, prBA, prA, prB);
            if (prAB > best_prob) {
                best_lemma = inf.substr(0, codepoints[i]) + lemsuf;
                best_prob = prAB;
                iter_found = true;
            }
        }

        // did we find anything?
        if (iter_found) {
            return best_lemma.substr(1); // trim the $ from beginning
        }
    }
    // did not find anything
    return inflected;
}

void Model::trim() {
    if (is_trimmed()) {
        return;
    }
    long numlem = 0;
    long numinf = 0;
    long numrep = 0;
    _is_trimmed = true;
    // trim lemma probs
    for (auto i=_lemcounts.begin(); i!=_lemcounts.end() ; ) {
        if (i->second.first == 0) {
            auto j = i; ++j;
            _lemcounts.erase(i);
            ++numlem;
            i = j;
            continue;
        }
        ++i;
    }
    // trim inflection probs
    for (auto i=_infcounts.begin(); i!=_infcounts.end() ; ) {
        if (i->second.first == 0) {
            auto j = i; ++j;
            _infcounts.erase(i);
            ++numinf;
            i = j;
            continue;
        }
        ++i;
    }
    // trim replacements
    for (auto i=_replacements.begin(); i!=_replacements.end() ; ) {
        for (auto j=i->second.begin(); j!=i->second.end() ; ) {
            if (j->second.first == 0) {
                auto k = j; ++k;
                i->second.erase(j);
                ++numrep;
                j = k;
                continue;
            }
            ++j;
        }
        if (i->second.size() == 0) {
            auto j = i; ++j;
            _replacements.erase(i);
            i = j;
        } else {
            ++i;
        }
    }
    //printf("%ld %ld %ld\n", numlem, numinf, numrep);
}

///////////////////////////////////////////////////////////////////////////////
// Other model related methods.
///////////////////////////////////////////////////////////////////////////////

Model Model::train(std::string const& filename,
                          long const max_suffix_size)
    throw(std::runtime_error)
{
    FILE* fin = fopen(filename.c_str(), "rb");
    if (!fin) {
        std::string err = "Could not open file " + filename;
        throw std::runtime_error(err);
    }
    Model model(max_suffix_size);

    char lemma[MAX_LINE_LENGTH+32];
    char inflected[MAX_LINE_LENGTH+32];
    long count;
    long lineno = 0;

    while (fscanf(fin, "%1024[^\t]\t%1024[^\t]\t%ld%*[\n]", 
            inflected, lemma, &count) == 3)
    {
        std::string inf = inflected;
        std::string lem = lemma;
        inf = suflem::trim(inf); lem = suflem::trim(lem);
        if (inf.size() == 0 || lem.size() == 0) {
            fclose(fin);
            std::string err = "Zero-length string on line "
                + std::to_string(lineno);
            throw std::runtime_error(err);
        }
        if (count <= 0) {
            fclose(fin);
            std::string err = "count<=0 on line "
                + std::to_string(lineno);
            throw std::runtime_error(err);
        }
        model.update(inf, lem, count);
        ++lineno;
    }
    if (!feof(fin)) {
        fclose(fin);
        throw std::runtime_error("Read error after line "
                                    + std::to_string(lineno));
    }

    fclose(fin);
    return model;
}

Model Model::load(std::string const& filename) throw(std::runtime_error) {
    // open the file for loading and initiate a reader
    FILE* fin = fopen(filename.c_str(), "rb");
    if (!fin) {
        std::string err = "Could not open file " + filename;
        throw std::runtime_error(err);
    }
    // read max suffix size and model trimmed state
    long maxsuf;
    int trimmed;
    if (fscanf(fin, "%ld\t%d%*[\n]", &maxsuf, &trimmed) != 2) {
        std::string err = "Could not read max suffix size and trimmed state.";
        throw std::runtime_error(err);
    }
    Model model(maxsuf);
    model._is_trimmed = static_cast<bool>(trimmed);

    // define some variables for reading
    char lemma[MAX_LINE_LENGTH+32];
    char inflected[MAX_LINE_LENGTH+32];
    lemma[MAX_LINE_LENGTH+1] = '\0';
    inflected[MAX_LINE_LENGTH+1] = '\0';
    std::string inf, lem;
    long tp;
    long fp;

    // read lemmas
    long numlemmas;
    if (fscanf(fin, "%ld%*[\n]", &numlemmas) != 1) {
        fclose(fin);
        throw std::runtime_error("Could not read number of lemmas.");
    }
    for (long i=0 ; i<numlemmas ; ++i) {
        if (fscanf(fin, "%1024[^\t]\t%ld\t%ld%*[^\n]", lemma, &tp, &fp) != 3) {
            fclose(fin);
            throw std::runtime_error("Could not read model data.");
        }
        lem = lemma; lem = suflem::trim(lem);
        model.update_lemma(lem, tp, fp);
    }
    // read inflections
    long numinflections;
    if (fscanf(fin, "%ld%*[\n]", &numinflections) != 1) {
        fclose(fin);
        throw std::runtime_error("Could not read number of inflected forms.");
    }
    for (long i=0 ; i<numinflections ; ++i) {
        if (fscanf(fin, "%1024[^\t]\t%ld\t%ld%*[^\n]",
            inflected, &tp, &fp) != 3)
        {
            fclose(fin);
            throw std::runtime_error("Could not read model data.");
        }
        inf = inflected; inf = suflem::trim(inf);
        model.update_inflected(inf, tp, fp);
    }
    // read replacements
    long numreplacements;
    if (fscanf(fin, "%ld%*[\n]", &numreplacements) != 1) {
        fclose(fin);
        throw std::runtime_error("Could not read the number of replacements");
    }
    for (long i=0 ; i<numreplacements ; ++i) {
        long numlems;
        if (fscanf(fin, "%1024[^\t]\t%ld%*[^\n]", inflected, &numlems) != 2) {
            fclose(fin);
            throw std::runtime_error("Could not read model data.");
        }
        inf = inflected; inf = suflem::trim(inf);
        for (long j=0 ; j<numlems ; ++j) {
            if (fscanf(fin, "%1024[^\t]\t%ld\t%ld%*[^\n]",
                lemma, &tp, &fp) != 3)
            {
                fclose(fin);
                throw std::runtime_error("Could not read model data.");
            }
            lem = lemma; lem = suflem::trim(lem);
            model.update_replacement(inf, lem, tp, fp);
        }
    }
    if (ferror(fin)) {
        fclose(fin);
        throw std::runtime_error("Could not read model data.");
    }
    fclose(fin);
    return model;
}

void Model::save(Model const& model, std::string const& filename)
    throw(std::runtime_error)
{
    FILE* fout = fopen(filename.c_str(), "wb");
    if (!fout) {
        throw std::runtime_error("Could not open file "
                                 + filename + " for writing");
    }

    fprintf(fout, "%ld\t%d\n", model._max_suffix_size, model.is_trimmed());
    fprintf(fout, "%ld\n", model._lemcounts.size());
    for (auto i=model._lemcounts.begin() ; i!=model._lemcounts.end(); ++i) {
        fprintf(fout, "%s\t%ld\t%ld\n",
                i->first.c_str(), i->second.first, i->second.second);
    }
    fprintf(fout, "%ld\n", model._infcounts.size());
    for (auto i=model._infcounts.begin() ; i!=model._infcounts.end(); ++i) {
        fprintf(fout, "%s\t%ld\t%ld\n",
                i->first.c_str(), i->second.first, i->second.second);
    }
    fprintf(fout, "%ld\n", model._replacements.size());
    auto i=model._replacements.begin();
    for ( ; i!=model._replacements.end(); ++i) {
        fprintf(fout, "%s\t%ld\n", i->first.c_str(), i->second.size());
        auto j = i->second.begin();
        for ( ; j!=i->second.end() ; ++j) {
            fprintf(fout, "%s\t%ld\t%ld\n",
                    j->first.c_str(), j->second.first, j->second.second);
        }
    }
    if (ferror(fout)) {
        fclose(fout);
        throw std::runtime_error("Could not write model to file!");
    }
    fclose(fout);
}

} // namespace suflem
