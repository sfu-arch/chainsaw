#ifndef __CHAIN__
#define __CHAIN__

#include "common.hpp"
//#include "Graph.hpp"
#include "newGraph.hpp"

struct chainNode
{
    uint32_t c_id;
    uint32_t old_id;
    uint32_t new_id;
    uint32_t name;
    std::string type;
    uint32_t latency;
};
typedef struct chainNode Node;



class Chain
{
    public:
        uint32_t id;
        uint32_t num_dependencies;
        std::set<uint32_t> live_in;
        std::set<uint32_t> live_out;
        std::vector<Node> instruction_list;
        bool ch_mem;

        Chain(uint32_t n):id{0},num_dependencies{0}, ch_mem{false}{}

        /**
         * Check whther the chain is free or not
         * @param out True if it's free
         */
        bool is_free();

        /**
         * Reduce dependencies
         */
        void depen_increase();
        void depen_decrease();

        void push_ins(Node m_node);
        void push_liveIn(uint32_t m_temp);
        void push_liveOut(uint32_t m_temp);

        uint32_t return_id();

        std::vector<Node> return_instruction();

        /**
         * Overloading < operator
         * Compare chains depending on their instruction's size
         */
        bool operator < (const Chain& rhs) { return instruction_list.size() < rhs.instruction_list.size(); }
        
};
#endif
