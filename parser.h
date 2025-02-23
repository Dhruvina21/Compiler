#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <exception>
#include <algorithm>
#include "lexer.h"


// Enums for different types
enum PrimaryKind {
    VAR,
    TERM_LIST
};

enum OpType {
    OP_PLUS,
    OP_MINUS
};

// Forward declarations for linked structures
struct term_list;
struct monomial_list;

// Primary represents either a variable or a nested term list
struct Primary {
    PrimaryKind kind;      // VAR or TERM_LIST
    int var;              // Used when kind is VAR - index in parameter list
    struct term_list* t_list;  // Used when kind is TERM_LIST
};

// Represents one monomial (e.g., x^2, yz, etc.)
struct Monomial {
    Primary* primary;
    int exponent;    // Power to which the primary is raised
};

// List of monomials (e.g., x^2 y^3)
struct monomial_list {
    Monomial monomial;
    struct monomial_list* next;
};

// Represents a term (e.g., 2x^2, -3xy, etc.)
struct Term {
    int coefficient;
    bool is_constant;
    std::string var;     // Variable name for single var terms
    int exponent;        // Exponent for single var terms
    struct monomial_list* monomial_list;  // For complex terms
};

// List of terms with operators between them
struct term_list {
    Term term;
    OpType op;          // Operator connecting to next term
    struct term_list* next;
    struct monomial_list* monomial_list;

};


//structure for polynomial tracking
struct PolynomialDecl {
  //checking for Error 1:
    std::string name;
    int line_no;
    //checking for error 2:
    std::vector<std::string> parameters;  
    bool has_explicit_params; 
    std::vector<Term> terms;
};

// structure for error reporting
struct SemanticError {
    std::vector<int> lines;
    void reportError(int code);
    bool has_errors;
};

struct VariableInfo{
  int location;
  std::string name;
};

struct PolyEvaluation {
    std::string target_var;
    std::string poly_name;
    std::vector<std::string> arg_vars;
};

//structure for instruction
struct Instruction {
    enum Type {
        INPUT,
        OUTPUT,
        EVAL
    } type;
    std::string var_name;
    PolyEvaluation eval;
};

struct EvalInfo {
    std::string target_var;
    std::string poly_name;
    std::vector<std::string> arguments;
};

struct ParsedPolynomial {
    std::string name;                  // Name of polynomial
    std::vector<Term> terms;          // List of terms
    std::vector<std::string> params;  // Parameters (if any)
};

class SyntaxError : public std::exception {
    public:
        SyntaxError() {}
};

class Parser {
  public:
     void ConsumeAllInput();
    Parser();
    void print_symbol_table() const;
    void print_input_values();
    void store_input_value(const std::string& num_lexeme);
     int evaluate_polynomial(const std::string& poly_name, const std::vector<int>& args);
    void execute_program();


  private:
    LexicalAnalyzer lexer;
    void syntax_error();
    Token expect(TokenType expected_type);
    struct term_list* current_term_list;

    bool tasks[7] = {false}; 
    void processTaskNumber(int num); 
    void executeAllTasks();
    
//task 3 tracking initialized variable
    std::set<std::string> initialized_vars;
     std::vector<int> warning_lines;
    
    // Helper functions for warning code 1
    void mark_variable_initialized(const std::string& var_name);
    void check_argument_initialization(const std::string& arg_name, int line_no);
    void report_warning_code_1();

    // Storage
    std::vector<VariableInfo> symbol_table;
    std::vector<int> mem;
    std::vector<int> input_values;
    std::vector<Instruction> instructions;
    std::vector<PolynomialDecl> polynomial_table;
    std::vector<std::string> current_args;
    std::vector<ParsedPolynomial> parsed_polynomials;

    
    int current_coefficient = 1;
    ParsedPolynomial current_poly;
   
    bool in_inputs_section = false;

    // Counters
    int next_available;
    int current_input_index;
    

    // fucntions for task 2
    int allocate_variable(const std::string& var_name);
    int get_next_input();
   
    void store_poly_eval_instruction(const std::string& target_var, const std::string& poly_name, const std::vector<std::string>& args);
    void store_polynomial_info() ;
    void store_term(int coef, const std::string& var, int exp);
    //data members for semantic checking
    //std::vector<int> duplicate_lines;
    
    SemanticError semantic_error;// Error 1
    SemanticError semantic_error2; // Error 2
    SemanticError semantic_error3; // Error 3
    SemanticError semantic_error4; // Error 3
  

  //polynomial evaluation
  int evaluate_primary(const Primary* primary, const std::vector<std::string>& params, const std::vector<int>& args);
    int evaluate_monomial(const Monomial& monomial, const std::vector<std::string>& params, const std::vector<int>& args);
    int evaluate_monomial_list(const struct monomial_list* list, const std::vector<std::string>& params, const std::vector<int>& args);
    int evaluate_term(const Term& term, const std::vector<std::string>& params, const std::vector<int>& args);
    int evaluate_term_list(const struct term_list* list, const std::vector<std::string>& params, const std::vector<int>& args);
   

    //funtion for Semantic errors
    void check_duplicate_polynomial(const std::string& name, int line_no);
    void check_invalid_monomial(const std::string& monomial_name, const PolynomialDecl& current_poly, int line_no);
    bool is_valid_monomial(const std::string& monomial_name, const PolynomialDecl& poly);
    void check_undeclared_polynomial(const std::string& name, int line_no);
    void check_wrong_number_of_arguments(const std::string& name, int line_no, int get_num);

    void parse_tasks_section();
        void parse_num_list();
        void parse_poly_section();
        void parse_poly_decl_list();
        void parse_poly_decl();
        void parse_poly_header();
        void parse_id_list(std::vector<std::string>& params);
        void parse_poly_name();
        void parse_poly_body();
        void parse_term_list();
        void parse_term();
        void parse_monomial_list();
        void parse_monomial();
        void parse_primary();
        void parse_exponent();
        void parse_add_operator();
        void parse_coefficient();
        void parse_execute_section();
        void parse_statement_list();
        void parse_statement();
        void parse_input_statement();
        void parse_output_statement();
        void parse_assign_statement();
        void parse_poly_evaluation();
        int parse_argument_list();
        void parse_argument();
        void parse_inputs_section();
};

#endif 