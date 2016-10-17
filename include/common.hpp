#ifndef __COMMON__
#define __COMMON__

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <cassert>
#include <queue>
#include <map>
#include <set>
#include <list>
#include <cmath>
#include <unordered_map>
#include "dbgmsh.hpp"

//Defining path variables
extern const std::string core_param_path;
extern const std::string graph_param_path;
extern const std::string llvm_instruction_path;

#define MAX_NUM_CHAIN 700
#define GRAPH_LIMIT 300
#define LANE_DEFAULT_SIZE 300

#endif
