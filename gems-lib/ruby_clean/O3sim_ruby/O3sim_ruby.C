#include "O3sim_ruby.h"
#include "protocol_name.h"
#include "System.h"
#include "init.h"
#include "Tester.h"
#include "EventQueue.h"
#include "getopt.h"
#include "Network.h"
#include "CacheRecorder.h"
#include "Tracer.h"

#include "System.h"
#include "SubBlock.h"
#include "Chip.h"
#include "RubyConfig.h"
#include "Sequencer.h"

#include "O3simInterface.h"

using namespace std;

#include <string>
#include <map>

extern "C" {
#include "simics/api.h"
};

#include "confio.h"
#include "initvar.h"

// A generated file containing the default tester parameters in string form
// The defaults are stored in the variables
// global_default_param and global_default_tester_param global_default_gpusim_param
#include "default_param.h"
#include "tester_param.h"
#include "O3sim_param.h"


static void O3sim_ruby_allocate( void );
static void O3sim_ruby_generate_values( void ) { };

O3sim_ruby::O3sim_ruby( unsigned num_processors,
                        unsigned SMT,
                        unsigned num_L2_banks,
                        unsigned num_memories,
                        bool CMP,
                        bool garnet,
                        unsigned debug_start_time,
                        char *debug_filter_string,
                        char *debug_verbosity_string,
                        char *debug_output_filename)
{
    //   printf("Ruby: constructed O3sim_ruby\n");
   m_num_processors = num_processors;
   m_num_warps_per_processor = SMT;
   m_num_L2_banks = num_L2_banks;
   m_num_memories = num_memories;
   m_CMP = CMP;
   m_garnet = garnet;

   m_debug_start_time = debug_start_time;
   m_debug_filter_string = debug_filter_string;
   m_debug_verbosity_string = debug_verbosity_string;
   m_debug_output_filename = debug_output_filename;

}

O3sim_ruby::~O3sim_ruby() {
}

void O3sim_ruby::initialize() {
   // printf("Ruby: initializing O3sim_ruby\n");

   // Instantiating params in the order. Rubyconfig, Tester, and
   // O3sim. If there is any overlap the later config overrules earlier
   // config.
   int param_len = strlen( global_default_param ) +
                   strlen( global_default_tester_param ) +
                   strlen( global_default_O3sim_param) + 1;

   char *default_param = (char *) malloc( sizeof(char) * param_len );
   my_default_param = default_param;
   strcpy( default_param, global_default_param );
   strcat( default_param, global_default_tester_param );
   strcat( default_param, global_default_O3sim_param );

   /** note: default_param is included twice in the tester:
    *       -once in init.C
    *       -again in this file
    */
   initvar_t *ruby_initvar = new initvar_t( "ruby", "../../../ruby_clean/",
                                            default_param,
                                            &O3sim_ruby_allocate,
                                            &O3sim_ruby_generate_values,
                                            NULL,
                                            NULL );
   my_initvar = ruby_initvar;
   ruby_initvar->checkInitialization();


   // Set global options here
   g_NUM_PROCESSORS = m_num_processors;
   g_NUM_SMT_THREADS = m_num_warps_per_processor;
   if (m_CMP) {
       /* [UGLY] Instantiating a single chip CMP here. This is an ugly hack */
      g_PROCS_PER_CHIP  = m_num_processors;
      g_NUM_L2_BANKS    = m_num_L2_banks;
      g_NUM_MEMORIES    = m_num_memories;
   }
   g_GARNET_NETWORK = m_garnet;

   DEBUG_START_TIME = m_debug_start_time;
   DEBUG_FILTER_STRING = strdup(m_debug_filter_string);
   DEBUG_VERBOSITY_STRING = strdup(m_debug_verbosity_string);
   DEBUG_OUTPUT_FILENAME = strdup(m_debug_output_filename);

   ruby_initvar->allocate();

   g_system_ptr->printConfig(cout);
   cout << "Testing clear stats...";
   g_system_ptr->clearStats();
   cout << "Done." << endl;

   m_driver_ptr = dynamic_cast<O3simInterface*>(g_system_ptr->getDriver());
}

void O3sim_ruby::print_stats(ostream& out) {
   g_system_ptr->printStats(out);
}

void O3sim_ruby::destroy() {
   g_debug_ptr->closeDebugOutputFile();
   free(my_default_param);
   delete my_initvar;
   // Clean up
   destroy_simulator();
   cerr << "Success: " << CURRENT_PROTOCOL << endl;
}


//
// Helper functions
//
static void O3sim_ruby_allocate( void )
{
  init_simulator();
}

/* Hooks that interact with the actual O3sim driver that feeds the sequencer */
/* Hooks that interact with O3sim to feed sequencer->O3Driver-callbacks */
void O3sim_ruby::send_request(
                              unsigned long long addr,
                              unsigned req_size,
                              unsigned sid,
                              unsigned tid,
                              bool is_write,
                              bool isPriv,
                              class mem_fetch * mf
                             )
{
   O3simInterface::makeRequest(addr, req_size, sid, tid, is_write, isPriv, mf);
}


void O3sim_ruby::send_prefetch(
                               unsigned long long addr,
                               unsigned req_size,
                               unsigned sid,
                               unsigned tid,
                               bool is_write,
                               bool isPriv,
                               class mem_fetch * mf
                              )
{
    // Placeholder
    // Fix O3simInterface makePrefetch
}


/* Move ruby clock by 1. */
void O3sim_ruby::advance_time()
{
   O3simInterface::advanceTime();
}


/*  */
bool O3sim_ruby::RubyQueueEmpty(unsigned core_id)
{
   return m_driver_ptr->RubyQueueEmpty(core_id);
}

/*  */
unsigned long long O3sim_ruby::RubyQueueTop(unsigned core_id)
{
   return m_driver_ptr->RubyQueueTop(core_id);
}

/*  */
void O3sim_ruby::RubyQueuePop(unsigned core_id)
{
   m_driver_ptr->RubyQueuePop(core_id);
}

/*  */
bool O3sim_ruby::isReady(unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, bool is_write, bool isPriv, class mem_fetch * mf)
{
   return O3simInterface::isReady(addr, req_size, sid, tid, is_write, mf);
}

void O3sim_ruby::ForceUnlockBlock(unsigned long long addr, unsigned core_id)
{

}

bool O3sim_ruby::tryAccCacheAccess(const unsigned long long addr)
{
  return m_driver_ptr->tryAccCacheAccess(addr);
}

bool O3sim_ruby::tryL1CacheAccess(const unsigned long long addr)
{
  return m_driver_ptr->tryL1CacheAccess(addr);
}
