#ifndef __DILWORTHDECOMPOSITION__
#define __DILWORTHDECOMPOSITION__

#include <vector>
#include <set>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/max_cardinality_matching.hpp>
#include "Graph.hpp"



// Boost undirected graph
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS> UndirectedGraph;


namespace dilworth{

    template<typename TGraph>
    class DilworthDecompose
    {
        public:
            DilworthDecompose(){}
            std::vector<std::vector<uint64_t>>
                generateMinimalChains(const TGraph& G);
    };
    template < typename TGraph> std::vector<std::vector<uint64_t>>
    DilworthDecompose<TGraph>::generateMinimalChains(const TGraph& G)
    {
        typedef typename TGraph::vertex_descriptor Vertex;

        auto num_vertices = boost::num_vertices(G);
        UndirectedGraph UG(2*num_vertices);

        typename boost::graph_traits<TGraph>::edge_iterator ei, ei_end;

        for(boost::tie(ei, ei_end) = boost::edges(G); ei != ei_end; ++ei){
            boost::add_edge(source(*ei,G), 
                    num_vertices + boost::target(*ei, G), UG);
        }

        std::vector<std::vector<uint64_t>> minimalChains;

        std::vector<Vertex> mateMap(boost::num_vertices(UG));

        bool success = boost::checked_edmonds_maximum_cardinality_matching(UG, &mateMap[0]);
        assert(success && "Doesn't work!");

        auto NullV = boost::graph_traits<UndirectedGraph>::null_vertex();

        std::set<Vertex> InChains;
        bool Flag = false;
        do{
            //Find the first non-nul and follow chain
            Flag = false;
            for(std::uint64_t I = 0; I < num_vertices; I++){
                if(mateMap[I] != NullV){
                    //Begin a chain
                    std::vector<std::uint64_t> Chain;
                    auto Next = I;
                    do {
                        if(Next >= num_vertices) Next -= num_vertices;
                        if(InChains.count(Next) == 0) {
                            Chain.push_back(Next);
                            InChains.insert(Next);
                        }
                        auto prev = Next;
                        Next = mateMap[Next];
                        mateMap[prev] = NullV;
                    }while( Next != NullV );
                    minimalChains.push_back(Chain);
                    Flag = true;
                }
            }
        }while(Flag);

        BGL_FORALL_VERTICES_T(V, G, TGraph){
            if(InChains.count(V) == 0) {
                std::vector<Vertex> tempChain;
                tempChain.push_back(V);
                minimalChains.push_back(tempChain);
            }
        }

        uint32_t sum = 0;
        for(auto &m : minimalChains) {
            sum += m.size();
        }

        assert(sum == boost::num_vertices(G) &&
                "All vertices not accounted for");

        return minimalChains;
    }

}
#endif
