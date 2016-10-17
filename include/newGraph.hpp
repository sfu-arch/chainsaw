#ifndef __NEWGRAPH__
#define __NEWGRAPH__
#include "common.hpp"
#include "Parameter.hpp"
#include "Instruction.hpp"

#include <numeric>
#include <sstream>
#include <iterator>

// Graph vertex's properties
struct vertex_prop {
    uint32_t name;
    std::string label;
    std::string ch;
    std::string opcode;
    std::string color;
    std::string ir;
};

struct vertex_prop_com {
    uint32_t name;
    uint32_t id;
    uint32_t ch;
    std::string label;
    std::string opcode;
    uint32_t latency;
    uint32_t level;
    std::string color;
    std::string ir;
};

// Graph edge's properties
struct edge_prop{
    std::string color;
    std::string style;
};


// Boost graph data type
typedef boost::adjacency_list <boost::listS, boost::vecS, 
        boost::bidirectionalS, vertex_prop, edge_prop> BoostGraph;

typedef boost::graph_traits<BoostGraph>::out_edge_iterator OutEdge;
typedef boost::graph_traits<BoostGraph>::in_edge_iterator InEdge;

typedef boost::graph_traits<BoostGraph>::vertex_iterator VI;


typedef boost::adjacency_list <boost::listS, boost::vecS, 
        boost::bidirectionalS, vertex_prop_com, edge_prop> BoostGraphComp;

// Defining dependencies
struct chainDependency{
    uint32_t source;
    uint32_t target;
};


struct chainGraph{
    uint32_t chainId;
    uint32_t blockID;
    uint32_t size;
    uint32_t live_in;
    uint32_t live_out;
    uint32_t fan_in;
    uint32_t fan_out;
    uint32_t ld_ins;
    uint32_t st_ins;
    uint32_t alloca_ins;
    uint32_t other;
    int32_t level;
    bool    is_mem;
    std::vector<uint64_t> nodes;
    chainGraph():chainId{0},blockID(0), size(0), live_in(0), live_out(0),
        fan_in(0), fan_out(0), ld_ins(0), st_ins(0), alloca_ins(0), other(0), level(-1), is_mem(false){}
};

struct Block{
    uint32_t blkID;
    std::vector<uint32_t> chains;
    Block(uint32_t n):blkID(n){}
};


class newGraph{

    public:
        
        uint32_t first_node_id;
        uint32_t first_ch;


        //A vector containing the chains
        std::vector<chainGraph> chain_graph;

        Parameter graph_parameter;
        Instruction llvm_ins;

        // There are three graphs
        // 1) dirty_graph : the input graph
        // 2) clean_graph : reorderd version of input graph
        // 3) orig_graph  : Only instruction graph
        BoostGraph dirty_graph;
        BoostGraph clean_graph;
        BoostGraph orig_graph;
        BoostGraph ilp_graph;

        //Complete version of the graph
        BoostGraphComp comp_graph;

        // Chains variable
        std::vector<std::vector<uint64_t>> Chains;

        //Graph stats
        uint32_t num_arg;
        uint32_t num_const;
        uint32_t max_ilp;
        float avg_ilp;
        float geo_avg_ilp;
        uint32_t max_mlp;
        uint32_t num_ops;
        uint32_t length;
        uint32_t firstlevelLD;
        int32_t  max_level;
        int32_t  min_level;
        std::vector<uint32_t> ilp;

        //Instruction Histogram
        std::map<std::string, uint32_t> ins_histo;

        //Chain level
        std::map<uint32_t, std::vector<uint32_t>> chain_level;

        //Chain Block
        std::vector<Block> block_chain;

        //Keep the ID mapping
        std::map<uint32_t, uint32_t> orig_map;

    private:

        //Helper Functions
        void cleanReadGraphHelper();
        void insReadGraphHelper();
        void completeReadGraphHelper(std::string str_name);
        void limitingLiveInHelper(BoostGraph& G);
        void limitingLiveOutHelper(BoostGraph& G);
        bool boundaryLiveOutCheckHelper(BoostGraph& G);
        bool boundaryLiveInCheckHelper(BoostGraph& G);
        void cleanRemoveReadGraphHelper();

        void breakSize();

        //Printing stats
        void printChain();
        void printDependencies();
        void printGraphstats(std::string str_name);

        bool checkLiveOutHelper(std::map<uint32_t, std::vector<uint32_t>>& m_live_out);
        bool checkLiveInHelper(std::map<uint32_t, std::vector<uint32_t>>& m_live_in);

        //Decomposing function
        std::vector<std::vector<uint64_t>> decomposeHelper(const BoostGraph& G);

        //Breaking chains at the first node
        void breakingStartNode();

        //Breaking chains at every Live-out
        void breakEveryLiveOut();

        //Breaking chains at every live-in
        void breakEveryLiveIn();

        std::map<uint32_t, std::set<uint32_t> >
        reverseMapping(std::map<uint32_t, std::set<uint32_t> > uniquedependency);

        std::map<uint32_t, std::vector<uint32_t> >
        reverseMapping(std::map<uint32_t, std::vector<uint32_t> > dependency);

        //Checking whether decomposed graph has loop
        void checkLoop();

        //Mergeing chains
        bool mergeChain(std::vector<std::vector<uint64_t>>& m_chain, const BoostGraph& orig_graph);

        //After merging we need to double check number of live-out and live-in for each chain
        void checkLiveOutLimit();
        void checkLiveInLimit();
        
        //Calculating stats of the chains
        void buildChainGraph();

        //Printing graph stats into files
        void printChainStats(std::string str_name);

        //Grouping chains into levels
        void buildChainLevel();

        //Create chain blocks
        void createChainBlocks();
    public:

        std::map<uint64_t, uint64_t> vertex_to_Chunk;
        std::vector<chainDependency> dependencies;
        std::map<uint32_t, std::vector<uint32_t>> chain_to_dependencies;      /*< live_out edges from C to other nodes>*/
        std::map<uint32_t, std::vector<uint32_t>> reverse_mapping;            /*< live_in  edges from other nodes to C >*/
        std::map<uint32_t, std::set<uint32_t>>    un_dependencies;            /*< fan_out  edges from C to other nodes >*/
        std::map<uint32_t, std::set<uint32_t>>    un_reverse_mapping;         /*< fan_in   edges from other nodes to C>*/

        std::map<uint32_t, std::vector<uint32_t>> chain_map;

        uint32_t graph_limit;

        //Path variables
        const std::string graph_param_path;  
        const std::string llvm_instruction_path;

        //Constructor
        newGraph():first_node_id(0), first_ch(0),dirty_graph(0), clean_graph(0), 
            orig_graph(0), ilp_graph(0), num_arg(0), num_const(0), firstlevelLD(0), graph_limit(1000),
            graph_param_path("def/Graph.def"),llvm_instruction_path("def/Instruction.def"){}

        //Function
        void initialize(std::string full_path, std::string str_name);

        void readGraph(std::ifstream& input_file);
        void writeGraph(std::string str_name, BoostGraph& orig_graph);
        void writeGraph(std::string str_name, BoostGraphComp& orig_graph);
        void writeChain(std::string str_name);

        void createChains();
        
        void findGrahpLevel(BoostGraph& G, uint32_t id, std::vector<int32_t>& m_level, int32_t current_level);

        void buildILPGraph();
        void ilpStat(std::string str_name);
};

#endif
