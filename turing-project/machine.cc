#include "machine.hh"
#include "utils.hh"
#include <cctype>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <limits>

using std::vector;

bool Transition::match(State cur_state, vector<char> cur_syms) const {
    if (cur_state != old_state()) {
        return false;
    }
    for (size_t i = 0; i < cur_syms.size(); ++i) {
        if (old_syms().at(i) == '*'
            && cur_syms.at(i) == '_') {
            return false;
        }
        if (old_syms().at(i) != '*'
            && cur_syms.at(i) != old_syms().at(i)) {
            return false;
        }
    }
    return true;
}

void Tape::gc() {
    while (_lm->_next->_content == _blank && _lm->_next != _cur) {
        _lm->_next        = _lm->_next->_next;
        _lm->_next->_prev = _lm;
    }
    while (_rm->_prev->_content == _blank && _rm->_prev != _cur) {
        _rm->_prev        = _rm->_prev->_prev;
        _rm->_prev->_next = _rm;
    }
}

std::vector<std::pair<char, int>> Tape::to_vec() {
    std::vector<std::pair<char, int>> result{};
    gc();
    for (
        auto it = _lm->_next;
        it != _rm;
        it = it->_next) {
        result.push_back(std::make_pair(it->_content, it->_index));
    }
    return result;
}

std::string Tape::to_string() {
    std::string result;
    for (
        auto it = _lm;
        it != _rm;
        it = it->_next) {
        result += it->_content;
    }
    utils::strip(result, _blank);
    return result;
}

void Tape::move(Direction dir) {
    switch (dir) {
        case L: {
            if (_cur->_prev == _lm) {
                _cur->_prev = std::make_shared<Node>(
                    Node{_cur, _lm, _blank, _cur->_index - 1});
                _lm->_next = _cur->_prev;
            }
            _cur = _cur->_prev;
            break;
        }
        case R: {
            if (_cur->_next == _rm) {
                _cur->_next = std::make_shared<Node>(
                    Node{_rm, _cur, _blank, _cur->_index + 1});
                _rm->_prev = _cur->_next;
            }
            _cur = _cur->_next;
            break;
        }
        case N: break;
    }
}

Tape::Tape(char blank, std::string input) :
    _blank(blank) {
    _cur        = std::make_shared<Node>(Node{._content = _blank});
    _cur->_prev = std::make_shared<Node>(Node{._content = _blank});
    _cur->_next = std::make_shared<Node>(Node{._content = _blank});

    _cur->_prev->_prev = _cur->_prev;
    _cur->_prev->_next = _cur;
    _cur->_index       = input.size() - 1;

    _cur->_next->_next = _cur->_next;
    _cur->_next->_prev = _cur;

    _lm         = _cur->_prev;
    _lm->_index = std::numeric_limits<int>::min();

    _rm         = _cur->_next;
    _rm->_index = std::numeric_limits<int>::max();

    std::reverse(input.begin(), input.end());

    for (char ch : input) {
        write(ch);
        move(Tape::L);
    }
    move(Tape::R);
}

Tape::Tape(char blank) :
    Tape::Tape(blank, "") {
}

Machine::Machine(
    size_t                    n,
    const std::string        &input,
    const vector<Transition> &transitions,
    const State              &initial_state) :
    _n(n),
    _transitions(transitions),
    _state(initial_state),
    _n_step(0) {
    // init tapes
    _tapes.reserve(_n);
    _tapes.push_back(Tape{'_', input});
    for (size_t i = 1; i < _n; ++i) {
        _tapes.push_back(Tape{'_'});
    }
}

void Machine::print_vec(const std::string &spec, const std::vector<std::string> &vec) const {
    std::cout << spec;
    std::for_each(
        vec.begin(),
        vec.end(),
        [](const std::string &str) {
            std::cout << str << " ";
        });
    std::cout << std::endl;
}

bool Machine::step(bool is_verbose) {
    vector<char> cur_syms;
    for (const auto &tp : _tapes) {
        cur_syms.push_back(tp.cur_sym());
    }

    if (is_verbose) {
        std::cout << "Step   : "
                  << n_step()
                  << std::endl;
        std::cout << "State  : "
                  << _state.name()
                  << std::endl;
        for (size_t i = 0; i < n_tape(); ++i) {
            std::vector<std::string> prt_index, prt_sym, prt_head;

            for (auto pair : _tapes.at(i).to_vec()) {
                std::string index = std::to_string(std::abs(pair.second));
                std::string sym(1, pair.first);
                std::string head(
                    index.size(),
                    (pair.second == _tapes.at(i).cur_index()) ? '^' : ' ');

                sym.append(std::string(index.size() - 1, ' '));
                head.append(std::string(index.size() - 1, ' '));

                prt_index.push_back(index);
                prt_sym.push_back(sym);
                prt_head.push_back(head);
            }
            print_vec("Index" + std::to_string(i) + " : ", prt_index);
            print_vec("Tape" + std::to_string(i) + "  : ", prt_sym);
            print_vec("Head" + std::to_string(i) + "  : ", prt_head);
        }
        std::cout << "---------------------------------------------" << std::endl;
    }

    _n_step++;
    for (auto trans : _transitions) {
        if (trans.match(_state, cur_syms)) {
            _state = trans.new_state();
            for (size_t i = 0; i < cur_syms.size(); ++i) {
                if (trans.new_syms().at(i) != '*') {
                    _tapes.at(i).write(trans.new_syms().at(i));
                }
                _tapes.at(i).move(trans.dirs().at(i));
            }
            return true;
        }
    }
    return false;
}

std::string Machine::output() {
    return _tapes[0].to_string();
}
