#include "Parameter.hpp"

using namespace std;

void
Parameter::readParam(std::string file_name)
{
    //Opening input file
    ifstream input_file(file_name, ios::in);
    assert(input_file.is_open() && "ERROR: Couldn't open Graph.def file");

    //Reading comments
    bool read_comment = false;

    //Defining comment regex
    regex reg_start_comment("^/\\*.*$");
    regex reg_end_comment("^.*\\*/$");
    //regex reg_whole_comment("^/\\*.*\\*/$");
    regex reg_whole_comment("/\\*(?:.|[\\n\\r])*?\\*/");
    regex reg_single_comment("^//.*$");
    regex reg_param_start("^param.*$");
    regex reg_empty_line("^\\s*$");
    regex reg_capture_param("^.*?\\<(.*),");
    regex reg_capture_value("^.*?\\<.*,\\s*(.*)\\>");

    smatch param_match;
    smatch value_match;

    stringstream comment;

    for( string line; getline(input_file, line); ){

        //Checking for batch comment in single line
        if(regex_match(line, reg_whole_comment))
            comment << line;
        //Check for single line comment
        else if(regex_match(line, reg_single_comment))
            comment << line;
        //Check if commenting block has been finished 
        else if(regex_match(line, reg_end_comment)){
            read_comment = false;
            comment << line;
            //cout << comment.str() << endl;
            comment.clear();
        }
        //Check to see if the commenting block has been started
        else if(regex_match(line, reg_start_comment)){
            read_comment = true;
            comment << line;
        }
        //Continue reading comment
        else if(read_comment)
            comment << endl << line;
        //skip empty line
        else if(regex_match(line, reg_empty_line))
            continue;
        else{
            //Comment lines they have been finished
            //Check whether doc is in a right format
            assert(regex_match(line, reg_param_start) && "ERROR: PARAM FILE WRONG FORMAT");

            regex_search(line, param_match, reg_capture_param);
            regex_search(line, value_match, reg_capture_value);
            param_set[param_match[1]] = stoi(value_match[1]);

        }

    }

}

