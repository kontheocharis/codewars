#include <memory>
#include <iostream>
#include <assert.h>
#include <string>
#include <vector>

enum class TokenType
{
    MUL,
    DIV,
    ADD,
    SUB,
    LEFT_PAREN,
    RIGHT_PAREN,
    NUMBER
};

struct Token
{
    TokenType type;
    double value; // for number
};

enum class ExprType
{
    NUMBER,
    ADD,
    SUB,
    MUL,
    DIV,
};

struct Expr
{
    ExprType type;
    union {
        double number;

        struct
        {
            std::unique_ptr<Expr> ex1;
            std::unique_ptr<Expr> ex2;
        } binary;
    };

    Expr() {}
    ~Expr() {}

    Expr(ExprType type, double value)
    {
        this->type = type;
        this->number = value;
    }

    Expr(ExprType type, std::unique_ptr<Expr> ex1, std::unique_ptr<Expr> ex2)
    {
        this->type = type;
        this->binary.ex1 = std::move(ex1);
        this->binary.ex2 = std::move(ex2);
    }
};


std::ostream& operator<<(std::ostream& stream, const Expr& ex)
{
    switch (ex.type) {
    case ExprType::NUMBER:
        stream << "Number(" << ex.number << ")";
        break;
    case ExprType::MUL:
        stream << "Mul(" << *ex.binary.ex1 << ", " << *ex.binary.ex2 << ")";
        break;
    case ExprType::DIV:
        stream << "Div(" << *ex.binary.ex1 << ", " << *ex.binary.ex2 << ")";
        break;
    case ExprType::ADD:
        stream << "Add(" << *ex.binary.ex1 << ", " << *ex.binary.ex2 << ")";
        break;
    case ExprType::SUB:
        stream << "Sub(" << *ex.binary.ex1 << ", " << *ex.binary.ex2 << ")";
        break;
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const Token& token)
{
    switch (token.type) {
    case TokenType::NUMBER:
        stream << token.value;
        break;
    case TokenType::MUL:
        stream << " * ";
        break;
    case TokenType::DIV:
        stream << " / ";
        break;
    case TokenType::ADD:
        stream << " + ";
        break;
    case TokenType::SUB:
        stream << " - ";
        break;
    case TokenType::LEFT_PAREN:
        stream << "(";
        break;
    case TokenType::RIGHT_PAREN:
        stream << ")";
        break;
    }

    return stream;
}

std::vector<std::unique_ptr<Token>> lex(const std::string& input)
{
    std::vector<std::unique_ptr<Token>> tokens;
    std::string number;
    auto make_token = [&](TokenType t, double num = 0) {
        tokens.push_back(std::unique_ptr<Token>(new Token { t, num }));
    };

    bool add_extra_paren = false;
    int extra_paren_count = 0;

    for (auto c = input.begin(); c != input.end(); ++c)
    {
        if (std::isdigit(*c) 
                || *c == '.' 
                || (*c == '-' 
                    && std::next(c) != input.end() 
                    && (std::isdigit(*std::next(c)) || *std::next(c) == '(') 
                    && (c == input.begin() || !std::isdigit(*std::prev(c))) 
                    && (tokens.size() == 0 || tokens.back()->type != TokenType::NUMBER))) {

            number += *c;
            continue;
        }

        if (!number.empty()) {
            if (number == "-") number = "-1";
            if (*c == '(') {
                bool div_quirk = tokens.size() != 0 && tokens.back()->type == TokenType::DIV;
                if (div_quirk) {
                    make_token(TokenType::LEFT_PAREN);
                    add_extra_paren = true;
                }
                make_token(TokenType::NUMBER, std::stod(number));
                make_token(TokenType::MUL);
            } else {
                make_token(TokenType::NUMBER, std::stod(number));
            }
            number.clear();
        }

        if (*c == ' ')
            continue;
        if (*c == '*') {
            make_token(TokenType::MUL);
            continue;
        }
        if (*c == '/') {
            make_token(TokenType::DIV);
            continue;
        }
        if (*c == '+') {
            make_token(TokenType::ADD);
            continue;
        }
        if (*c == '-') {
            make_token(TokenType::SUB);
            continue;
        }
        if (*c == '(') {
            if (add_extra_paren) ++extra_paren_count;
            make_token(TokenType::LEFT_PAREN);
            continue;
        }
        if (*c == ')') {
            if (add_extra_paren) --extra_paren_count;
            make_token(TokenType::RIGHT_PAREN);
            if (add_extra_paren && extra_paren_count == 0) {
                make_token(TokenType::RIGHT_PAREN);
                add_extra_paren = false;
                extra_paren_count = 0;
            }
            continue;
        }

        assert(0);
    }

    if (!number.empty()) {
        make_token(TokenType::NUMBER, std::stod(number));
    }

    return tokens;
}


std::unique_ptr<Expr> parse(std::vector<std::unique_ptr<Token>>::iterator begin,
                                  std::vector<std::unique_ptr<Token>>::iterator end)
{
    // NUMBER
    if (std::next(begin) == end && (*begin)->type == TokenType::NUMBER) {
        return std::make_unique<Expr>(ExprType::NUMBER, (*begin)->value);
    }

    // PARENS
    if ((*begin)->type == TokenType::LEFT_PAREN && (*std::prev(end))->type == TokenType::RIGHT_PAREN) {
        int left_paren_counter = 0;
        int right_paren_counter = 0;
        bool some_found = false;
        bool is_lone_paren = true;

        for (auto t = begin; t != end; ++t) {
            TokenType token_type = (*t)->type;

            if (some_found && (left_paren_counter == right_paren_counter)) {
                is_lone_paren = false;
            } 
            if (token_type == TokenType::LEFT_PAREN) {
                ++left_paren_counter;
                some_found = true;
            } else if (token_type == TokenType::RIGHT_PAREN) {
                ++right_paren_counter;
                some_found = true;
            }
        }

        if (is_lone_paren) return parse(std::next(begin), std::prev(end));
    }

    // TERMS
    {
        int left_paren_counter = 0;
        int right_paren_counter = 0;
        for (auto token = std::prev(end);; --token) {
            TokenType token_type = (*token)->type;

            if ((token_type == TokenType::ADD || token_type == TokenType::SUB) && left_paren_counter == right_paren_counter) {
                return std::make_unique<Expr>(
                    (token_type == TokenType::ADD) ? ExprType::ADD : ExprType::SUB,
                    parse(begin, token),
                    parse(std::next(token), end));
            } else if (token_type == TokenType::LEFT_PAREN) {
                ++left_paren_counter;
            } else if (token_type == TokenType::RIGHT_PAREN) {
                ++right_paren_counter;
            }
            
            if (token == begin) break;
        }
    }

    // MUL, DIV
    {
        int left_paren_counter = 0;
        int right_paren_counter = 0;
        for (auto token = std::prev(end);; --token) {
            TokenType token_type = (*token)->type;

            if ((token_type == TokenType::MUL || token_type == TokenType::DIV) && left_paren_counter == right_paren_counter) {
                return std::make_unique<Expr>(
                    (token_type == TokenType::MUL) ? ExprType::MUL : ExprType::DIV,
                    parse(begin, token),
                    parse(std::next(token), end));
            } else if (token_type == TokenType::LEFT_PAREN) {
                ++left_paren_counter;
            } else if (token_type == TokenType::RIGHT_PAREN) {
                ++right_paren_counter;
            }

            if (token == begin) break;
        }
    }

    assert(0);
}

std::unique_ptr<Expr> parse(std::vector<std::unique_ptr<Token>>& tokens)
{
    return parse(tokens.begin(), tokens.end());
}

double evaluate(const Expr& expr)
{
    switch (expr.type) {
    case ExprType::NUMBER:
        return expr.number;
    case ExprType::MUL:
        return evaluate(*expr.binary.ex1) * evaluate(*expr.binary.ex2);
    case ExprType::DIV:
        return evaluate(*expr.binary.ex1) / evaluate(*expr.binary.ex2);
    case ExprType::ADD:
        return evaluate(*expr.binary.ex1) + evaluate(*expr.binary.ex2);
    case ExprType::SUB:
        return evaluate(*expr.binary.ex1) - evaluate(*expr.binary.ex2);
    default:
        return 0;
    }
}

int main(int argc, char *argv[])
{
    std::vector<std::unique_ptr<Token>> tokens = lex(argv[1]);
    std::unique_ptr<Expr> exp = parse(tokens);
    double res = evaluate(*exp);
    printf("%f\n", res);
    return 0;
}
