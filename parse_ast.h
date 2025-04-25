#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cmath>

enum SD_tokenType {
    SD_VARIABLE,
    SD_NUMBER,
    SD_PLUS,
    SD_MINUS,
    SD_MULTIPLY,
    SD_CARET,
    SD_ENDEXPR,
};

struct SD_token {
    SD_tokenType ttype;
    std::string literal;

    SD_token(SD_tokenType type, std::string str) :
        ttype(type), literal(str) {};
};

enum class SD_ExprKind {
    ATOMIC,
    BINARY,
    BOTCHED,
};

struct Expr {
    SD_ExprKind kind;

    virtual void print_as_ast() {;}
    virtual void print_as_expr(char var) {;}
    virtual void differentiate(std::vector<std::unique_ptr<Expr>>& kch) {
        ;
    }
};

struct AtomicExpr : Expr {
    double constant;
    int power;

    AtomicExpr(double constant, int power) : constant(constant), power(power) {}

    void print_as_ast() override {
        std::cout << "[" << constant << ", " << power << "]";
    }

    void print_as_expr(char var) override {
        if (constant >= 1 && power == 0 ||
            constant > 1 && power >= 1) 
        {
            std::cout << constant;
        }
        if (power == 1) std::cout << var;
        else if (power >= 1) std::cout << var << "^" << power;
    }

    void differentiate(std::vector<std::unique_ptr<Expr>>& expr_vec) override {
        auto new_constant = constant * power;
        auto new_power = (power == 0 ? 0 : power - 1);
        auto atomic_diff = std::make_unique<AtomicExpr>(new_constant, new_power);
        atomic_diff->kind = SD_ExprKind::ATOMIC;
        expr_vec.push_back(std::move(atomic_diff));
    }
};

struct BinExpr : Expr {
    char op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinExpr(char op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right) :
        op(op), left(std::move(left)), right(std::move(right)) {}

    void print_as_ast() override {
        std::cout << "(";
        left->print_as_ast();
        std::cout << " " << op << " ";
        right->print_as_ast();
        std::cout << ")";
    }

    void print_as_expr(char var) override {
        left->print_as_expr(var);
        std::cout << " " << op << " ";
        right->print_as_expr(var);
    }

    void differentiate(std::vector<std::unique_ptr<Expr>>& expr_vec) override {
        if (op == '*') {
            left->differentiate(expr_vec);
            right->differentiate(expr_vec);
            
            auto right_diff = std::move(expr_vec.back());
            expr_vec.pop_back();
            auto left_diff = std::move(expr_vec.back());
            expr_vec.pop_back();
            
            auto prod_left = std::make_unique<BinExpr>('*', std::move(left_diff), std::move(right));
            prod_left->kind = SD_ExprKind::BINARY;
            auto prod_right = std::make_unique<BinExpr>('*', std::move(right_diff), std::move(left));
            prod_right->kind = SD_ExprKind::BINARY;
            
            expr_vec.push_back(std::move(prod_left));
            expr_vec.push_back(std::move(prod_right));
        } else if (op == '+' || op == '-') {
            left->differentiate(expr_vec);
            right->differentiate(expr_vec);

            auto right_diff = std::move(expr_vec.back());
            expr_vec.pop_back();
            auto left_diff = std::move(expr_vec.back());
            expr_vec.pop_back();

            auto sum = std::make_unique<BinExpr>(op, std::move(left_diff), std::move(right_diff));
            sum->kind = SD_ExprKind::BINARY;

            expr_vec.push_back(std::move(sum));
        }
    }
};

struct BotchedExpr : Expr {
    std::string message;

    BotchedExpr(std::string msg) : message(msg) {}
};

static std::unique_ptr<Expr> parse_atomic(bool negate, std::vector<SD_token> tokens, int& curr) {
    int constant = 1, power = 0;
    auto start_token = curr;
    if (tokens[curr].ttype == SD_tokenType::SD_NUMBER) {
        constant = std::stod(tokens[curr].literal);
    } else if (tokens[curr].ttype == SD_tokenType::SD_VARIABLE) {
        constant = 1;
        power = 1;
    } else {
        // Unexpected, return a botched expr with error message
        std::string msg = "Error: Unexpected symbol '";
        msg.append(tokens[curr].literal);
        msg.append("'. Expected a number/variable instead");
        auto botched = std::make_unique<BotchedExpr>(msg);
        botched->kind = SD_ExprKind::BOTCHED;
        return std::move(botched);
    }

    // step past number/variable token
    curr += 1;

    if (tokens[curr].ttype == SD_tokenType::SD_CARET) {
        // step past caret token
        curr += 1;
        if (tokens[curr].ttype == SD_tokenType::SD_NUMBER) {
            int pow = std::stod(tokens[curr].literal);
            if (tokens[start_token].ttype == SD_tokenType::SD_NUMBER) {
                constant = std::pow(constant, pow);
            } else {
                power = pow;
            }
        } else {
            // Unexpected, return a botched expr with error message
            std::string msg = "Error: Unexpected symbol '";
            msg.append(tokens[curr].literal);
            msg.append("' following '^'. Expected a number as exponent");
            auto botched = std::make_unique<BotchedExpr>(msg);
            botched->kind = SD_ExprKind::BOTCHED;
            return std::move(botched);
        }
        // step past number token
        curr += 1;
    } else if (tokens[curr].ttype == SD_tokenType::SD_VARIABLE) {
        // step past variable token
        curr += 1;
        power = 1;
        if (tokens[curr].ttype == SD_tokenType::SD_CARET) {
            // step past '^' token
            curr += 1;
            if (tokens[curr].ttype == SD_tokenType::SD_NUMBER) {
                power = std::stod(tokens[curr].literal);
            } else {
                // Unexpected, return a botched expr with error message
                std::string msg = "Error: Unexpected symbol '";
                msg.append(tokens[curr].literal);
                msg.append("' following '^'. Expected a number as exponent");
                auto botched = std::make_unique<BotchedExpr>(msg);
                botched->kind = SD_ExprKind::BOTCHED;
                return std::move(botched);
            }
            // step past number token
            curr += 1;
        }
    }

    // Generate and return the Atomic type here.
    constant = negate ? constant * -1 : constant;
    auto atomic = std::make_unique<AtomicExpr>(constant, power);
    atomic->kind = SD_ExprKind::ATOMIC;
    return std::move(atomic);
}

static std::unique_ptr<Expr> parse_unary(std::vector<SD_token> tokens, int& curr) {
    auto negation_count = 0;
    while (tokens[curr].ttype == SD_tokenType::SD_MINUS) {
        negation_count += 1;
        // step past minus token
        curr += 1;
    }

    auto unary = parse_atomic(negation_count % 2, tokens, curr);
    return std::move(unary);
}

static std::unique_ptr<Expr> parse_factor(std::vector<SD_token> tokens, int& curr) {
    auto left = parse_unary(tokens, curr);

    if (left->kind == SD_ExprKind::BOTCHED) return std::move(left);

    while (tokens[curr].ttype == SD_tokenType::SD_MULTIPLY) {
        // step past the '*' token
        curr += 1;
        auto right = parse_unary(tokens, curr);
        if (right->kind == SD_ExprKind::BOTCHED) return std::move(right);
        auto binary = new BinExpr('*', std::move(left), std::move(right));
        binary->kind = SD_ExprKind::BINARY;
        left.reset(binary);
    }

    return std::move(left);
}

static std::unique_ptr<Expr> parse_term(std::vector<SD_token> tokens, int& curr) {
    std::unique_ptr<Expr> left = parse_factor(tokens, curr);

    if (left->kind == SD_ExprKind::BOTCHED) return std::move(left);

    while (tokens[curr].ttype == SD_tokenType::SD_MINUS ||
           tokens[curr].ttype == SD_tokenType::SD_PLUS) 
    {
        char op = *(tokens[curr].literal.data());
        // step past the '+' or '-' token
        curr += 1;
        auto right = parse_factor(tokens, curr);
        if (right->kind == SD_ExprKind::BOTCHED) return std::move(right);
        //auto binary = std::make_unique<BinExpr>(op, std::move(left), std::move(right));
        auto binary = new BinExpr(op, std::move(left), std::move(right));
        binary->kind = SD_ExprKind::BINARY;
        left.reset(binary);
    }

    return std::move(left);
}

static std::unique_ptr<Expr> parse_binary(std::vector<SD_token> tokens, int& curr) {
    std::unique_ptr<Expr> binary = parse_term(tokens, curr);
    if (tokens[curr].ttype != SD_tokenType::SD_ENDEXPR) {
        std::string msg = "Error: Unexpected symbol '";
        msg.append(tokens[curr].literal);
        msg.append("'. Expected the implicit end-of-expression token");
        auto botched = new BotchedExpr(msg);
        botched->kind = SD_ExprKind::BOTCHED;
        binary.reset(botched);
    }
    // step past end-of-expr token
    curr += 1;
    return std::move(binary);
}

std::unique_ptr<Expr> SD_parse_to_ast(std::vector<SD_token> tokens, bool print_ast) {
    int curr = 0;
    auto ast = std::move(parse_binary(tokens, curr));
    if (print_ast) {
        if (ast->kind == SD_ExprKind::ATOMIC) {
            AtomicExpr* atomic = static_cast<AtomicExpr*>(ast.get());
            std::cout << "[" << atomic->constant << ", " << atomic->power << "]\n";
        } else if (ast->kind == SD_ExprKind::BINARY) {
            BinExpr* binary = static_cast<BinExpr*>(ast.get());
            binary->print_as_ast();
            std::cout << "\n";
        }
    }
    return std::move(ast);
}