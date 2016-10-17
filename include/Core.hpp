#ifndef __CORE__
#define __CORE__

#include "common.hpp"
#include "Chain.hpp"
//#include "Graph.hpp"
#include "newGraph.hpp"
#include "Lane.hpp"
#include "Memory.hpp"


class Core
{
    private:
        Instruction llvm_ins;

        uint32_t num_iter;

    public:

        //Variables
        //
        //number of lanes
        uint32_t num_lanes;

        //Input Graph
        //Graph m_graph;
        newGraph m_graph;

        //Core parameter
        Parameter core_parameter;

        //input chains
        std::vector<Chain>input_chain;

        //dependency table
        std::array<std::array<uint32_t, MAX_NUM_CHAIN>, MAX_NUM_CHAIN> depen_table;
        std::array<uint32_t, MAX_NUM_CHAIN> lane_pre_schedule_size;
        
        //lane preschedule
        std::vector<std::vector<uint32_t>> lane_pre_schedule;
        
        //scheduling stack
        std::vector<uint32_t> stack_sche_chain;

        //Set parameter
        const std::string core_param_path;

        //Memory map file
        //std::ifstream ldstfile;
        std::string mem_map_path;

        //Memory system
        Memory m_memory;

        //Lanes
        std::vector<Lane> m_lanes;

        //Keeping mapping from chains to lanes C -> L
        std::map<uint32_t, uint32_t> chaintoLane;

        //Number of chains
        uint32_t num_chains;

        //Processor cycles
        uint32_t cycle_cnt = 0;

        //Lane memory ops map
        std::map<uint32_t, uint32_t> mem_req_map;
        
        //Lane status
        std::ofstream lane_status_file;

        uint32_t external_com;
        uint32_t internal_com;

        uint32_t core_free;
        uint32_t core_idle;

        //Class constructor
        //TODO Add graph limit
        Core(uint32_t n, std::string m_mem_path, O3sim_ruby* _ruby):num_iter(0),num_lanes(n),m_graph(), lane_pre_schedule(n),
            core_param_path("def/Core.def"),mem_map_path(m_mem_path),m_memory(_ruby), num_chains(MAX_NUM_CHAIN),
            cycle_cnt(0), external_com(0), internal_com(0), core_free(0), core_idle(0)
        {

            core_parameter.readParam(core_param_path);
            llvm_ins.readInst("def/Instruction.def");


            //initlizing dependancy table;
            for(auto& row : depen_table){
                for(auto& col : row){
                    col = 0;
                }
            }

            //initilize buffer size
            for(auto&l : lane_pre_schedule_size)
                l = 0;

            //TODO Add lane size
            //uint32_t lane_size = resource/n;
            //Initilizing Lanes
            for(uint32_t i = 0; i < n; ++i){
                Lane n_lane(i, core_parameter.param_set.find("MEMORY_MODE")->second, LANE_DEFAULT_SIZE);
                this->m_lanes.push_back(n_lane);
            }

        }

        //Mapping Chain to its dependency C -> [dependencies]
        std::map<uint32_t, std::set<uint32_t>> un_dependencies;

        /**
        * Getting a Dilworth's algorithm output and building
        * Chains with their dependencies and fill "input_chain"
        * @param chains Decomposed graph
        */
        void buildChains(
                std::vector<std::vector<uint64_t>> decom_chains,
                std::map<uint64_t, uint64_t> chain_mapping,
                std::vector<chainDependency> chain_deppendenceis,
                std::map<uint32_t, std::set<uint32_t>> un_dependencies,
                std::map<uint32_t, std::set<uint32_t>> reverse_dependencies,
                std::map<uint32_t, std::vector<uint32_t>> chain_live_out,
                std::map<uint32_t, uint32_t> node_map,
                BoostGraphComp& G);

        /**
         * Initilizing dependancy table
         */
        void initialize_depen_table();

        /**
         * Fill pre_schedule queue
         */
        void preschedule();

        /**
         * Pre scheduling based on block IDs
         */
        void block_preschedule();

        /**
         * Check whether the scheduler has ran all the chains or not?
         */
        bool is_finish();

        /*
         * Start the core
         */
        void run_all();

        /**
         * run one cycle of the core
         */
        bool run_a_cycle();

        /**
         * Run each lane and Memory system one cycle
         */
        void run_one_cycle_core();

        /**
         * Sending lanes' memory requests into memory system
         */
        void send_memory_req();

        /**
         * Update lane's exe counter
         */
        void update_mem_exe();

        /**
         * Loading scheduled chains into lanes
         * Scheduling is out of order
         */
        void ooo_scheduler();

        /**
         * Updating chains' dependencies
         */
        void updateChains();

        /**
         * READ MEMORY MAP FILE
         */
        void read_memcsv(std::ifstream& file_name);

        /**
         * Printing memory map
         */
        void print_memmap();

        /**
         * Set core iteration
         */
        void coreSetIter(uint32_t n);

        /**
         * Initializing memory
         */
        void memoryInitialize();

        /**
         * Initialize the core
         */
        void initialize();

        void finishCore();
};

#endif
