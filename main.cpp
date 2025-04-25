#include <iostream>
#include <unordered_map>
#include <vector>

#include "parse_ast.h"

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_letter(char c) {
    return ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z'));
}

static bool SD_scan(std::string& input, std::vector<SD_token>& output, char& var) {
    bool error = false;
    auto start = 0;

    for (auto i = 0; i < input.size(); i++) {
        switch (input[i]) {
        case ' ': {
            start = i + 1;
            break;
        }
        case '+': {
            output.emplace_back(SD_token(SD_tokenType::SD_PLUS, "+"));
            start = i + 1;
            break;
        }
        case '-': {
            output.emplace_back(SD_token(SD_tokenType::SD_MINUS, "-"));
            start = i + 1;
            break;
        }
        case '*': {
            output.emplace_back(SD_token(SD_tokenType::SD_MULTIPLY, "*"));
            start = i + 1;
            break;
        }
        case '^': {
            output.emplace_back(SD_token(SD_tokenType::SD_CARET, "^"));
            start = i + 1;
            break;
        }
        default:
            if (is_digit(input[i])) {
                while (is_digit(input[i])) {
                    ++i;
                }
                output.emplace_back(SD_token(SD_tokenType::SD_NUMBER, std::string(input, start, i - start)));
                start += i;
                i--;
            } else if (is_letter(input[i])) {
                input[i] = std::tolower(input[i]);
                if (var == '\0') var = input[i];
                else if (var != input[i]) {
                    std::cout << "Error: Attempt to re-bind differentiating variable '" << var << "' with '" << input[i] << "'\n";
                    return false;
                }
                output.emplace_back(SD_token(SD_tokenType::SD_VARIABLE, std::to_string(input[i])));
                start += 1;
            } else {
                std::cout << "Error: Unknown symbol '" << input[i] << "'\n";
                return false;
            }
        }
    }

    output.emplace_back(SD_token(SD_tokenType::SD_ENDEXPR, "$"));
    return true;
}

void SD_differentiate_and_print(std::unique_ptr<Expr> ast, char var) {
    std::vector<std::unique_ptr<Expr>> expr_vec;
    ast->differentiate(expr_vec);
    for (auto i = 0; i < expr_vec.size(); i++) {
        expr_vec[i]->print_as_expr(var);
        if (i < (expr_vec.size() - 1)) std::cout << " + ";
        else std::cout << "\n";   
    }
}

int main() {
    std::vector<SD_token> tokens;

    int sanityCheckFailIndex;
    char var = '\0';

    std::cout << "SymbDiff ('q'/'exit'/'quit'/CTRL-C to exit)\n";
    while (true) {
        std::string input;
        std::cout << "> ";
        if (!std::getline(std::cin, input) || input == "q" ||
            input == "quit" || input == "exit")
        {
            std::cout << (input == "q" ? "quit..\n" : "quit...\n");
            return 0;
        } else if (input == "") {
            continue;
        }

        if (!SD_scan(input, tokens, var)) {
            tokens.clear();
            continue;
        }

        auto ast = SD_parse_to_ast(tokens, false);
        if (ast->kind == SD_ExprKind::BOTCHED) {
            BotchedExpr* botched = static_cast<BotchedExpr*>(ast.get());
            std::cout << botched->message << "\n";
            tokens.clear();
            continue;
        }

        SD_differentiate_and_print(std::move(ast), var);
        tokens.clear();
    }
}