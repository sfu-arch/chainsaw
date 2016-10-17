#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/max_cardinality_matching.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/bipartite.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/property_map/property_map.hpp>

#include "newGraph.hpp"

using namespace std;
using namespace boost;


static std::vector<std::string> brewer_color {"#a6cee3","#1f78b4","#b2df8a","#33a02c","#fb9a99","#e31a1c","#fdbf6f","#ff7f00","#cab2d6","#6a3d9a","#ffff99","#b15928"};

/**
 * Retrun mapping for each vertex to its
 * corresponding chunk
 */
std::map<uint64_t, uint64_t> 
vertexToChainMap(const std::vector<std::vector<uint64_t>> chains)
{
    //First we create a map from vertex to the chunk ID
    std::map<uint64_t, uint64_t> vertexToChunk;

    for(uint32_t i = 0; i < chains.size(); ++i){
        for(auto& c : chains[i]){
            vertexToChunk.emplace(c,i);
        }
    }

    return vertexToChunk;
}


/**
 * Returning list of unique dependencies for each chains
 */
std::map<uint32_t, std::set<uint32_t>>
uniqueDependency(std::map<uint32_t, std::vector<uint32_t> > histodependency)
{
    std::map<uint32_t, std::set<uint32_t>> un_depen;
    for(auto& c : histodependency){
        for(auto& d: c.second)
            un_depen[c.first].emplace(d);
    }

    return un_depen;

}
/**
 * Getting fan-out (unique dependencies) and returning fan-in
 */
std::map<uint32_t, std::set<uint32_t> >
newGraph::reverseMapping(std::map<uint32_t, std::set<uint32_t>> uniquedependency)
{
    std::map<uint32_t, std::set<uint32_t>> reverse_depen;
    for(auto& c : uniquedependency){
        for(auto& r : c.second){
            reverse_depen[r].emplace(c.first);
        }
    }
    return reverse_depen;

}

/**
 * Overloading reverseMapping function
 */
std::map<uint32_t, std::vector<uint32_t> >
newGraph::reverseMapping(std::map<uint32_t, std::vector<uint32_t> > dependency)
{
    std::map<uint32_t, std::vector<uint32_t>> reverse_depen;
    for(auto& c : dependency){
        for(auto& r : c.second){
            reverse_depen[r].push_back(c.first);
        }
    }
    return reverse_depen;

}


/**
 * Getting decomposed graph as an input and return depenencies
 */
std::vector<chainDependency>
dependenciesGenerate(const BoostGraph& BG, std::vector<std::vector<uint64_t>>& chains, const std::map<uint64_t, uint64_t> chainsMap)
{
    std::vector<chainDependency> dependencies;

    for(uint32_t j = 0; j < chains.size(); j++){
        auto& c = chains[j];

        //For each vertex we look at its out edges, for each target vertex
        //if it's in different chain then we count it as an dependency
        for(uint32_t i = 0; i < c.size(); ++i){
            auto EI = out_edges(c[i], BG);
            for(auto EB = EI.first, EE = EI.second; EB != EE; EB++){
                auto T = target(*EB, BG);
                if(find(c.begin(), c.end(), T) == c.end()){
                    chainDependency temp;
                    temp.source = chainsMap.find(c[i])->second;
                    temp.target = chainsMap.find(T)->second;

                    //Check whether the decomposed graph have self loop or not
                    assert(temp.source != temp.target && "Decomposed graph can not have self loop!");
                    dependencies.push_back(temp);
                }
            }
        }

    }
    return dependencies;
}

/**
 * Returning list of dependencies for each chains
 */
std::map<uint32_t, std::vector<uint32_t>>
histoDependency(vector<chainDependency> dependencies)
{
    std::map<uint32_t, std::vector<uint32_t>> map_dependency;
    for (auto& dep : dependencies){
        map_dependency[dep.source].push_back(dep.target);
    }
    return map_dependency;

}

/**
 * Check whether the decomposed graph has loop or not
 */
bool
checkLoopComplete(uint32_t id, map<uint32_t, set<uint32_t>>& dependencies, list<uint32_t>& stack)
{


    auto node_it = dependencies.find(id);

    if(node_it == dependencies.end()){
        stack.pop_back();
        return false;
    }


    for(auto& ch : node_it->second){
        auto st_it = find(stack.begin(), stack.end(), ch);
        if(st_it != stack.end())
            return true;
        else{
            stack.push_back(ch);
            if(checkLoopComplete(ch, dependencies, stack))
                return true;
        }
    }

    stack.pop_back();
    return false;
}

/**
 * Checking do the chains have specific chain with more than 2 live-outs?
 */
bool
newGraph::checkLiveOutHelper(std::map<uint32_t, std::vector<uint32_t>>& m_live_out)
{
    for(auto& ch : m_live_out){
        if(ch.second.size() > graph_parameter.param_set.find("MAX_LIVE_OUT")->second 
                && ch.first != 0)
            return true;
    }
    return false;
}

/**
 * Checking do the chains have specific chain with more than 2 live-outs?
 */
bool
newGraph::checkLiveInHelper(std::map<uint32_t, std::vector<uint32_t>>& m_live_in)
{
    for(auto& ch : m_live_in){
        if(ch.second.size() > graph_parameter.param_set.find("MAX_LIVE_IN")->second)
            return true;
    }
    return false;
}



/**
 * Computes chains' level
 */
void
findChainsLevel(int32_t m_level, uint32_t id, map<uint32_t, set<uint32_t>>& dependencies,
                vector<chainGraph>& ch_graph)
{
    auto m_it = std::find_if(ch_graph.begin(), ch_graph.end(), 
            [id](const chainGraph &m) -> bool {return m.chainId == id;});

    //if(m_it == ch_graph.end())
        //return;
    //if((id == 166) || (id == 42) || (id == 165) || (id == 85)){
        //cout << id << ": " << endl;
        //for(auto& dep : dependencies.find(id)->second)
            //cout << dep << endl;
    //}
    //if(m_it->level == -1){
    if(m_it->level < m_level){
        m_it->level = m_level++;
        auto node_it = dependencies.find(id);

        if(node_it == dependencies.end())
            return;

        for(auto& dep : node_it->second){
            findChainsLevel(m_level, dep, dependencies, ch_graph);
        }
    }
    else
        return;
}

/**
 * Breaking chains at every live-outs helper
 */
bool
breakOutHelper(std::vector<std::vector<uint64_t>>& m_chain,
        const std::vector<chainDependency>& m_depen, const BoostGraph& orig_graph)
{

    for(auto& dep : m_depen){
        auto& a_ch = m_chain[dep.source];
        auto& b_ch = m_chain[dep.target];
        for(auto& node : a_ch){
            if(node == a_ch.back())
                continue;
            auto edges = boost::out_edges(node, orig_graph);
            OutEdge ei, ei_end;
            for(boost::tie(ei, ei_end) = edges; ei != ei_end; ++ei){
                auto target = boost::target(*ei, orig_graph);
                auto b_temp_it = std::find(b_ch.begin(), b_ch.end(), target) ;
                if(b_temp_it != b_ch.end()){
                    auto a_temp_it = std::find(a_ch.begin(), a_ch.end(), node);
                    std::vector<uint64_t> newVector(std::make_move_iterator(a_temp_it+1), std::make_move_iterator(a_ch.end()));
                    a_ch.erase(a_temp_it+1, a_ch.end());
                    a_ch.shrink_to_fit();
                    m_chain.push_back(newVector);
                    return true;
                }
            }
        }
    }
    return false;
}


/**
 * Explointing maximum available ILP
 * between chains (Breaking at live-in)
 */
bool 
breakInHelper(std::vector<std::vector<uint64_t>>& m_chain, const std::vector<chainDependency>& m_depen, BoostGraph& orig_graph)
{
    for(auto& dep : m_depen){
        auto& a_ch = m_chain[dep.source];
        auto& b_ch = m_chain[dep.target];
        for(auto& node : a_ch){
            auto edges = boost::out_edges(node, orig_graph);
            OutEdge ei, ei_end;
            for(boost::tie(ei, ei_end) = edges; ei != ei_end; ++ei){
                auto target = boost::target(*ei, orig_graph);
                auto temp_it = std::find(b_ch.begin(), b_ch.end(), target) ;
                if(*temp_it == b_ch.front())
                    continue;
                if(temp_it != b_ch.end()){
                    std::vector<uint64_t> newVector(std::make_move_iterator(temp_it), std::make_move_iterator(b_ch.end()));
                    b_ch.erase(temp_it, b_ch.end());
                    b_ch.shrink_to_fit();
                    m_chain.push_back(newVector);
                    return true;
                }
            }
        }
    }
    return false;
}





/**
 * Initilize the graph object
 */
void
newGraph::initialize(std::string full_path, std::string str_name)
{
    ifstream input_file(full_path, ios::in);
    assert(input_file.is_open() && "Error! File couldn't open");

    //1) Start reading graph parameters
    graph_parameter.readParam(graph_param_path);

    //Reading llvm instructions
    llvm_ins.readInst(llvm_instruction_path);

    //2) Reading input graph
    readGraph(input_file);

    //3) Clean read graph
    cleanReadGraphHelper();

    //6) Removing Arg and Const nodes
    cleanRemoveReadGraphHelper();
    
    //7) Extracting INS graph
    //insReadGraphHelper();
    completeReadGraphHelper(str_name);
    writeGraph("test", orig_graph);


    //4) Create chains
    createChains();

    //5) Breaking chains at first node
    breakingStartNode();


    //6) Braking at every live-out
    //First we check whether the parameter has been set
    auto search_param = graph_parameter.param_set.find("BREAK_LIVE_OUT");
    assert((search_param != graph_parameter.param_set.end()) &&
            "ERROR: BREAK_LIVE_OUT parameter has not been defined");
    if(search_param->second)
        breakEveryLiveOut();

    //11) Braking at every live-in
    //First we check whether the parameter has been set
    search_param = graph_parameter.param_set.find("BREAK_LIVE_IN");
    assert((search_param != graph_parameter.param_set.end()) &&
            "ERROR: BREAK_LIVE_IN parameter has not been defined");
    if(search_param->second)
        breakEveryLiveIn();

    //12) Merging the chains in order to increase the length
    search_param = graph_parameter.param_set.find("MERGE");
    assert((search_param != graph_parameter.param_set.end()) &&
        "ERROR: MERGE parameter has not been defined");
    if(search_param->second)
        while(mergeChain(Chains, clean_graph));

    //13) Check whether number of live-out for each chain is limit to LIMIT variable
    //checkLiveOutLimit();

    //14) Check whether number of live-in for each chain is limit to LIMIT variable
    //checkLiveInLimit();



    //5) Building final dependencies
    //
    this->vertex_to_Chunk = vertexToChainMap(Chains);
    this->dependencies = dependenciesGenerate(orig_graph, Chains, vertex_to_Chunk);
    this->chain_to_dependencies = histoDependency(dependencies);
    this->un_dependencies= uniqueDependency(chain_to_dependencies); 
    this->un_reverse_mapping = reverseMapping(un_dependencies);
    this->reverse_mapping = reverseMapping(chain_to_dependencies);




    //Printing final chains
    std::cout << "Chain size: " << Chains.size() << std::endl;
    printChain();
    printDependencies();

    list<uint32_t> loop_stack;
    loop_stack.push_back(0);
    bool has_loop = checkLoopComplete(0, un_dependencies, loop_stack);
    assert(!has_loop && "CHAIN HAS LOOP!");
    buildChainGraph();

    //6) filling lelvel mappint
    buildChainLevel();

    //7) Create Blocks
    createChainBlocks();

    //8) create ilp graph
    buildILPGraph();
    ilpStat(str_name);

    printGraphstats(str_name);
    printChainStats(str_name);

    //TODO: FIX THIS PART
    writeGraph(str_name+"-clean", clean_graph);
    writeGraph(str_name+"-ilpgraph", ilp_graph);
    //writeGraph(str_name+"-complete", comp_graph);
    writeChain(str_name+"-chain");



}


/**
 * Loading dot file and load into BG
 */
void
newGraph::readGraph(ifstream& input_file)
{
    dynamic_properties dp;

    //Vertex
    auto name = get(&vertex_prop::name, dirty_graph);
    dp.property("node_id", name);

    auto label = get(&vertex_prop::label, dirty_graph);
    dp.property("label", label);

    auto ch = get(&vertex_prop::ch, dirty_graph);
    dp.property("ch", ch);

    auto opcode = get(&vertex_prop::opcode, dirty_graph);
    dp.property("opcode", opcode);

    auto color = get(&vertex_prop::color, dirty_graph);
    dp.property("color", color);


    //Edge
    auto ecolor = get(&edge_prop::color, dirty_graph);
    dp.property("color", ecolor);

    auto style = get(&edge_prop::style, dirty_graph);
    dp.property("style", style);

    try
    {
        read_graphviz(input_file, dirty_graph, dp, "node_id");
    }
    catch(std::exception &err)
    {
        cerr << err.what() << endl;
        //cerr << "read_graphviz failed for " << input_file << "\n";
        exit(0);
    }

    return;

}

/**
 * Reordering the input graph
 */
void
newGraph::cleanReadGraphHelper()
{
    std::map<uint32_t, uint32_t> map_v;
    std::vector<std::pair<uint32_t,uint32_t>> m_added;
    //uint32_t j = 0;

    auto vertices = boost::vertices(dirty_graph);
    auto name = get(&vertex_prop::name, dirty_graph);

    //Adding vertices to the clean graph
    //for(uint32_t i = 0; i < boost::num_vertices(dirty_graph); i++){
    for(auto v = vertices.first; v != vertices.second; ++v){
        auto new_v = boost::add_vertex(clean_graph);
        map_v.emplace(get(name,*v),new_v);
        clean_graph[new_v].name = get(get(&vertex_prop::name, dirty_graph),*v);
        clean_graph[new_v].label = get(get(&vertex_prop::label, dirty_graph),*v);
        clean_graph[new_v].ch = get(get(&vertex_prop::ch, dirty_graph), *v);
        //Making all nodes color black
        clean_graph[new_v].opcode = get(get(&vertex_prop::opcode, dirty_graph),*v);
        clean_graph[new_v].color = "black";
    }
    //}

    //Adding edges to the clean graph
    typename boost::graph_traits<BoostGraph>::out_edge_iterator ei, ei_end;
    for( auto v = vertices.first; v != vertices.second; ++v){
        for(boost::tie(ei, ei_end) = boost::out_edges(*v, dirty_graph); ei != ei_end; ei++){
            auto source = boost::source(*ei, dirty_graph);
            auto target = boost::target(*ei, dirty_graph);


            boost::add_edge(map_v.find(get(name, source))->second, map_v.find(get(name, target))->second, clean_graph);

            //std::pair<uint32_t,uint32_t> temp_pair(map_v.find(get(name, source))->second,
                    //map_v.find(get(name, target))->second);


            ////Making sure that we only add one edge from source to target
            ////This case usually happens for BB node while we remove ARG and CONST nodes
            //auto it = std::find_if(m_added.begin(), m_added.end(), 
                    //[temp_pair](const std::pair<uint32_t, uint32_t>& t) -> bool {
                        //return ((temp_pair.first == t.first) && (temp_pair.second == t.second));
                    //}
                    //);
            //if(it == m_added.end()){
                //m_added.push_back(temp_pair);
            //}
        }
    }
}

/**
 * Removing edges from ARG and CONST nodes
 */
void
newGraph::cleanRemoveReadGraphHelper()
{
    std::map<uint32_t, uint32_t> v_map;

    //Using regex for extracting type of the nodes
    //regex reg_type("^(.*?)\\(");

    //uint32_t j = 0;
    //Adding vertices to the clean graph
    for(uint32_t i = 0; i < boost::num_vertices(clean_graph); i++){
        auto new_v = boost::add_vertex(orig_graph);
        orig_graph[new_v].name   = clean_graph[i].name;
        orig_graph[new_v].label  = clean_graph[i].label;
        orig_graph[new_v].ch     = clean_graph[i].ch;
        orig_graph[new_v].opcode = clean_graph[i].opcode;
        orig_graph[new_v].color  = clean_graph[i].color;

        v_map.emplace(new_v,i);


    }

    auto m_vertices = boost::vertices(clean_graph);
    //Adding edges to the clean graph
    typename boost::graph_traits<BoostGraph>::out_edge_iterator ei, ei_end;
    for( auto v = m_vertices.first; v != m_vertices.second; ++v){
        for(boost::tie(ei, ei_end) = boost::out_edges(*v, clean_graph); ei != ei_end; ei++){
            auto source = boost::source(*ei, clean_graph);
            auto target = boost::target(*ei, clean_graph);

            //boost::add_edge(source,target,orig_graph);
            //if(source != target) 
                boost::add_edge(v_map.find(source)->second, v_map.find(target)->second, orig_graph );
        }
    }

}

/**
 * Making a new graph which has more information
 */
void
newGraph::completeReadGraphHelper(std::string str_name)
{
    auto vertices = boost::vertices(orig_graph);

    map<uint32_t, uint32_t> new_v;
    vector<int32_t> m_level(boost::num_vertices(orig_graph));
    num_ops = boost::num_vertices(orig_graph);
    for(auto& l : m_level)
        l = -1;

    findGrahpLevel(orig_graph, 0, m_level, 0);

    uint32_t j = 0;
    //Adding vertices to the clean graph
    for(uint32_t i = 0; i < boost::num_vertices(orig_graph); i++){
        //Getting type
        regex reg_type("^(.*?)\\(");
        regex reg_id("^.*?\\((\\d+)\\)");
        smatch match;
        smatch id_match;
        regex_search(orig_graph[i].label, match, reg_type);
        regex_search(orig_graph[i].label, id_match, reg_id);

        stringstream s_temp;
        s_temp << match[1] << "(" << id_match[1] << ")";

        new_v.emplace(i,j++);
        auto n_v = boost::add_vertex(comp_graph);
        comp_graph[n_v].name = orig_graph[i].name;
        comp_graph[n_v].id = orig_graph[i].name;
        comp_graph[n_v].ch = stoi(orig_graph[i].ch);
        comp_graph[n_v].label = s_temp.str();
        comp_graph[n_v].opcode = match[1];
        comp_graph[n_v].latency = llvm_ins.instruction_llvm.find(match[1])->second;
        comp_graph[n_v].level = m_level[i];
        comp_graph[n_v].color = orig_graph[i].color;

        //keep original mapping
        orig_map[n_v] = stoi(id_match[1]);
    }

    //Adding edges to the clean graph
    typename boost::graph_traits<BoostGraph>::out_edge_iterator ei, ei_end;
    for( auto v = vertices.first; v != vertices.second; ++v){
        for(boost::tie(ei, ei_end) = boost::out_edges(*v, orig_graph); ei != ei_end; ei++){
            auto source = boost::source(*ei, orig_graph);
            auto target = boost::target(*ei, orig_graph);

            cout << "ADD edge: " << new_v.find(source)->second << " -> " <<  new_v.find(target)->second << endl;
            boost::add_edge(new_v.find(source)->second, new_v.find(target)->second, comp_graph);
        }
    }

    //Computing graph stats
    //1) Max length
    auto m_l = std::max_element(m_level.begin(), m_level.end());
    length = *m_l;
    //2) ILP
    //
   
    //uint32_t m_i=0;
    uint32_t sum_gilp=0;
    uint32_t mul_glip=1;

    std::map<uint32_t,uint32_t> graph_ilp;
    for(auto &ml : m_level){
        auto ml_it = graph_ilp.find(ml);
        if(ml_it == graph_ilp.end())
            graph_ilp[ml]=1;
        else
            ml_it->second++;
    }

    for(auto it_ml = graph_ilp.begin(); it_ml != graph_ilp.end(); ++it_ml){
        if(it_ml->second > max_ilp){
            max_ilp = it_ml->second;
        }
    }


    for(auto& m : graph_ilp){
        sum_gilp += m.second;
        mul_glip *= m.second;
    }
    
    geo_avg_ilp = std::pow((float)mul_glip,(float)((float)1/(float)(graph_ilp.size()-1)));
    avg_ilp = (double)sum_gilp/(double)graph_ilp.size();
    //max_ilp = m_i;
    

    //3) num ops
    num_ops = m_level.size();

    //4) Histogram and first level LD and MLP
    std::map<uint32_t, uint64_t> m_graph_mlp;
    for(uint32_t i = 0; i < m_level.size(); i++){

        auto m_node = comp_graph[i];

        //Instruction histogram
        auto ins_it = ins_histo.find(m_node.opcode); 
        if(ins_it == ins_histo.end())
            ins_histo[m_node.opcode] = 1;
        else
            ins_histo[m_node.opcode]++;

        //Check for first level LD
        if((m_node.level == 1) && (m_node.opcode== "Load"))
            firstlevelLD++;

        //Computing MLP
        if((m_node.opcode== "Load") || (m_node.opcode== "Store")){
            auto it_mlp = m_graph_mlp.find(m_node.level);
            if(it_mlp == m_graph_mlp.end())
                m_graph_mlp[m_node.level] = 1;
            else
                m_graph_mlp[m_node.level]++;
        }
        //Getting max of mlp
        max_mlp = 0;
        for(auto& node : m_graph_mlp){
            if(node.second > max_mlp)
                max_mlp = node.second;
        }
    }
}



/**
 * Reordering the input graph
 */
//void
//newGraph::cleanReadGraphHelper()
//{
    //std::map<uint32_t, uint32_t> map_v;
    ////std::vector<std::pair<uint32_t,uint32_t>> m_added;
    //uint32_t j = 0;

    //auto vertices = boost::vertices(dirty_graph);
    //auto name = get(&vertex_prop::name, dirty_graph);

    ////Adding vertices to the clean graph
    //for(uint32_t i = 0; i < boost::num_vertices(dirty_graph); i++){
        //for(auto v = vertices.first; v != vertices.second; ++v){
            //if(get(name, *v) == i){

                //regex reg_type("^(.*?)\\(");
                //regex reg_id("^.*?\\((\\d+)\\)");
                //smatch match;
                //smatch id_match;
                //regex_search(get(get(&vertex_prop::label, dirty_graph),*v), match, reg_type);
                //regex_search(get(get(&vertex_prop::label, dirty_graph),*v), id_match, reg_id);

                //stringstream s_temp;
                //s_temp << match[1] << "(" << id_match[1] << ")";

                //map_v.emplace(i,j++);
                //auto new_v = boost::add_vertex(clean_graph);
                //clean_graph[new_v].name = get(get(&vertex_prop::name, dirty_graph),*v);
                //clean_graph[new_v].label = get(get(&vertex_prop::label, dirty_graph),*v);
                //clean_graph[new_v].ch = get(get(&vertex_prop::ch, dirty_graph),*v);
                ////Making all nodes color black
                //clean_graph[new_v].color = "black";
                //clean_graph[new_v].opcode = get(get(&vertex_prop::opcode, dirty_graph),*v);

                ////keep original mapping
                //orig_map[new_v] = stoi(id_match[1]);
            //}
        //}
    //}


    //typename boost::graph_traits<BoostGraph>::out_edge_iterator ei, ei_end;
    //for( auto v = vertices.first; v != vertices.second; ++v){
        //for(boost::tie(ei, ei_end) = boost::out_edges(*v, dirty_graph); ei != ei_end; ei++){
            //auto source = boost::source(*ei, dirty_graph);
            //auto target = boost::target(*ei, dirty_graph);
            //boost::add_edge(map_v.find(source)->second, map_v.find(target)->second, clean_graph);
        //}
    //}

    //num_ops = boost::num_vertices(clean_graph);
//}

void
newGraph::createChains()
{
    auto vertices = boost::vertices(orig_graph);
    auto name = get(&vertex_prop::name, orig_graph);
    auto opcode = get(&vertex_prop::opcode, orig_graph);
    auto ch = get(&vertex_prop::ch, orig_graph);

    std::map<uint32_t,std::vector<uint32_t>> temp_map;
    //Get the mapping for the chains
    for(auto v = vertices.first; v != vertices.second; ++v){
        if(get(opcode,*v) == "BB"){
            first_ch = std::stoi(get(ch, *v));
            first_node_id = get(name, *v);
        }

        temp_map[std::stoi(get(ch,*v))].push_back(*v);
    }

    //Remap chain numbers
    uint32_t i = 0;
    chain_map[i++] = temp_map[first_ch];
    for(auto tm : temp_map){
        if(tm.first != first_ch)
            chain_map[i++] = tm.second;
    }

    //Get the vector of the mappings
    for(auto& ch : chain_map){
        std::vector<uint64_t> temp_ch;
        for(auto& n: ch.second){
            temp_ch.push_back(n);
        }
        Chains.push_back(temp_ch);
    }
}


/**
 * Printing chains
 */
void
newGraph::printChain()
{
    uint32_t i = 0;
    std::cout << "Chains: \n";
    for(auto& ch : Chains){
        cout << "C[" << i++ << "]: \n";
        for(auto&c : ch){
            cout << "\t" << c << endl;
        }
    
    }

}

/**
 * Calculating stats of the chains
 */
void
newGraph::buildChainGraph()
{
    uint32_t i = 0;
    for(auto& ch: Chains){
        chainGraph temp_node;
        //chain id
        temp_node.chainId = i;
        
        //chain size
        temp_node.size = ch.size();

        //chain fan_in
        auto it_live_in = reverse_mapping.find(i);
        if(it_live_in != reverse_mapping.end() && !it_live_in->second.empty())
            temp_node.fan_in = it_live_in->second.size();

        //chain fan_out
        auto it_live_out = chain_to_dependencies.find(i);
        if(it_live_out != chain_to_dependencies.end() && !it_live_out->second.empty())
            temp_node.fan_out = it_live_out->second.size();

        //chain live_in
        auto it_fan_in = un_reverse_mapping.find(i);
        if(it_fan_in != un_reverse_mapping.end() && !it_fan_in->second.empty())
            temp_node.live_in = it_fan_in->second.size();

        //chain live_out
        auto it_fan_out = un_dependencies.find(i);
        if(it_fan_out != un_dependencies.end() && !it_fan_out->second.empty())
            temp_node.live_out = it_fan_out->second.size();

        temp_node.nodes = ch;

        //Check whether chains is MEM
        for(auto& t_ins : ch){
            if((orig_graph[t_ins].opcode == "Load") || (orig_graph[t_ins].opcode == "Store"))
                temp_node.is_mem = true;
        }
        chain_graph.push_back(temp_node);
        i++;
    }

    //Counting chains' level
    findChainsLevel(0, 0, un_dependencies, chain_graph);
}


/**
 * Printing dependencies
 */
void
newGraph::printDependencies()
{
    cout << "Dependencies: " << endl;
    for(auto& m: this->un_dependencies){
        cout << m.first << ": " << endl;
        for(auto& v:m.second){
            cout << "\t" << v << endl;
        }
    }
}

/**
 * Writing new clean graph to a dot file format
 *
 */
void
newGraph::writeGraph(std::string str_name, BoostGraph& G)
{
    std::ofstream output_file;
    std::stringstream f;
    f << "output/" << str_name << ".dot";
    output_file.open(f.str(), ios::out);
    //Initiating dot graph file format
    std::stringstream temp;
    temp << "digraph G {\n";

    //Writing vertices to the clean graph
    for(uint32_t i = 0; i < boost::num_vertices(G); i++)
        temp << i << "[id=" << i << ", label=\"" << G[i].label << "\", ch=\"" << G[i].ch << "\", color=\"" << G[i].color << "\"" << "];\n";

    //Writing edges to the clean graph
    auto edges = boost::edges(G);
    for( auto v = edges.first; v != edges.second; ++v){
            auto source = boost::source(*v, G);
            auto target = boost::target(*v, G);

            temp << source << "->" << target << "\n";
    }

    //Closing the file
    temp << "}";

    output_file << temp.str();
    output_file.close();

}


/**
 * Filling chain levels
 */
void
newGraph::buildChainLevel()
{
    for(auto& ch: chain_graph)
        chain_level[ch.level].push_back(ch.chainId);
}


/**
 * Create chain blocks
 */
void
newGraph::createChainBlocks()
{

    uint32_t i = 0;

    // <BLOCKID, NUM_BYPASS>
    std::map<uint32_t, uint32_t> m_block_pass;

    //uint32_t temp_level = 0;
    for(auto& ch_l : chain_level){
        //if(temp_level++ > 2)
            //break;
        for(auto& ch_id : ch_l.second){
            auto ch = find_if(chain_graph.begin(), chain_graph.end(),
                [ch_id](const chainGraph &m) -> bool {return m.chainId == ch_id;});

            // If CHID = 0 then we create one block for each child
            if(ch_id == 0){
                ch->blockID = i;
                m_block_pass[i] = 0;
                auto ch_it = un_dependencies.find(ch->chainId);
                for(auto& child: ch_it->second){
                    //set child blockID
                    auto m_it = std::find_if(chain_graph.begin(), chain_graph.end(),
                            [child](const chainGraph &m) -> bool {return m.chainId == child;});
                    if(m_it->level == 1){
                        m_it->blockID = ++i;
                        m_block_pass[i]=1;
                    }
                }
            }

            // ELSE we traverse the graph
            else{
                auto ch_it = un_dependencies.find(ch->chainId);
                if(ch_it != un_dependencies.end()){
                    uint32_t cnt_child = 1;
                    for(auto& child : ch_it->second){
                        //cnt_child++;
                        auto parent_it = un_reverse_mapping.find(child);

                        bool is_single_parent = false;
                        if(parent_it->second.size() == 2)
                            is_single_parent = (parent_it->second.find(0) != parent_it->second.end());

                        //if(is_single_parent && cnt_child < graph_parameter.param_set.find("BLOCK_LIMIT")->second){
                        if(is_single_parent && (cnt_child == 1)) {
                            auto child_it = find_if(chain_graph.begin(), chain_graph.end(),
                                [child](const chainGraph &m) -> bool {return m.chainId == child;});
                            child_it->blockID = ch->blockID;
                            cnt_child++;
                        }

                        else if(is_single_parent && (cnt_child > 1)){
                            if((m_block_pass.find(ch->blockID)->second+1)  <= graph_parameter.param_set.find("BLOCK_LIMIT")->second){
                                auto child_it = find_if(chain_graph.begin(), chain_graph.end(),
                                    [child](const chainGraph &m) -> bool {return m.chainId == child;});
                                m_block_pass.find(ch->blockID)->second++;
                                child_it->blockID = ch->blockID;
                                cnt_child++;
                            }
                            else{
                                auto m_it = std::find_if(chain_graph.begin(), chain_graph.end(),
                                        [child](const chainGraph &m) -> bool {return m.chainId == child;});
                                if(m_it->blockID == 0){
                                    m_it->blockID = ++i;
                                    m_block_pass[i] = 1;
                                }
                                //cnt_child++;
                            }

                        }
                        else{
                            //set child blockID
                            auto m_it = std::find_if(chain_graph.begin(), chain_graph.end(),
                                    [child](const chainGraph &m) -> bool {return m.chainId == child;});
                            if(m_it->blockID == 0){
                                m_it->blockID = ++i;
                                m_block_pass[i] = 1;
                            }

                        }
                    }
                }
            }
        }
    }

    for(auto& ch : chain_graph){
        uint32_t temp_blID = ch.blockID;
        auto bl = find_if(block_chain.begin(), block_chain.end(),
            [temp_blID](const Block &m) -> bool {
            return m.blkID == temp_blID;
            });

        if(bl == block_chain.end()){
            Block new_bl(temp_blID);
            new_bl.chains.push_back(ch.chainId);
            block_chain.push_back(new_bl);
        }
        else{
            bl->chains.push_back(ch.chainId);
        }

    }
}

/**
 * Printing chains' stats into CSV file
 */
void
newGraph::printChainStats(string str_name)
{
    std::ofstream file;
    std::stringstream f;
    f << "output/" << str_name << "-chainSummary.csv";
    file.open(f.str());
    assert(file.is_open() && "Chain output file couldn't create");
    std::stringstream res;

    res << "ch_id, size, live_in, live_out, fan_in, fan_out, MEM_LD, MEM_ST, MEM_ALLOCA, other, level\n";
    for(auto& ch : chain_graph){
        res << ch.chainId << ", " << ch.size << ", " << ch.live_in << ", " << ch.live_out 
            << ", " << ch.fan_in << ", " << ch.fan_out << ", " << ch.ld_ins << ", " << ch.st_ins 
            << ", " << ch.alloca_ins << ", "<< ch.other << ", " << ch.level << "\n";
    }
    file << res.str();
    file.close();


    f.str("");
    res.str("");
    f << "output/" << str_name << "-chainStat.csv";
    file.open(f.str());
    assert(file.is_open() && "Chain output file couldn't create");

    //Computing graph stats
    //
    //
    //1) Max length
    int32_t max_level = 0;
    uint32_t max_size = 0;
    uint32_t ch_tot_size = 0;
    for(auto&c : chain_graph){
        ch_tot_size += c.size;
        if(c.level > max_level)
            max_level = c.level;
        if(c.size > max_size)
            max_size = c.size;
    }


    //2) ILP
    uint32_t m_m = 0;
    uint32_t sum_ilp = 0;

    //ILP histogram
    std::map<uint32_t,uint32_t> ch_ilp;
    for(auto &ch : chain_graph){
        auto ch_it = ch_ilp.find(ch.level);
        if(ch_it == ch_ilp.end())
            ch_ilp[ch.level]=1;
        else
            ch_it->second++;
    }

    //Find max ILP
    for(auto it_ch = ch_ilp.begin(); it_ch != ch_ilp.end(); ++it_ch){
        if(it_ch->second > m_m){
            m_m = it_ch->second;
        }
    }


    for(auto& c : ch_ilp)
        sum_ilp += c.second;

    std::ofstream ilp_file;
    std::stringstream f_n;

    //Dumping graph stats
    f_n << "output/" << str_name << "-chILP.csv";
    ilp_file.open(f_n.str());
    assert(ilp_file.is_open() && "Graph output file couldn't create");
    //res << "num_arg, num_const, num_ops, avg_ilp, geo_ilp, max_ilp, max_level, first_level_LD, max_MLP\n";
    //res << num_arg << ", " << num_const << ", " << num_ops << ", " << avg_ilp << ", " << geo_avg_ilp << ", "
        //<< max_ilp << ", " << length << ", " << firstlevelLD << ", " << max_mlp << endl;
    //file << res.str();
    //file.close();


    res.str("");
    res << "level, ilp" << endl;
    //DUMPING CHAIN ILP
    for(auto &ch : ch_ilp){
        res << ch.first << "," << ch.second << endl;
    }

    ilp_file << res.str();
    ilp_file.close();


    //3) MLP
    std::map<uint32_t,uint32_t> ch_mlp;
    for(auto &ch : chain_graph){
        auto ch_it = ch_mlp.find(ch.level);
        if(ch_it == ch_ilp.end()){
            if(ch.is_mem)
                ch_mlp[ch.level]=1;
        }
        else{
            if(ch.is_mem)
                ch_it->second++;
        }
    }

    uint32_t m_max_mlp = 0;
    for(auto it_ch = ch_mlp.begin(); it_ch != ch_mlp.end(); ++it_ch){
        if(it_ch->second > m_max_mlp){
            m_max_mlp = it_ch->second;
        }
    }

    //4) Chain size graph histogram
    std::map<uint32_t, uint32_t> ch_size_histo;
    for(auto& ch : chain_graph){
        if(ch.size < 5){
            auto ch_it = ch_size_histo.find(ch.size);
            if(ch_it == ch_size_histo.end())
                ch_size_histo[ch.size] = 1;
            else
                ch_size_histo[ch.size]++;
        }
        else{
            auto ch_it = ch_size_histo.find(5);
            if(ch_it == ch_size_histo.end())
                ch_size_histo[5] = 1;
            else
                ch_size_histo[5]++;
        }

    }

    //5) Summing intra communication
    uint32_t m_intra = 0;
    for(auto& c : chain_graph)
        m_intra += c.fan_in;

    //6) Live-in and Live-out hitogram
    std::map<uint32_t, uint32_t> ch_liveIn_hist;
    std::map<uint32_t, uint32_t> ch_liveOut_hist;
    for(auto& ch : chain_graph){
        auto chin_it = ch_liveIn_hist.find(ch.live_in);
        auto chout_it = ch_liveOut_hist.find(ch.live_out);
        if(ch.live_in < 5){
            if(chin_it == ch_liveIn_hist.end())
                ch_liveIn_hist[ch.live_in] = 1;
            else
                ch_liveIn_hist[ch.live_in]++;
        }
        else{
            chin_it = ch_liveIn_hist.find(5);
            if(chin_it == ch_liveIn_hist.end())
                ch_liveIn_hist[5] = 1;
            else
                ch_liveIn_hist[5]++;
        }

        //live_out

        if(ch.live_out < 5){
            if(chout_it == ch_liveOut_hist.end())
                ch_liveOut_hist[ch.live_out] = 1;
            else
                ch_liveOut_hist[ch.live_out]++;
        }
        else{
            chout_it = ch_liveOut_hist.find(5);
            if(chout_it == ch_liveOut_hist.end())
                ch_liveOut_hist[5] = 1;
            else
                ch_liveOut_hist[5]++;
        }
    }

    //4) Chain OP histogram
    std::map<uint32_t, std::vector<uint32_t>> ch_op_list;
    for(auto& ch : chain_graph){
        if(ch.size < 5){
            for(auto&op : ch.nodes)
                ch_op_list[ch.size].push_back(op);
        }
        else{
            for(auto&op : ch.nodes)
                ch_op_list[5].push_back(op);
        }

    }

    double m_avg_ilp = (double)sum_ilp/(double)ch_ilp.size();
    double m_avg_length = (double)ch_tot_size/(double)chain_graph.size();


    //Compute number of blocks
    std::set<uint32_t> un_block;
    for(auto& ch: chain_graph){
        un_block.insert(ch.blockID);
    }

    uint32_t m_inter = 0;
    for(auto& ch: chain_graph){
        m_inter += (ch.size - 1);
    }

    res << "num_ch, num_block, avg_ch_length, max_ch_length, ILP, MAX_ILP, max_level, max_mlp, inter_com, intra_com\n";
    res << chain_graph.size() << ", " << un_block.size() << ", " << m_avg_length << ", " << max_size << ", " << 
        m_avg_ilp << ", " << m_m << ", " << max_level << ", " << max_mlp << ", " << m_inter << ", " << m_intra << endl;

    file << res.str();
    file.close();

    //Printing histogram
    f.str("");
    f << "output/" << str_name << "-chainOpHisto.csv";
    file.open(f.str());
    assert(file.is_open() && "Chain size histogram file couldn't be open");

    std::stringstream m_header;
    for(uint32_t i = 0; i < 5; i++){
        std::map<std::string, std::uint32_t> temp_hist;
        auto ch_it = ch_op_list.find(i+1);
        if(ch_it != ch_op_list.end()){
            //counting instructions
            for(auto& ch : ch_it->second){
                auto n_it = temp_hist.find(orig_graph[ch].opcode);
                if(n_it == temp_hist.end())
                    temp_hist[orig_graph[ch].opcode] = 1;
                else
                    temp_hist[orig_graph[ch].opcode]++;
            }


            //printing instructions
            m_header << i+1 << endl;
            for(auto& ch : temp_hist){
                m_header << ch.first << ", ";
            }
            m_header << "\b\b" << " " << endl;

            for(auto& ch : temp_hist){
                m_header << ch.second << ", ";
            }
            m_header << "\b\b" << " " << endl;
        }

    }
    file << m_header.str();
    file.close();




    //chainsize histogram
    f.str("");
    res.str("");
    f << "output/" << str_name << "-chainSize.csv";
    file.open(f.str());
    assert(file.is_open() && "Chain size histogram file couldn't be open");
    res << "1,2,3,4,5" << endl;

    for(uint32_t i = 0; i < 5 ; ++i){
        auto ch_it = ch_size_histo.find(i+1);
        if(ch_it == ch_size_histo.end())
            res << "0, ";
        else
            res << ch_it->second << ", " ;
    }
    res << "\b\b" << " " << endl;

    file << res.str();
    file.close();


    //Live-in histogram
    f.str("");
    res.str("");
    f << "output/" << str_name << "-liveInHisto.csv";
    file.open(f.str());
    assert(file.is_open() && "Chain size histogram file couldn't be open");
    res << "1,2,3,4,5" << endl;

    for(uint32_t i = 0; i < 5 ; ++i){
        auto ch_it = ch_liveIn_hist.find(i+1);
        if(ch_it == ch_liveIn_hist.end())
            res << "0, ";
        else
            res << ch_it->second << ", " ;
    }
    res << "\b\b" << " " << endl;

    file << res.str();
    file.close();



    //Live-out histogram
    f.str("");
    res.str("");
    f << "output/" << str_name << "-liveOutHisto.csv";
    file.open(f.str());
    assert(file.is_open() && "Chain size histogram file couldn't be open");
    res << "1,2,3,4,5" << endl;

    for(uint32_t i = 0; i < 5 ; ++i){
        auto ch_it = ch_liveOut_hist.find(i+1);
        if(ch_it == ch_liveOut_hist.end())
            res << "0, ";
        else
            res << ch_it->second << ", " ;
    }
    res << "\b\b" << " " << endl;

    file << res.str();
    file.close();


    //Chain json file
    f.str("");
    res.str("");
    f << "output/" << str_name << "-chains.json";
    file.open(f.str());
    assert(file.is_open() && "Chain output file couldn't create");
    //res.str("");
    res << "{ \"Chains\" : [ \n";
    uint32_t i = 0;
    for(auto& ch: chain_graph){
        if(i++ < chain_graph.size()-1){
            res << "{ \"chain_id\" : " << ch.chainId << ", \"block_id\" : " << ch.blockID <<  ", \"size\" : " << ch.size << ", \"live_in\" : " << ch.live_in << ", \"live_out\" : " <<
                ch.live_out << ", \"fan_in\" : " << ch.fan_in << ", \"fan_out\" : " << ch.fan_out << ", \"LD\" : " << ch.ld_ins << ", \"ST\" : " <<
                ch.st_ins << ", \"ALLOCA\" : " << ch.alloca_ins <<", \"other\" : " << ch.other << ", \"level\" : " << ch.level << ", \"nodes\" : [";
            uint32_t j = 0;
            if (ch.nodes.size() > 0){
                for(auto&n : ch.nodes){
                    if(j++ < ch.nodes.size()-1)
                        res << "\"" << orig_graph[n].opcode << "(" << n << ")\"" << ",";
                    else
                        res << "\"" << orig_graph[n].opcode << "(" << n << ")\"" << "]";
                } 
            }
            else
                res << " ]";
               
            res << "},\n";
        }
        else{
             res << "{ \"chain_id\" : " << ch.chainId << ", \"block_id\" : " << ch.blockID << ", \"size\" : " << ch.size << ", \"live_in\" : " << ch.live_in << ", \"live_out\" : " <<
                ch.live_out << ", \"fan_in\" : " << ch.fan_in << ", \"fan_out\" : " << ch.fan_out << ", \"LD\" : " << ch.ld_ins << ", \"ST\" : " <<
                ch.st_ins << ", \"ALLOCA\" : " << ch.alloca_ins << ", \"other\" : " << ch.other << ", \"level\" : " << ch.level << ", \"nodes\" : [";
            uint32_t j = 0;
            if (ch.nodes.size() > 0){
                for(auto&n : ch.nodes){
                    if(j++ < ch.nodes.size()-1)
                        res << "\"" << orig_graph[n].opcode << "(" << n << ")\"" << ",";
                    else
                        res << "\"" << orig_graph[n].opcode << "(" << n << ")\"" << "]";
                } 
            }
            else
                res << " ]";  

            res << "}\n ]}\n" ;
        }
    }
    file << res.str();
    file.close();
}

/*
 * Breaking starting node
 */
void
newGraph::breakingStartNode()
{
    //Breaking the chains
    auto& ch_first = Chains[0];
    auto it = std::find(ch_first.begin(), ch_first.end(), this->first_node_id); 
    if(it != ch_first.end() && ch_first.size() > 1){
        std::vector<uint64_t> newVector(std::make_move_iterator(it+1), std::make_move_iterator(ch_first.end()));
        ch_first.erase(it+1, ch_first.end());
        Chains.push_back(newVector);
    }
}

/*
 * Breaking chains at every live-out
 */
void
newGraph::breakEveryLiveOut()
{
    bool out_flag = false;
    do{
    
        auto vertex_to_Chunk = vertexToChainMap(Chains);
        auto dependencies = dependenciesGenerate(orig_graph,Chains, vertex_to_Chunk);
        out_flag = breakOutHelper(Chains, dependencies, orig_graph);
    }
    while(out_flag);
}

/*
 * Breaking chains at every live-in
 */
void
newGraph::breakEveryLiveIn()
{
    bool in_flag = false;
    do{
        auto vertex_to_Chunk = vertexToChainMap(Chains);
        auto dependencies = dependenciesGenerate(orig_graph,Chains, vertex_to_Chunk);
        in_flag = breakInHelper(Chains, dependencies, orig_graph);
    }
    while(in_flag);
}


/*
 * Check whether number of chains' live-out is limited to two
 */
void
newGraph::checkLiveOutLimit()
{
    bool has_liveOut = false;
    do{
        if((Chains.size() > graph_parameter.param_set.find("MAX_NUM_CHAIN")->second))
                cout << Chains.size() << endl;
        assert((Chains.size() < graph_parameter.param_set.find("MAX_NUM_CHAIN")->second)
                && "Can't support huge graph!");
        auto vertex_to_Chunk = vertexToChainMap(Chains);
        auto dependencies = dependenciesGenerate(orig_graph, Chains, vertex_to_Chunk);
        auto chain_to_dependencies = histoDependency(dependencies);
        auto un_dependencies= uniqueDependency(chain_to_dependencies); 
        auto un_reverse_mapping = reverseMapping(un_dependencies);
        auto reverse_mapping = reverseMapping(chain_to_dependencies);
        has_liveOut = checkLiveOutHelper(chain_to_dependencies);
        
        if(has_liveOut){
            for(auto& live : chain_to_dependencies){
                if(live.second.size() > graph_parameter.param_set.find("MAX_LIVE_OUT")->second){
                    //traverse two node from begining of the chain and 
                    //then break the chain
                    //Since we are sure that each node can not have more than
                    //two live-out so if a chain has more than two live out it
                    //means size of the chain is also grater than two
                  
                    auto& rit = Chains[live.first];
                    uint32_t cnt_out = 0;
                    //for(auto& ch: rit){
                    for(auto ch = rit.rbegin(); ch!=rit.rend(); ch++){
                        if(*ch == 0)
                            continue;
                        auto edges = boost::out_edges(*ch, orig_graph);
                        OutEdge ei, ei_end;
                        for(boost::tie(ei, ei_end) = edges; ei != ei_end; ++ei){
                            auto target = boost::target(*ei, orig_graph);
                            if(std::find(rit.begin(), rit.end(), target) == rit.end())
                                cnt_out++;
                        }

                        if(cnt_out >= graph_parameter.param_set.find("MAX_LIVE_OUT")->second){
                            auto it = std::find(rit.begin(), rit.end(), *ch);
                            std::vector<uint64_t> newVector(std::make_move_iterator(it), std::make_move_iterator(rit.end()));
                            rit.erase(it, rit.end());
                            rit.shrink_to_fit();
                            Chains.push_back(newVector);
                            break;
                        }
                    }
                }
            }
        }
    }
    while(has_liveOut);
}


/*
 * Check whether number of chains' live-out is limited to two
 */
void
newGraph::checkLiveInLimit()
{
    //Cheking number of livei-in
    bool has_liveIn = false;
    do{

        cout << "Chain Size: " << Chains.size() << endl;
        assert((Chains.size() < graph_parameter.param_set.find("MAX_NUM_CHAIN")->second)
                && "Can support huge graph!");
        auto vertex_to_Chunk = vertexToChainMap(Chains);
        auto dependencies = dependenciesGenerate(orig_graph,Chains, vertex_to_Chunk);
        auto chain_to_dependencies = histoDependency(dependencies);
        auto un_dependencies= uniqueDependency(chain_to_dependencies); 
        auto un_reverse_mapping = reverseMapping(un_dependencies);
        auto reverse_mapping = reverseMapping(chain_to_dependencies);
        has_liveIn = checkLiveInHelper(reverse_mapping);
        
        if(has_liveIn){
            for(auto& live : reverse_mapping){
                if(live.second.size() > graph_parameter.param_set.find("MAX_LIVE_IN")->second){
                    //traverse two nodes from begining of the chain and 
                    //then break the chain
                    //Since we are sure that each node can not have more than
                    //two live-out so if a chain has more than two live out it
                    //means size of the chain is also grater than two
                  
                    auto& rit = Chains[live.first];
                    uint32_t cnt_in = 0;
                    //for(auto& ch: rit){
                    for(auto ch = rit.begin(); ch!=rit.end(); ch++){
                        auto edges = boost::in_edges(*ch, orig_graph);
                        InEdge ei, ei_end;
                        for(boost::tie(ei, ei_end) = edges; ei != ei_end; ++ei){
                            //auto target = boost::target(*ei, orig_graph);
                            auto source = boost::source(*ei, orig_graph);
                            if(std::find(rit.begin(), rit.end(), source) == rit.end())
                                cnt_in++;
                        }

                        //TODO fix live-in
                        if(cnt_in > graph_parameter.param_set.find("MAX_LIVE_IN")->second){
                            cout << orig_graph[*ch].name << endl;;
                        }
                        assert(cnt_in <= graph_parameter.param_set.find("MAX_LIVE_IN")->second
                                && "Node with more than two live-in!!!");

                        if(cnt_in == graph_parameter.param_set.find("MAX_LIVE_IN")->second){
                            auto it = std::find(rit.begin(), rit.end(), *ch);
                            std::vector<uint64_t> newVector(std::make_move_iterator(it+1), std::make_move_iterator(rit.end()));
                            rit.erase(it+1, rit.end());
                            rit.shrink_to_fit();
                            Chains.push_back(newVector);
                            break;
                        }
                    }
                }
            }
        }
    }
    while(has_liveIn);
}

/**
 * Check wether can we merge a chain's child with itself or not
 */
bool
newGraph::mergeChain(std::vector<std::vector<uint64_t>>& m_chain, const BoostGraph& orig_graph)
{
    auto vertex_to_chunk = vertexToChainMap(m_chain);
    //auto dependencies = dependenciesGenerate(orig_graph,m_chain, vertex_to_chunk);

    for(uint32_t i = 1; i < m_chain.size(); i++){
        bool has_loop = false;
        auto& ch = m_chain[i];
        auto& node = ch.back();
        auto edges = boost::out_edges(node, orig_graph);
        OutEdge ei, ei_end;
        for(boost::tie(ei, ei_end) = edges; ei != ei_end; ++ei){
            //iterate over out edges of the last node, check whether can we 
            //merge two chains or not
            auto target = boost::target(*ei, orig_graph);
            auto t_it = vertex_to_chunk.find(target); 

            if((t_it->second != i) && (m_chain[t_it->second].front() == target)){
                std::vector<std::vector<uint64_t>> temp_chain = m_chain;
                temp_chain[i].insert(temp_chain[i].end(), temp_chain[t_it->second].begin(), temp_chain[t_it->second].end() );
                temp_chain.erase(temp_chain.begin() + t_it->second);

                //Check wether new chain has loop or not
                auto temp_vtc = vertexToChainMap(temp_chain);
                auto dependencies = dependenciesGenerate(orig_graph,temp_chain, temp_vtc);
                auto chain_to_dependencies = histoDependency(dependencies);
                auto un_dependencies= uniqueDependency(chain_to_dependencies); 

                list<uint32_t> loop_stack;
                has_loop = false;
                loop_stack.push_back(0);
                has_loop = checkLoopComplete(0, un_dependencies, loop_stack);

                if(!has_loop){
                    //if it's a valid merge then do merge on the real chain
                    m_chain[i].insert(m_chain[i].end(), m_chain[t_it->second].begin(), m_chain[t_it->second].end() );
                    m_chain.erase(m_chain.begin() + t_it->second);
                    cout << "Chain Size: " << m_chain.size() << endl;
                    return true;
                }
            }

        }

    }

    return false;
}

/**
 * Find graph level
 */
void
newGraph::findGrahpLevel(BoostGraph& G, uint32_t id, std::vector<int32_t>& m_level, int32_t current_level)
{

    if(m_level[id] < current_level){
        if(id == 0){
            m_level[id] = current_level++;
            typename boost::graph_traits<BoostGraph>::out_edge_iterator ei, ei_end;
            if(boost::out_degree(id, G) == 0)
                return;
            else{
                for(boost::tie(ei, ei_end) = boost::out_edges(id, G); ei != ei_end; ei++){
                    auto target = boost::target(*ei, G);
                    if(boost::in_degree(target,G) == 1)
                        findGrahpLevel(G, target, m_level, current_level);
                }
            }
        }
        else{
            m_level[id] = current_level++;
            typename boost::graph_traits<BoostGraph>::out_edge_iterator ei, ei_end;
            if(boost::out_degree(id, G) == 0)
                return;
            else{
                for(boost::tie(ei, ei_end) = boost::out_edges(id, G); ei != ei_end; ei++){
                    auto target = boost::target(*ei, G);
                    findGrahpLevel(G, target, m_level, current_level);
                }
            }
        }
    }
    else
        return;
}

/**
 * Printing graph stats into CSV file
 */
void
newGraph::printGraphstats(std::string str_name)
{
    std::ofstream file;
    std::stringstream f;

    //Dumping graph stats
    f << "output/" << str_name << "-graphStat.csv";
    file.open(f.str());
    assert(file.is_open() && "Graph output file couldn't create");
    std::stringstream res;
    res << "num_arg, num_const, num_ops, avg_ilp, geo_ilp, max_ilp, max_level, first_level_LD, max_MLP\n";
    res << num_arg << ", " << num_const << ", " << num_ops << ", " << avg_ilp << ", " << geo_avg_ilp << ", "
        << max_ilp << ", " << length << ", " << firstlevelLD << ", " << max_mlp << endl;
    file << res.str();
    file.close();

    //Dumping graph histogram
    f.str("");
    res.str("");
    f << "output/" << str_name << "-graphHisto.csv";
    file.open(f.str());
    assert(file.is_open() && "Graph histo output file couldn't create");

    for(auto& c: llvm_ins.instruction_llvm)
        res << c.first << ", " ;
    res << "\b\b" << " " << endl;

    for(auto& c: llvm_ins.instruction_llvm){

        auto it_ins = ins_histo.find(c.first);
        if(it_ins == ins_histo.end())
            res << 0 << ", ";
        else
            res << it_ins->second << ", ";
    }
    res << "\b\b" << " " << endl;

    file << res.str();
    file.close();

}

void
newGraph::writeChain(std::string str_name)
{
    std::ofstream output_file;
    std::stringstream f;
    f << "output/" << str_name << ".dot";
    output_file.open(f.str(), ios::out);
    std::stringstream temp_string;
    temp_string << "digraph G{\n";

    //Writing vertices to the clean graph
    for(auto& ch : chain_graph){
        temp_string << ch.chainId << "[id=" << ch.chainId << ", block_id=" << ch.blockID <<
            ", level=" << ch.level << " style=\"filled\", fillcolor= \"" << brewer_color[ch.blockID % 12]  << "\", fontcolor=\"black\"" << "];\n";
    }

    for(auto&depen : un_dependencies){
        for(auto& d: depen.second){
            temp_string << depen.first << " -> " << d << ";\n";
        }

    }
    temp_string << "}";
    output_file << temp_string.str();
    output_file.close();
}


void
newGraph::buildILPGraph()
{
    std::vector<chainDependency> new_edges;
    std::vector<uint32_t>       new_vertices;
    for(auto& ch : chain_graph){
        //std::map<uint32_t, std::vector<uint32_t>> parent_set;
        std::vector<uint32_t> parent_set;
        auto ch_it = un_reverse_mapping.find(ch.chainId);
        if(ch_it == un_reverse_mapping.end()){
            for(auto& n : ch.nodes)
                new_vertices.push_back(n);
            continue;
        }
        for(auto& ch_p : ch_it->second){
            auto  p_it = find_if(chain_graph.begin(), chain_graph.end(),
                [ch_p](const chainGraph &m) -> bool {return m.chainId == ch_p;});

            std::sort(p_it->nodes.begin(), p_it->nodes.end());
            parent_set.push_back(p_it->nodes.back());

        }

        std::sort(ch.nodes.begin(), ch.nodes.end());

        for(uint32_t i = 0; i < ch.nodes.size(); i++){
            if(i < ch.nodes.size()-1){
                chainDependency temp_edge;
                temp_edge.source = ch.nodes[i];
                temp_edge.target = ch.nodes[i+1];
                new_edges.push_back(temp_edge);
            }
            
            if(i == 0){
                for(auto& t_p : parent_set){
                    //auto  p_it = find_if(chain_graph.begin(), chain_graph.end(),
                        //[t_p](const chainGraph &m) -> bool {return m.chainId == t_p;});
                    chainDependency temp_edge;
                    temp_edge.source = t_p;
                    temp_edge.target= ch.nodes.front();
                    new_edges.push_back(temp_edge);
                }
            }

            new_vertices.push_back(ch.nodes[i]);
        }
    }

    //MAKING NEW GRAPH
    std::map<uint32_t, uint32_t> v_map;

    for(uint32_t i = 0 ; i < new_vertices.size(); i++){
        auto new_v = boost::add_vertex(ilp_graph);
        orig_graph[new_v].name   = new_vertices[i];
        orig_graph[new_v].label  = "NOP";
        orig_graph[new_v].ch     = "1";
        orig_graph[new_v].opcode = "NOP";
        orig_graph[new_v].color  = "black";

        v_map.emplace(new_v,i);
    }
    for(auto& edge: new_edges){
        boost::add_edge(v_map.find(edge.source)->second, v_map.find(edge.target)->second, ilp_graph);
    }


}


void
newGraph::ilpStat(std::string str_name)
{
    vector<int32_t> m_level(boost::num_vertices(orig_graph));
    num_ops = boost::num_vertices(orig_graph);
    for(auto& l : m_level)
        l = -1;

    findGrahpLevel(ilp_graph, 0, m_level, 0);


    //uint32_t m_i=0;
    //uint32_t sum_gilp=0;
    //uint32_t mul_glip=1;

    std::map<uint32_t,uint32_t> ilp_histo;
    for(auto &ml : m_level){
        auto ml_it = ilp_histo.find(ml);
        if(ml_it == ilp_histo.end())
            ilp_histo[ml]=1;
        else
            ml_it->second++;
    }

    uint32_t set1 = 0;
    uint32_t set2 = 0;
    uint32_t set3 = 0;

    uint32_t m_sum = 0;
    double avg_ilp = 0;
    for(auto& m : ilp_histo){
        if((m.second == 1) || (m.second == 2))
            set1++ ;
        else if((m.second== 3) || (m.second== 4))
            set2++;
        else
            set3++;

        m_sum += m.second;
    }

    avg_ilp = (double)m_sum/(double)ilp_histo.size();

    std::ofstream file;
    std::stringstream f;
    f << "output/" << str_name << "-ilpHisto.csv";
    file.open(f.str());
    assert(file.is_open() && "Chain output file couldn't create");
    std::stringstream res;

    res << str_name << "," << set1 << "," << set2 << "," << set3 << "," << avg_ilp;
    file << res.str();
    file.close();




}
