
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include "parser.h"

using namespace std;

Parser::Parser() : next_available(0), current_input_index(0), current_term_list(nullptr), current_coefficient(1) {}

int Parser::evaluate_primary(const Primary* primary, const std::vector<std::string>& params, const std::vector<int>& args) {
    if (!primary) return 0;
    if (primary->kind == VAR) {
        if (primary->var >= 0 && primary->var < static_cast<int>(args.size())) {
            return args[primary->var];
        }
        return 0;
    } else {
        return evaluate_term_list(primary->t_list, params, args);
    }
}

int Parser::evaluate_monomial(const Monomial& monomial, const std::vector<std::string>& params, const std::vector<int>& args) {
   if (!monomial.primary) return 1;
    
    // Get the base value
    int base = evaluate_primary(monomial.primary, params, args);
    
    // Apply exponent
    int result = 1;
    for (int i = 0; i < monomial.exponent; i++) {
        result *= base;
    }
    return result;
}


int Parser::evaluate_monomial_list(const struct monomial_list* list, const std::vector<std::string>& params, const std::vector<int>& args) {
    if (!list) return 1;
    
    // Multiply current monomial value with rest of the list
    int current = evaluate_monomial(list->monomial, params, args);
    int rest = evaluate_monomial_list(list->next, params, args);
    return current * rest;
    }

int Parser::evaluate_term(const Term& term, const std::vector<std::string>& params, const std::vector<int>& args) {
    if (term.is_constant) {
        return term.coefficient;
    }
    
    int result = term.coefficient;  // Start with coefficient
    
    if (!term.var.empty()) {
        // Handle variable terms
        auto it = std::find(params.begin(), params.end(), term.var);
        if (it != params.end()) {
            int index = std::distance(params.begin(), it);
            if (index < args.size()) {
                int base = args[index];
                int powered = 1;
                // Apply exponent
                for (int i = 0; i < term.exponent; i++) {
                    powered *= base;
                }
                result *= powered;
            }
        }
    } else if (term.monomial_list) {
        // Handle complex terms
        result *= evaluate_monomial_list(term.monomial_list, params, args);
    }
    
    return result;
}
int Parser::evaluate_term_list(const struct term_list* list, const std::vector<std::string>& params, const std::vector<int>& args) {
    if (!list) return 0;
    
    int term_val = evaluate_term(list->term, params, args);
    
    if (!list->next) return term_val;
    
    int next_val = evaluate_term_list(list->next, params, args);
    return list->op == OP_PLUS ? term_val + next_val : term_val - next_val;
}
                
//polynomial evaluation
int Parser::evaluate_polynomial(const std::string& poly_name, const std::vector<int>& args) {

     for (auto& p : parsed_polynomials) {
        if (p.name == poly_name) {
            int result = 0;
            for (const auto& term : p.terms) {
                result += evaluate_term(term, p.params, args);
            }
            return result;
        }
    }
    return 0;

}

void Parser::execute_program() {
  mem.resize(1000, 0);
    current_input_index = 0;
    
    for (const auto& inst : instructions) {
        switch (inst.type) {
            case Instruction::INPUT: {
                for (const auto& var : symbol_table) {
                    if (var.name == inst.var_name) {
                        mem[var.location] = input_values[current_input_index++];
                        break;
                    }
                }
                break;
            }
            case Instruction::OUTPUT: {
                for (const auto& var : symbol_table) {
                    if (var.name == inst.var_name) {
                        std::cout << mem[var.location] << std::endl;
                        break;
                    }
                }
                break;
            }
            case Instruction::EVAL: {
                std::vector<int> arg_values;
                for (const auto& arg_name : inst.eval.arg_vars) {
                    for (const auto& var : symbol_table) {
                        if (var.name == arg_name) {
                            arg_values.push_back(mem[var.location]);
                            break;
                        }
                    }
                }
                
                for (const auto& var : symbol_table) {
                    if (var.name == inst.eval.target_var) {
                        mem[var.location] = evaluate_polynomial(inst.eval.poly_name, arg_values);
                        break;
                    }
                }
                break;
            }
        }
    }
}


void Parser::syntax_error()
{
    cout << "SYNTAX ERROR !!!!!&%!!\n";
    exit(1);
}

Token Parser::expect(TokenType expected_type)
{
    Token t = lexer.GetToken();
    if (t.token_type != expected_type)
        syntax_error();
    return t;
}
// Implementation of allocate_variable
int Parser::allocate_variable(const std::string& var_name) {
    // Check if variable already exists
    for (const auto& var : symbol_table) {
        if (var.name == var_name) {
            return var.location;
        }
    }
    
    // Variable doesn't exist, allocate new location
    VariableInfo new_var;
    new_var.name = var_name;
    new_var.location = next_available++;
    symbol_table.push_back(new_var);
    
    return new_var.location;
}

// Implementation of print_symbol_table
void Parser::print_symbol_table()const
 {
    std::cout << "\nSymbol Table Contents:" << std::endl;
    std::cout << "Variable\tLocation" << std::endl;
    std::cout << "--------\t--------" << std::endl;
    for (const auto& var : symbol_table) {
        std::cout << var.name << "\t\t" << var.location << std::endl;
    }
}
//storing input from num_list parsing 
void Parser::store_input_value(const std::string& num_lexeme) {
    input_values.push_back(std::atoi(num_lexeme.c_str()));
}
// Get next input value (for use during execution)
int Parser::get_next_input() {
    if (current_input_index < input_values.size()) {
        return input_values[current_input_index++];
    }
    // Handle error case - not enough inputs
    std::cout << "Error: Not enough input values\n";
    exit(1);
}

// Debug function to print input values
void Parser::print_input_values() {
    std::cout << "\nStored Input Values:" << std::endl;
    for (size_t i = 0; i < input_values.size(); i++) {
        std::cout << i << ": " << input_values[i] << std::endl;
    }
}


// Parsing
void Parser::ConsumeAllInput()
{
    parse_tasks_section();
    parse_poly_section();
    parse_execute_section();
    parse_inputs_section();

    expect(END_OF_FILE);

    //reporting error at the end
    if (!semantic_error.lines.empty()) {
        semantic_error.reportError(1);
    }
    if (!semantic_error2.lines.empty()) {
        semantic_error2.reportError(2);
    }
    if (!semantic_error3.lines.empty()) {
        semantic_error3.reportError(3); 
    }
    if (!semantic_error4.lines.empty()) {
        semantic_error4.reportError(4); 
    }

}

void Parser::parse_tasks_section()
{
    expect(TASKS);
    parse_num_list();
}

void Parser::parse_num_list()
{
    Token t = expect(NUM);
    // Only store if we're in INPUTS section
    if (in_inputs_section) {  // Add this as a boolean member variable
        store_input_value(t.lexeme);
    }


    Token next = lexer.peek(1);
    if (next.token_type == NUM) {
        parse_num_list();
    }
}

//parse_poly_section -> parse_poly_dec_list
void Parser::parse_poly_section()
{
    expect(POLY);
    parse_poly_decl_list();
}

//parse_poly_decl_list -> parse_poly_decl ->(recursively parse_poly_decl)
void Parser::parse_poly_decl_list()
{
    parse_poly_decl();
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        parse_poly_decl_list();
    }
}

//Semantic Error : reporting error to this function
void SemanticError::reportError(int code) {
    if (lines.empty()) return;
    
    // Sort line numbers as required by project spec
    std::sort(lines.begin(), lines.end());
    
    std::cout << "Semantic Error Code " << code << ": ";
    for (size_t i = 0; i < lines.size(); i++) {
        if (i > 0) std::cout << " ";
        std::cout << lines[i];
    }
    std::cout << std::endl;
    exit(1);
}
//error 1 :adding duplicate checking function:
void Parser::check_duplicate_polynomial(const std::string& name, int line_no) {
   // bool found_duplicate = false;

     for(const auto& poly : polynomial_table) {
        if(poly.name == name) {
            semantic_error.lines.push_back(line_no);
          //  found_duplicate = true;
          //semantic_error.reportError(1);
           break;
            // Don't exit immediately - continue checking for more duplicates
        }
    }
    
}
//error 2: Checking valid and invalid monomial
bool Parser::is_valid_monomial(const std::string& monomial_name, const PolynomialDecl& poly) {
    if (!poly.has_explicit_params) {
        return monomial_name == "x";
    }
    return std::find(poly.parameters.begin(), poly.parameters.end(), monomial_name) != poly.parameters.end();
}

void Parser::check_invalid_monomial(const std::string& monomial_name, const PolynomialDecl& current_poly, int line_no) {
    if (!is_valid_monomial(monomial_name, current_poly)) {
        semantic_error2.lines.push_back(line_no);
    }
}
//error 3 : checking undeclared polynomial evaluations
void Parser::check_undeclared_polynomial(const std::string& name, int line_no)
{
    bool found = false;
    for (const auto& poly : polynomial_table) {
        if (poly.name == name) {
            found = true;
            break;
        }
    }
    if (!found) {
        semantic_error3.lines.push_back(line_no);
    }
}
//error 4: checking wrong numbner of agruments
void Parser::check_wrong_number_of_arguments(const std::string& name, int line_no, int get_num)
{
    for (const auto& poly : polynomial_table) {
        if (poly.name == name) {
            if (poly.parameters.size() != get_num) {
                semantic_error4.lines.push_back(line_no);
            }
            break;
        }
    }
}

//parse_poly_decl -> parse_poly_header -> parse_poly_body
void Parser::parse_poly_decl()
{   
    Token name_token = lexer.peek(1);  
    check_duplicate_polynomial(name_token.lexeme, name_token.line_no);

    // Create and store polynomial information
    current_poly = ParsedPolynomial();  // Reset current polynomial
    current_poly.name = name_token.lexeme;
    current_coefficient = 1;  // Reset coefficient

    PolynomialDecl new_poly;
    new_poly.name = name_token.lexeme;
    new_poly.line_no = name_token.line_no;
    polynomial_table.push_back(new_poly);

    //normal parsing
    parse_poly_header();
    expect(EQUAL);
    parse_poly_body();
    expect(SEMICOLON);

 // After successful parsing, store the polynomial
    current_poly.params = polynomial_table.back().parameters;
    parsed_polynomials.push_back(current_poly);

}
void Parser::store_polynomial_info() {
    ParsedPolynomial poly;
    poly.name = polynomial_table.back().name;
    poly.params = polynomial_table.back().parameters;
    
    // Store terms from current polynomial
    parsed_polynomials.push_back(poly);
}

void Parser::store_term(int coef, const std::string& var, int exp) {
    Term term;
    term.coefficient = coef;
    term.var = var;
    term.exponent = exp;
    term.is_constant = var.empty();
    parsed_polynomials.back().terms.push_back(term);
}

//parse_poly_header -> parse_poly_name -> (optionally parse_id_list)
void Parser::parse_poly_header()
{
    
    parse_poly_name();
    Token t = lexer.peek(1);
    
    if (t.token_type == LPAREN) {
        polynomial_table.back().has_explicit_params = true;
        expect(LPAREN);
        parse_id_list(polynomial_table.back().parameters);
        expect(RPAREN);
    } else {
        polynomial_table.back().has_explicit_params = false;
        polynomial_table.back().parameters.clear();  // Clear any existing parameters
        polynomial_table.back().parameters.push_back("x");
    }
}

//parse_id_list -> expect(ID) -> (recursively parse_id_list if more IDs)
void Parser::parse_id_list(std::vector<std::string>& params)
{
    Token t = expect(ID);
    params.push_back(t.lexeme);


    Token next = lexer.peek(1);
    if (next.token_type == COMMA) {
        expect(COMMA);
        parse_id_list(params);
    }
    else if (next.token_type != RPAREN) {
        // If next token is not a comma or right paren, it's a syntax error
        syntax_error();
    }
}

//parse_poly_name -> expect(ID)
void Parser::parse_poly_name()
{
    expect(ID);
}

//parse_poly_body -> parse_term_list
void Parser::parse_poly_body()
{   
    parse_term_list();

}

//parse_term_list -> parse_term -> (optionally parse_add_operator and recursively parse_term_list)
void Parser::parse_term_list()
{   
    parse_term();
    Token t = lexer.peek(1);
    if (t.token_type == PLUS || t.token_type == MINUS) {

        parse_add_operator();
        parse_term_list();
        
    }
}

//parse_term -> (optionally parse_coefficient) -> (optionally parse_monomial_list)
void Parser::parse_term()
{   
  Token t = lexer.peek(1);
    
    if (t.token_type == NUM) {
        parse_coefficient();
        t = lexer.peek(1);
        if (t.token_type == ID || t.token_type == LPAREN) {
            parse_monomial_list();
        }
    } else {
        current_coefficient = 1;  // Default coefficient
        parse_monomial_list();
    }
}

//parse_monomial_list -> parse_monomial -> (recursively parse_monomial_list if more monomials)
void Parser::parse_monomial_list()
{
    parse_monomial();
    Token t = lexer.peek(1);
    if (t.token_type == ID || t.token_type == LPAREN) {
        parse_monomial_list();
    }
}

//parse_monomial -> parse_primary -> (optionally parse_exponent)
void Parser::parse_monomial()
{
    parse_primary();
    Token t = lexer.peek(1);
    if (t.token_type == POWER) {
        parse_exponent();
    }
}

//parse_primary -> (either expect(ID) or parse parenthesized term_list)
void Parser::parse_primary()
{
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        Token id_token = expect(ID);

        // Check for invalid monomial without triggering syntax error
        if (!polynomial_table.empty()) {  
            check_invalid_monomial(id_token.lexeme, polynomial_table.back(), id_token.line_no);
        } 
        Term term;
        term.coefficient = current_coefficient;  // Use current coefficient
        term.var = id_token.lexeme;
        term.exponent = 1;  // Default exponent
        term.is_constant = false;
        current_poly.terms.push_back(term);
    

    //normal parse
        } else if (t.token_type == LPAREN) {
        expect(LPAREN);
        parse_term_list();
        expect(RPAREN);
    } else {
        syntax_error();
    }
}


void Parser::parse_exponent()
{
    expect(POWER);
    Token t = expect(NUM);

    if (!current_poly.terms.empty()) {
        current_poly.terms.back().exponent = std::atoi(t.lexeme.c_str());
    }
}

void Parser::parse_add_operator()
{
    Token t = lexer.GetToken();
    if (t.token_type != PLUS && t.token_type != MINUS) {
        syntax_error();
    }
    if (t.token_type == MINUS && current_poly.terms.size() > 0) {
        current_poly.terms.back().coefficient *= -1;
    }
}

void Parser::parse_coefficient()
{
   Token  t =  expect(NUM);
  current_coefficient = std::atoi(t.lexeme.c_str());
  // Store as constant term if no variable follows
    if (lexer.peek(1).token_type != ID && lexer.peek(1).token_type != LPAREN) {
        Term term;
        term.coefficient = current_coefficient;
        term.is_constant = true;
        term.exponent = 0;
        current_poly.terms.push_back(term);
    }
  }

// parse_execute_section -> parse_statement_list
void Parser::parse_execute_section()
{
    expect(EXECUTE);
    parse_statement_list();
}

//parse_statement_list -> parse_statement -> (recursively parse_statement_list if more statements)
void Parser::parse_statement_list()
{
    parse_statement();
    Token t = lexer.peek(1);
    if (t.token_type == INPUT || t.token_type == OUTPUT || t.token_type == ID) {
        parse_statement_list();
    }
}

void Parser::parse_statement()
{
    Token t = lexer.peek(1);
    if (t.token_type == INPUT) {
        parse_input_statement();
    } else if (t.token_type == OUTPUT) {
        parse_output_statement();
    } else if (t.token_type == ID) {
        parse_assign_statement();
    } else {
        syntax_error();
    }
}

void Parser::parse_input_statement()
{
    expect(INPUT);
    Token var_token = expect(ID);
    expect(SEMICOLON);
    allocate_variable(var_token.lexeme);
    
    // Store instruction
    Instruction inst;
    inst.type = Instruction::INPUT;
    inst.var_name = var_token.lexeme;
    instructions.push_back(inst);

    
   
}

void Parser::parse_output_statement()
{
    expect(OUTPUT);
    Token var_token = expect(ID);
    expect(SEMICOLON);

    // Store instruction
    Instruction inst;
    inst.type = Instruction::OUTPUT;
    inst.var_name = var_token.lexeme;
    instructions.push_back(inst);

  
}

void Parser::parse_assign_statement()
{
    Token target = expect(ID);
    expect(EQUAL);
    Token poly_name = lexer.peek(1);  // Just peek without consuming
    parse_poly_evaluation();
    expect(SEMICOLON);

    // Add instruction after successful parsing
    Instruction inst;
    inst.type = Instruction::EVAL;
    inst.eval.target_var = target.lexeme;
    inst.eval.poly_name = poly_name.lexeme;
    inst.eval.arg_vars = current_args;  // Store collected arguments
    instructions.push_back(inst);
    allocate_variable(target.lexeme);
    
    current_args.clear();

}

void Parser::parse_poly_evaluation()
{
   // parse_poly_name();
   Token name_token = expect(ID);
    check_undeclared_polynomial(name_token.lexeme, name_token.line_no); // Check for undeclared polynomial
    expect(LPAREN);
    int get_num = parse_argument_list();
    expect(RPAREN);
    check_wrong_number_of_arguments(name_token.lexeme, name_token.line_no, get_num); // Check for wrong number of arguments
}

int Parser::parse_argument_list()
{   
    int count = 1;
   parse_argument();
    Token t = lexer.peek(1);
    if (t.token_type == COMMA) {
        expect(COMMA);
        count += parse_argument_list();
    }
    
    return count;
}

void Parser::parse_argument()
{
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        Token t2 = lexer.peek(2);
        if (t2.token_type == LPAREN){
            parse_poly_evaluation();
        }
        else{
            Token arg = expect(ID);
            current_args.push_back(arg.lexeme);
            }
        
    } else if (t.token_type == NUM) {
        expect(NUM);
    } else {
        parse_poly_evaluation();
         //syntax_error();
    }
}

void Parser::parse_inputs_section()
{
    expect(INPUTS);
    in_inputs_section = true;
    parse_num_list();
    in_inputs_section = false;
}



int main()
{
    // note: the parser class has a lexer object instantiated in it. You should not be declaring
    // a separate lexer object. You can access the lexer object in the parser functions as shown in the
    // example method Parser::ConsumeAllInput
    // If you declare another lexer object, lexical analysis will not work correctly
    Parser parser;
    parser.ConsumeAllInput();
    //int evaluate_polynomial(const std::string& poly_name, const std::vector<int>& args);
   
    parser.execute_program();
    parser.print_symbol_table();
    parser.print_input_values();

    return 0;

}
