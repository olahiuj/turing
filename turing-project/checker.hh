#pragma once

#include "machine.hh"
#include <algorithm>
#include <exception>
#include <vector>

class input_exception : public std::exception {
  public:
    input_exception(size_t index) :
        _index(index) {
    }

    size_t what() {
        return _index;
    }

  private:
    const size_t _index;
};

class Checker {
  public:
    Checker(
        std::vector<char> tape_syms,
        std::vector<char> input_syms) :
        _tape_syms(tape_syms),
        _input_syms(input_syms) {
    }

    void check(const std::string &input) const {
        for (size_t i = 0; i < input.size(); ++i) {
            char ch = input.at(i);
            if (std::find(
                    _input_syms.begin(),
                    _input_syms.end(),
                    ch)
                == _input_syms.end()) {
                throw input_exception(i);
            }
        }
    }

  private:
    std::vector<char> _tape_syms;
    std::vector<char> _input_syms;
};