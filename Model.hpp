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
#ifndef MODEL_HPP_INCLUDED
#define MODEL_HPP_INCLUDED

#include <string>
#include <unordered_map>
#include <exception>
#include <stdexcept>

namespace suflem {

/// Statistical suffix replacement model class.
class Model {
    std::unordered_map<std::string,
                       std::unordered_map<std::string,
                                          std::pair<long, long>>> _replacements;
    std::unordered_map<std::string, std::pair<long, long>> _lemcounts;
    std::unordered_map<std::string, std::pair<long, long>> _infcounts;
    size_t _max_suffix_size;
    bool _is_trimmed;

protected:
    void update_replacement(std::string const& inf,
                            std::string const& lem,
                            long tp,
                            long fp);
    void update_inflected(std::string const& inf, long tp, long fp);
    void update_lemma(std::string const& lem, long tp, long fp);
    void update(std::string const& inflected,
                std::string const& lemma,
                long count) throw(std::runtime_error);

    Model(size_t max_suffix_size=8) throw(std::runtime_error);

public:
    /// Lemmatize a word.
    /// \param inflected The inflected form of a word.
    /// \return lemmatized form of the word.
    std::string lemmatize(std::string const& inflected)
        const throw(std::runtime_error);

    /// Trim the model to reduce size.
    /// You won't be able to update() the model after trimming.
    void trim();

    /// Is the model trimmed.
    bool is_trimmed() const { return _is_trimmed; }

    /// Train the model from data set specified by filename.
    static Model train(std::string const& filename, long const max_suffix_size)
        throw(std::runtime_error);
    /// Load a previously trained model from file specified by filename.
    static Model load(std::string const& filename) throw(std::runtime_error);
    /// Save a model to file specified by filename.
    static void save(Model const& model, std::string const& filename)
        throw(std::runtime_error);
};

} //namespace suflem

#endif // MODEL_HPP_INCLUDED

