#include "Instruction.hpp"
using namespace std;

/**
 * Reading Instruction file format
 */
void
Instruction::readInst(std::string file_name)
{
    //Opening input file
    ifstream input_file(file_name, ios::in);
    assert(input_file.is_open() && "ERROR: Couldn't open Instruction.def file");

    //Reading comments
    //
    //Defining comment regex
    regex reg_start_comment("^/\\*.*$");
    regex reg_end_comment("^.*\\*/$");
    regex reg_whole_comment("/\\*(?:.|[\\n\\r])*?\\*/");
    regex reg_single_comment("^//.*$");

    //start reading instructions
    //regex reg_start_line("^\\s*.*\\s*\\(\\s*\\d+,\\s*([^\\s]+)\\s*,\\s*.*\\).*$");
    regex reg_start_line("^\\s*.*\\s*\\(\\s*\\d+,\\s*([^\\s]+)\\s*,\\s*.*,\\s*([^\\s]+)\\s*.*\\).*$");
    regex reg_empty_line("^\\s*$");

    smatch pattern_match;

    stringstream comment;

    for( string line; getline(input_file, line); ){

        if(regex_search(line,pattern_match, reg_start_line))
            instruction_llvm[pattern_match[1]] = std::stoi(pattern_match[2]);
    }
}
