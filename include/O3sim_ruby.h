#ifndef O3SIM_RUBY_H
#define O3SIM_RUBY_H

#include <stdio.h>
#include <fstream>

class O3simInterface;

class O3sim_ruby {
public:
    O3sim_ruby( unsigned num_processors,
                unsigned num_warps_per_processor,
                unsigned num_L2_banks,
                unsigned num_memories,
                bool CMP,
                bool garnet,
                unsigned debug_start_time,
                char *debug_filter_string,
                char *debug_verbosity_string,
                char *debug_output_filename);

    ~O3sim_ruby();

    /* Check if sequencer is ready. Not ready if RubyConfig:number of msgs already fed */
    bool isReady( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, bool is_write, bool isPriv, class mem_fetch * mf );

    /* Ruby hook to send addresses to sequencer */
    void send_request( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, bool is_write, bool isPriv, class mem_fetch * mf );

    /* Ruby hook to send prefetch addresses to sequencer */
    void send_prefetch( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, bool is_write, bool isPriv, class mem_fetch * mf );

    /* Cycle advancer */
    void advance_time();

    /* Check if back port from Ruby to O3 sim is empty */
    bool RubyQueueEmpty(unsigned core_id);


    /* Address. Block or line address that has returned */
    unsigned long long RubyQueueTop(unsigned core_id);


    void RubyQueuePop(unsigned core_id);

    // Hack to make sure stray blocks are unlocked
    void ForceUnlockBlock(unsigned long long addr, unsigned core_id);


    void print_stats(std::ostream& out);

    void initialize();

    void destroy();
private:
    char * my_default_param;
    class initvar_t * my_initvar;
    O3simInterface * m_driver_ptr;

    // Options passed in
    unsigned m_num_processors;
    unsigned m_num_warps_per_processor;
    unsigned m_num_L2_banks;
    unsigned m_num_memories;

    bool m_CMP;
    bool m_garnet;

    unsigned m_debug_start_time;
    char * m_debug_filter_string;
    char * m_debug_verbosity_string;
    char * m_debug_output_filename;

};

#endif
