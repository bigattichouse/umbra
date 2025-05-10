/**
 * @file test_select.c
 * @brief Test SELECT query functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/parser/lexer.h"
#include "../src/parser/select_parser.h"
#include "../src/parser/ast.h"
#include "../src/kernel/kernel_generator.h"
#include "../src/kernel/filter_generator.h"
#include "../src/query/query_executor.h"

/**
 * @brief Test lexer with SELECT statement
 */
static void test_lexer(void) {
    printf("Testing lexer...\n");
    
    const char* sql = "SELECT name, age FROM users WHERE age > 21";
    Lexer lexer;
    lexer_init(&lexer, sql);
    
    Token token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_SELECT);
    
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_IDENTIFIER);
    assert(strcmp(token.value, "name") == 0);
    
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_COMMA);
    
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_IDENTIFIER);
    assert(strcmp(token.value, "age") == 0);
    
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_FROM);
    
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_IDENTIFIER);
    assert(strcmp(token.value, "users") == 0);
    
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_WHERE);
    
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_IDENTIFIER);
    assert(strcmp(token.value, "age") == 0);
    
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_GREATER);
    
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_NUMBER);
    assert(strcmp(token.value, "21") == 0);
    
    token = lexer_next_token(&lexer);
    assert(token.type == TOKEN_EOF);
    
    lexer_free(&lexer);
    printf("Lexer test passed!\n");
}

/**
 * @brief Test parser with SELECT statement
 */
static void test_parser(void) {
    printf("Testing parser...\n");
    
    const char* sql = "SELECT name, age FROM users WHERE age > 21";
    Lexer lexer;
    lexer_init(&lexer, sql);
    
    Parser parser;
    parser_init(&parser, &lexer);
    
    SelectStatement* stmt = parse_select_statement(&parser);
    assert(stmt != NULL);
    
    // Check FROM clause
    assert(strcmp(stmt->from_table->table_name, "users") == 0);
    
    // Check SELECT list
    assert(stmt->select_list.count == 2);
    assert(stmt->select_list.expressions[0]->type == AST_COLUMN_REF);
    assert(strcmp(stmt->select_list.expressions[0]->data.column.column_name, "name") == 0);
    assert(stmt->select_list.expressions[1]->type == AST_COLUMN_REF);
    assert(strcmp(stmt->select_list.expressions[1]->data.column.column_name, "age") == 0);
    
    // Check WHERE clause
    assert(stmt->where_clause != NULL);
    assert(stmt->where_clause->type == AST_BINARY_OP);
    assert(stmt->where_clause->data.binary_op.op == OP_GREATER);
    
    free_select_statement(stmt);
    parser_free(&parser);
    lexer_free(&lexer);
    
    printf("Parser test passed!\n");
}

/**
 * @brief Test kernel generation
 */
static void test_kernel_generator(void) {
    printf("Testing kernel generator...\n");
    
    // Create a simple schema
    TableSchema schema;
    strcpy(schema.name, "users");
    schema.column_count = 2;
    schema.columns = malloc(2 * sizeof(ColumnDefinition));
    
    strcpy(schema.columns[0].name, "name");
    schema.columns[0].type = TYPE_VARCHAR;
    schema.columns[0].length = 255;
    
    strcpy(schema.columns[1].name, "age");
    schema.columns[1].type = TYPE_INT;
    
    // Create a simple SELECT statement
    SelectStatement* stmt = create_select_statement();
    stmt->select_list.has_star = true;
    
    stmt->from_table = malloc(sizeof(TableRef));
    stmt->from_table->table_name = strdup("users");
    stmt->from_table->alias = NULL;
    
    // Generate kernel
    GeneratedKernel* kernel = generate_select_kernel(stmt, &schema, "./test_db");
    assert(kernel != NULL);
    assert(kernel->code != NULL);
    assert(kernel->kernel_name != NULL);
    
    printf("Generated kernel:\n%s\n", kernel->code);
    
    free_generated_kernel(kernel);
    free_select_statement(stmt);
    free(schema.columns);
    
    printf("Kernel generator test passed!\n");
}

/**
 * @brief Main test function
 */
int main(void) {
    printf("Running SELECT query tests...\n\n");
    
    test_lexer();
    test_parser();
    test_kernel_generator();
    
    printf("\nAll tests passed!\n");
    return 0;
}
