#include "parser.hh"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <regex>
#include <stdexcept>

namespace parser {

static std::string::const_iterator _end;

static size_t _n;

// skip comments, blanks, spaces
void parse_skip(std::string::const_iterator &it) {
    while (it != _end && (std::isblank(*it) || std::isspace(*it) || *it == ';')) {
        if (*it == ';') {
            while (it != _end && *it != '\n') {
                it++;
            }
        }
        it++;
    }
}

// parse exactly one character, ignoring leading skips
bool parse_char(std::string::const_iterator &it, char ch) {
    parse_skip(it);
    if (*it == ch) {
        it++;
        return true;
    }
    return false;
}

// parse identifier, ignoring leading skips
std::string parse_id(std::string::const_iterator &it) {
    std::regex  p_id("[a-zA-Z_0-9]+");
    std::smatch result{};

    parse_skip(it);
    if (!std::isalnum(*it) && *it != '_') {
        throw parser_exception("expected identifier");
    }
    if (std::regex_search(it, _end, result, p_id)) {
        it += result[0].length();
        return result[0].str();
    } else {
        throw parser_exception("expected identifier");
    }
}

// parse a comma seperated list of identifiers
std::vector<std::string> parse_id_list(std::string::const_iterator &it) {
    std::vector<std::string> result{};
    if (!parse_char(it, '{')) {
        throw parser_exception("expected '{'");
    }
    result.push_back(parse_id(it));

    while (it != _end && parse_char(it, ',')) {
        result.push_back(parse_id(it));
    }

    if (!parse_char(it, '}')) {
        throw parser_exception("expected '}'");
    }
    if (result.empty()) {
        throw parser_exception("expected non-empty list");
    }

    return result;
}

vector<char> parse_syms(std::string::const_iterator &it) {
    std::vector<char> result;

    for (auto ch : parse_id(it)) {
        result.push_back(ch);
    }
    return result;
}

State parse_state(std::string::const_iterator &it) {
    return State(parse_id(it));
}

vector<Tape::Direction> parse_dirs(std::string::const_iterator &it) {
    vector<Tape::Direction> dirs;
    parse_skip(it);

    while (*it == 'l' || *it == 'r' || *it == '*') {
        char ch = *it++;
        switch (ch) {
            case 'l': dirs.push_back(Tape::L); break;
            case 'r': dirs.push_back(Tape::R); break;
            case '*': dirs.push_back(Tape::N); break;
        }
    }

    if (dirs.empty()) {
        throw parser_exception("expected directions");
    }

    return dirs;
}

std::vector<Transition> parse_tran(std::string::const_iterator &it) {
    std::vector<Transition> transitions;

    parse_skip(it);
    while (it != _end) {
        State                   old_state = parse_state(it);
        vector<char>            old_syms  = parse_syms(it);
        vector<char>            new_syms  = parse_syms(it);
        vector<Tape::Direction> dirs      = parse_dirs(it);
        State                   new_state = parse_state(it);

        transitions.push_back(
            Transition{
                old_state,
                new_state,
                old_syms,
                new_syms,
                dirs,
                _n});

        parse_skip(it);
    }
    return transitions;
}

Machine parse(const std::string &program, const std::string &input) {
    std::string::const_iterator it = program.cbegin();

    _end = program.cend();

    vector<Transition> transitions;
    vector<State>      states;
    vector<char>       input_syms;
    vector<char>       tape_syms;
    State              initial_state{""};
    char               blank;
    size_t             tm_counter = 0;

    while (it != _end) {
        parse_skip(it);

        char ch = *it++;
        if (ch == '#') { // Q, S, G, q0, B, F, N
            ch = *it++;
            switch (ch) {
                case 'Q': {
                    tm_counter++;
                    if (!parse_char(it, '=')) {
                        throw parser_exception("expected '='");
                    }
                    for (auto str : parse_id_list(it)) {
                        states.push_back(State(str));
                    }
                    break;
                }
                case 'S': {
                    tm_counter++;
                    if (!parse_char(it, '=')) {
                        throw parser_exception("expected '='");
                    }
                    for (auto str : parse_id_list(it)) {
                        input_syms.push_back(str.at(0));
                    }
                    break;
                }
                case 'G': {
                    tm_counter++;
                    if (!parse_char(it, '=')) {
                        throw parser_exception("expected '='");
                    }
                    for (auto str : parse_id_list(it)) {
                        tape_syms.push_back(str.at(0));
                    }
                    break;
                }
                case 'q': {
                    tm_counter++;
                    if (!parse_char(it, '0')) {
                        throw parser_exception("expected 'q0'");
                    }
                    if (!parse_char(it, '=')) {
                        throw parser_exception("expected '='");
                    }
                    initial_state = State(parse_id(it));
                    break;
                }
                case 'B': {
                    tm_counter++;
                    if (!parse_char(it, '=')) {
                        throw parser_exception("expected '='");
                    }
                    parse_skip(it);
                    blank = *it++;
                    break;
                }
                case 'F': {
                    tm_counter++;
                    if (!parse_char(it, '=')) {
                        throw parser_exception("expected '='");
                    }
                    parse_id_list(it);
                    break;
                }
                case 'N': {
                    if (tm_counter != 6) {
                        throw parser_exception("expected complete .tm file");
                    }

                    if (!parse_char(it, '=')) {
                        throw parser_exception("expected '='");
                    }
                    parse_skip(it);
                    _n          = std::stoi(parse_id(it));
                    transitions = parse_tran(it);
                    return Machine{_n, input, transitions, initial_state};
                    break;
                }
                default: throw parser_exception("expected Q, S, G, q0, B, F, N");
            }
        } else {
            throw parser_exception("expected '#'");
        }
    }
    throw parser_exception("expected complete .tm file");
}

Machine parse_file(const std::string &filename, const std::string &input) {
    std::ifstream ifs(filename, std::ios_base::in);
    if (ifs.is_open()) {
        std::string program, line;
        while (std::getline(ifs, line)) {
            program += line + '\n';
        }
        return parse(program, input);
    } else {
        throw parser_exception("cannot open file");
    }
}

} // namespace parser