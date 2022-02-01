import pygraphviz as pgv
import networkx as nx
from networkx.drawing.nx_pydot import write_dot
from collections import defaultdict
from collections import OrderedDict
import collections


def checkEdgeOut(ng,limit):
    # TODO add COPY node
    for n in nx.nodes(ng):
        if len(list(ng.successors(n))) > limit:
            print('error out' + n)


def checkEdgeIn(ng,limit):
    # TODO add COPY node
    for n in nx.nodes(ng):
        if len(list(ng.in_edges(n))) > limit:
            print('error in ' + n)


def build_chain_dic_helper(ng):
    # Building chain dicitonary
    chdic = defaultdict(list)
    for n in nx.nodes(ng):
        chdic[ng.nodes[n]['cid']].append(n)
        if ng.nodes[n]['opcode'] == 'BB':
            headnode = n
            headcid  = ng.nodes[n]['cid']
    return chdic


def build_chain_edge_helper(ng):
    # Building chain's edges
    ch_edge = defaultdict(list)
    for n in nx.nodes(ng):
        for t in ng.successors(n):
            if ng.nodes[n]['cid'] != ng.nodes[t]['cid']:
                ch_edge[ng.nodes[n]['cid']].append(ng.nodes[t]['cid'])
    return ch_edge


def build_chain_edge_reverse_helper(ng):
    # Building chain's edges
    ch_edge = defaultdict(list)
    for n in nx.nodes(ng):
        for (s,t) in ng.in_edges(n):
            if ng.nodes[n]['cid'] != ng.nodes[s]['cid']:
                ch_edge[ng.nodes[n]['cid']].append(ng.nodes[s]['cid'])
    return ch_edge


def SetColor(ng, n, rank):
	r = rank % 6 
	if (r == 0):
		ng.nodes[n]['style'] = 'filled'
		ng.nodes[n]['fillcolor'] = 'red'
	if (r == 1):
		ng.nodes[n]['style'] = 'filled'
		ng.nodes[n]['fillcolor'] = 'blue'
	if (r == 2):
		ng.nodes[n]['style'] = 'filled'
		ng.nodes[n]['fillcolor'] = 'green'
	if (r == 3):
		ng.nodes[n]['style'] = 'filled'
		ng.nodes[n]['fillcolor'] = 'yellow'
	if (r == 4):
		ng.nodes[n]['style'] = 'filled'
		ng.nodes[n]['fillcolor'] = 'orange'
	if (r == 5):
		ng.nodes[n]['style'] = 'filled'
		ng.nodes[n]['fillcolor'] = 'violet'


def limitChainOut(ng, limit):
    has_liveout = True
    while has_liveout:
        has_liveout = False
        ch_dic = build_chain_dic_helper(ng)
        ch_edge = build_chain_edge_helper(ng)

        new_cid = len(ch_dic)
        for key, value in ch_edge.items():
            if len(value) > limit:
                has_liveout = True
                temp_ch = ch_dic[key]
                temp_ch.sort()

                # iterate from top node and if it has an edge outside of the chain
                # break the chain from that point
                update = True
                for c in temp_ch:
                    if update:
                        for t in ng.successors(c):
                            if ng.nodes[c]['cid'] != ng.nodes[t]['cid']:
                                update = False
                    else:
                        ng.nodes[c]['cid'] = new_cid
                        SetColor(ng, c, new_cid)


                new_cid += 1



def limitChainIn(ng, limit):
    has_livein = True
    while has_livein:
        has_livein = False
        ch_dic = build_chain_dic_helper(ng)
        ch_edge = build_chain_edge_reverse_helper(ng)

        new_cid = len(ch_dic)
        for key, value in ch_edge.items():
            if len(value) > limit:
                has_livein = True
                temp_ch = ch_dic[key]
                int_temp_ch = [int(x) for x in temp_ch]
                int_temp_ch.sort()
                temp_ch = [str(x) for x in int_temp_ch]

                # iterate from top node and if it has an edge outside of the chain
                # break the chain from that point
                update = True
                for c in temp_ch:
                    if update:
                        for (s,t) in ng.in_edges(c):
                            if ng.nodes[c]['cid'] != ng.nodes[s]['cid']:
                                update = False
                    else:
                        ng.nodes[c]['cid'] = new_cid
                        SetColor(ng, c, new_cid)


                new_cid += 1
