#include "common.hpp"
#include "newGraph.hpp"
#include "Parameter.hpp"
#include "Core.hpp"

#define REQ_SIZE 64
#define CHUNK_LIMIT 99
//#define CHUNK_LIMIT 1


using namespace std;
using namespace boost;
using namespace boost::filesystem;
namespace po = boost::program_options;


int
main(int argc, char* argv[]){


    // Reading program's input
    string m_file_path;
    string m_memory_path_acc;
    string m_memory_path_ooo;
    uint32_t m_lane_number;
    bool m_print_log;
    path p;
    path p_acc;
    path p_ooo;

    //Processing program input
    try {

        po::options_description desc("Usage: chain [Program's options] <input_file>\n"
                "Chain's options:");
        desc.add_options()
            ("help", "Display this help and exit")
            ("input-file,i", po::value<string>(&m_file_path)->required(),
             "Input file [should be in dot format]")
            ("acc-file,a", po::value<string>(&m_memory_path_acc),
             "Accelerator memory file [should be in gz format]")
            ("ooo-file,m", po::value<string>(&m_memory_path_ooo),
             "OOO memory file [should be in gz format]")
            ("lane-number,n", po::value<uint32_t>(&m_lane_number)->default_value(4),
             "Number of lanes")
            ("log", po::value<bool>(&m_print_log)->default_value(false),
             "Printing log into logfile")
        ;

        po::variables_map vm;        
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            cout << desc << "\n";
            return 0;
        }

        if (vm.count("input-file")) {
            cout << "input file was set to " 
                 << vm["input-file"].as<string>() << ".\n";
            p = vm["input-file"].as<string>();
            if (p.extension() != ".dot") {
                cerr << "DOT FILE is needed" << endl;
            }
        }
        if (vm.count("acc-file")) {
            cout << "Accelerator memory file was set to " 
                 << vm["acc-file"].as<string>() << ".\n";
            p_acc = vm["acc-file"].as<string>();
            if (p_acc.extension() != ".gz") 
                cerr << "GZ FILE is needed" << endl;

        }
        if (vm.count("ooo-file")) {
            cout << "OOO memory file was set to " 
                 << vm["ooo-file"].as<string>() << ".\n";
            p_ooo= vm["ooo-file"].as<string>();
            if (p_ooo.extension() != ".gz") 
                cerr << "GZ FILE is needed" << endl;

        }
        if (vm.count("lane-number")){
            cout << "Number of lanes set to: "
                << vm["lane-number"].as<uint32_t>() << ".\n";
        }
        if (vm.count("log")){
            cout << "Printing log set to: "
                << vm["log"].as<bool>() << ".\n";
        }

        po::notify(vm);    
    }
    catch(std::exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
    }


    // You can choose whether print to stdout or to a log file
    if(m_print_log){

        //Redirecting standard output to file
        stringstream m_temp;
        m_temp << "output/" << p.stem().c_str() << "-log.log";
        std::ofstream out(m_temp.str());
        std::cout.rdbuf(out.rdbuf());
    }


    //////////////////////////////
    //Get iteration numbers
    //////////////////////////////

    std::cout << "Loading ACC file..... "<< std::endl;

    std::ifstream acc_file(p_acc.c_str(), std::ios_base::in | std::ios_base::binary);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> inbufacc;
    inbufacc.push(boost::iostreams::gzip_decompressor());
    inbufacc.push(acc_file);

    //Convert streambuf to istream
    std::istream instream(&inbufacc);
    std::string line;
    boost::char_separator<char> sep(",");
    std::set<uint32_t> iter_set;

    while(std::getline(instream, line)){
        boost::tokenizer<boost::char_separator<char>> tokens(line, sep);
        auto t_itr = tokens.begin();
        iter_set.insert(std::stoi(*t_itr));
    }
    /////////////////////////////////////////
    
    smatch match;
    
  
    ////Initialize RUBY
    auto m_ruby = new O3sim_ruby(1, 1,4,1, true, true, 1 ,"","med","/dev/null");
    m_ruby->initialize();

    /*****************************
     * Start reading from ooo-mem file
     *****************************/
    std::ifstream input_file(p_ooo.c_str(), std::ios_base::in | std::ios_base::binary);

    boost::iostreams::filtering_streambuf<boost::iostreams::input> inbufooo;
    inbufooo.push(boost::iostreams::gzip_decompressor());
    inbufooo.push(input_file);

    //Convert streambuf to istream
    std::istream ooostream(&inbufooo);

    //Initializing the ACC core
    Core testCore(m_lane_number,p_acc.c_str(), m_ruby);
    testCore.m_graph.initialize(p.c_str(), p.stem().c_str());
    testCore.memoryInitialize();

    //// Ruby initial values
    uint32_t core_id    = 0;
    uint32_t thread_id  = 0;

    std::unordered_map<uint64_t, uint32_t> memory_map;
    std::vector<uint64_t> memory_vector;
    uint64_t r_addr;

    cout << "***************************************" << endl;
    cout << "**** SIMULATION STARTED ***************" << endl;
    cout << "***************************************" << endl;

    ////Global cycle
    uint32_t global_cnt = 0;
    uint32_t cycle_cnt = 0;
    uint32_t ruby_cycle = 0;
    uint32_t num_chunk_call = 0;
    bool flag_empty = true;
    while(std::getline(ooostream, line)){
        std::vector<std::string> match;
        boost::tokenizer<boost::char_separator<char>> tokens(line, sep);
        for(const auto t : tokens)
            match.push_back(t);
        
        assert(((match.size() == 4) || (match.size() == 3) || (match.size() == 2))
                && "\nMem input file is in wrong format");

        //if(regex_search(line, match, chunk_parse)){
        if(match.size() == 2){
            //Free all the mem requests
            if(!flag_empty){
                
                cout << "***************************************" << endl;
                cout << "**** CHUNK BEGIN **********************" << endl;
                cout << "***************************************" << endl;

                while(!memory_vector.empty()){

                    m_ruby->advance_time();
                    global_cnt++;
                    ruby_cycle++;

                    while(!m_ruby->RubyQueueEmpty(core_id))
                    {
                        uint64_t addr = m_ruby->RubyQueueTop(core_id);
                        auto mem_it = find(memory_vector.begin(), memory_vector.end(), addr);
                        if(mem_it != memory_vector.end()){
                            memory_vector.erase(mem_it);
                            memory_vector.shrink_to_fit();
                        }
                        else
                            assert(0 && "ERROR");

                        m_ruby->RubyQueuePop(core_id);
                    }
                }

                flag_empty = true;
            }


            //////////////////////////////////////////////////
            ////Start acc
            ////if it's chunk begin read one line from ACC file
            //////////////////////////////////////////////////
            
            //regex_search(line, match, ooo_parse);
            
            testCore.buildChains(testCore.m_graph.Chains,
                        testCore.m_graph.vertex_to_Chunk, testCore.m_graph.dependencies, testCore.m_graph.un_dependencies,
                        testCore.m_graph.un_reverse_mapping, testCore.m_graph.chain_to_dependencies, testCore.m_graph.orig_map, testCore.m_graph.comp_graph);

            testCore.coreSetIter(stol(match[0]));

            testCore.initialize();

            while(!testCore.run_a_cycle()){
                //increamenting acc and global counter
                global_cnt++;
                cycle_cnt++;
            }
            if(num_chunk_call++ > CHUNK_LIMIT){
                cout << "Finished [ " << num_chunk_call-1 << " ]" << endl;
                break;
            }
        }

        else{
            //1) Extract request's details
            string opcode;
            bool isWrite = false;
            flag_empty = false;

            //if(regex_search(line, match, mem_parse)){
            if(match.size() == 4){
                r_addr = stoll(match[3]);
                opcode = match[1];
            }
            //else if(regex_search(line, match, ooo_parse)){
            if(match.size() == 3){
                r_addr = stoll(match[0]);
                opcode = match[1];
            }

            // Send request to ruby
            if(opcode == "Store")
                isWrite = true;
            else
                isWrite = false;


            //2)
            m_ruby->advance_time();
            global_cnt++;
            ruby_cycle++;

            Addr req_addr = Memory::base_addr(r_addr);
            //3) Check whether request exist
            auto mem_it = find(memory_vector.begin(), memory_vector.end(), req_addr);
            if(mem_it == memory_vector.end()){
                //4) Calling Ruby
                //
                while(!m_ruby->isReady(req_addr,REQ_SIZE,core_id, thread_id, false, false, NULL)){
                    m_ruby->advance_time();
                    ruby_cycle++;
                    global_cnt++;
                }

                if(m_ruby->isReady(req_addr,REQ_SIZE,core_id, thread_id, false, false, NULL)){
                    //cout << "Request: " << req_addr << endl;
                    memory_vector.push_back(req_addr);
                    m_ruby->send_request(req_addr, REQ_SIZE , core_id, thread_id, isWrite, false, NULL); 
                }
            }

            //5) Process MSHR
            while(!m_ruby->RubyQueueEmpty(core_id))
            {
              uint64_t addr = m_ruby->RubyQueueTop(core_id);
              //std::cerr<<"m_ruby: get response: addr " << addr <<endl;
              auto mem_it = find(memory_vector.begin(), memory_vector.end(), addr);
              if(mem_it != memory_vector.end()){
                  memory_vector.erase(mem_it);
                  memory_vector.shrink_to_fit();
              }
              else
                  assert(0 && "ERROR");

              m_ruby->RubyQueuePop(core_id);
            }
        }
    }


    cout << "*********************" << endl;
    cout << "TOTAL CYCLE:  " << global_cnt << endl;
    cout << "RUBY  CYCLE:  " << ruby_cycle << endl;
    cout << "ACC   CYCLE:  " << global_cnt - ruby_cycle << endl;
    testCore.finishCore();
    cout << "*********************" << endl;

    // Free RUBY object
    delete m_ruby;

    return 0;
}

