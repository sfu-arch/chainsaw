#ifndef __LANE__
#define __LANE__

#include "common.hpp"
#include "newGraph.hpp"
#include "Chain.hpp"
#include "Memory.hpp"


class Lane 
{
    public:
        //Lane's variable
        uint32_t laneID;
        uint32_t m_memory_mode;
        uint64_t lane_size;
        uint32_t fetch_cnt;
        uint32_t decode_cnt;
        int32_t  exe_cnt;
        uint32_t m_memory_stall;
        uint32_t m_stall;
        int32_t m_current_ch;
        std::string m_current_op;

        //Instruction list
        Instruction llvm_ins;

        //Lane's instruction queue
        std::queue<Node> instruction_queue;

        //Pipline stack stages
        std::queue<Node> fetch_stack;
        std::queue<Node> decode_stack;
        std::queue<Node> exe_stack;

        //Lane's parameter
        Parameter lane_param;

        //Lanes' global queues
        static std::queue<int32_t> finished_chains;
        static std::queue<std::tuple<uint32_t, uint32_t>> mem_req_queue;

        //Profile information
        std::vector<uint32_t> schChainsList;

        //Set parameter paths
        const std::string lane_param_path;
        const std::string llvm_instruction_path;

        //Constructor
        Lane(uint32_t ID, uint32_t m_mode, uint32_t lane_size): laneID{ID}, m_memory_mode{m_mode}, 
            lane_size{lane_size},fetch_cnt{0},decode_cnt{0},exe_cnt{0}, m_memory_stall{0}, m_stall{0},m_current_ch{-1},
            lane_param_path("def/Lane.def"), llvm_instruction_path("def/Instruction.def"){
                lane_param.readParam(lane_param_path);
                llvm_ins.readInst(llvm_instruction_path);
            }

        //Default constructor
        Lane(): Lane(0,0, LANE_DEFAULT_SIZE){}

        /**
         * Loads instructions into instruction buffer
         * and returns size of the instruction buffer
         */
        int LoadInstructionQueue(std::vector<Node> input_chain, uint32_t chain_id);

        /**
         * Fecthes instructions from instruction buffer and loads into fetch vector
         * return 0 if fecth vector is full otherwise it returns 1
         * @param window size of fetch window
         */
        //void LoadFetchBuffer( const int window);

        /**
         * Setting decode counter value
         */
        void setFCounter(const uint32_t decode_delay);

        /**
         * Setting decode counter value
         */
        void setDCounter(const uint32_t decode_delay);

        /**
         * Setting execute counter value
         */
        void setExeCounter(const int32_t execute_delay);


        /**
         * Fecthes instructions from instruction buffer and loads into fetch vector
         * return 0 if fecth vector is full otherwise it returns 1
         * @param window size of fetch window
         */
        void LoadExecutionBuffer();
        //void LoadExecutionBuffer( const int window);

        /**
         * Load from insturction memory and start Fetch
         */
        void fetch();

        /**
         * Load from insturction memory and start Decode
         */
        void decode();

        /**
         * Issue decoded command using execution unit
         */
        void execute();

        /**
         * Update lane's counters
         */
        void updateCounter();

        /**
         * Running one cycle simulation
         */
        void run();

        /**
         * Check whether exe queue is available
         */
        bool is_exe_full(){return !(exe_stack.empty());}

        /**
         * Returns lane's availabe free slots
         */
        int available_size() const {return (lane_size - instruction_queue.size());}

         /**
         * Returning number of free slots
         */
        uint32_t freeSlots() const { return (this->lane_size - this->instruction_queue.size()); }

        /**
         * Check wheter the lane is free
         * free here means utilization of the lane
         */
        bool is_free();

        /**
         * Updating lane's profile information for each cycle
         */
        void profile_update();

        /**
         * Print Lane's status
         */
        void printLaneStatus();

        /**
         * Overloading < operator
         * Compare lanes depending on their free slots 
         */
        bool operator < (const Lane &rhs) const { return (freeSlots() < rhs.freeSlots()); }
 
};
#endif
