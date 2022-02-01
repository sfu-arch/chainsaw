import sys
import os
import pygraphviz as pgv
import networkx as nx
from networkx.drawing.nx_pydot import write_dot
from collections import defaultdict
from collections import OrderedDict
# from scipy import stats as sp


nodeid = 1
unmatched = 0
fig = "graph.dot"

def PrintGraph(g):
    global fig
    a = nx.nx_agraph.to_agraph(g)
    a.layout()
    a.draw(fig)

# create networkx graph
def CreateGraph(filename, attrs):
    B=pgv.AGraph(filename)
    ng = nx.DiGraph(B)	
    for attr in attrs:
        attr_name = attr[0]
        attr_val = attr[1]
        nx.set_node_attributes(ng, attr_name, attr_val)
    return(ng)


def RemoveFakeEdges(ng):
    for x,y,z in ng.edges(data=True):
        if z.has_key('style'):
            if z['style'] == 'dotted':
                ng.remove_edge(x,y)

def RemoveRootNode(ng):
    ng.remove_node('1')


def GetParents(ng, n):
    l = []
    for e in nx.all_neighbors(ng, n):
        if (e, n) in nx.edges(ng, e):
            l.append(e)
    return l


def AddNode(ng, opcode, color='black'):
    global nodeid
    ident = 'N' + str(nodeid)
    nodeid += 1
    ng.add_node(ident);
    ng.nodes[ident]['opcode'] = opcode
    ng.nodes[ident]['label'] = opcode + '(' + ident + ')' 
    ng.nodes[ident]['color'] = color
    AddEdge(ng, '1', ident, 'dotted')
    return ident

def AddEdge(ng, src, dst, style=''):
	ng.add_edge(src, dst)
	ng[src][dst]['style'] = style

	#print 'added edge', src, dst

def RemoveNode(ng, n):
	for (src,dst) in nx.edges(ng, n):
		RemoveEdge(ng, src, dst)
	for e in nx.all_neighbors(ng, n):
		RemoveEdge(ng, e, n)
	ng.remove_node(n)

	#print 'removed node', n


def RemoveEdge(ng, src, dst):
	ng.remove_edge(src, dst)

	#print 'removed edge', src, dst


def SetColor(ng, n, rank):
	r = rank % 4 
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

# def SetEdgeColor(ng, (u,v), rank):
# 	r = rank % 4 
# 	ng[u][v]['penwidth']='5'
# 	if (r == 0):
# 		ng[u][v]['color'] = 'red'
# 	if (r == 1):
# 		ng[u][v]['color'] = 'blue'
# 	if (r == 2):
# 		ng[u][v]['color'] = 'green'
# 	if (r == 3):
# 		ng[u][v]['color'] = 'yellow'
		

def ExpandGEP(ng):
	rank = 0
	for n in nx.nodes(ng):
		if ng.nodes[n]['opcode'] != 'GetElementPtr':
			continue
		#print ng.nodes[n]
		l = []
		for p in GetParents(ng, n):
			if p == '1':
				RemoveEdge(ng, p, n)
				continue
			if len(l) == 0:
				RemoveEdge(ng, p, n)
				l.append(p)
				continue
			mul = AddNode(ng, 'Mul')
			SetColor(ng, mul, rank)
			RemoveEdge(ng, p, n)
			AddEdge(ng, p, mul)
			l.append(mul)
		while len(l) >= 2:
			n1 = l[0]
			n2 = l[1]
			l = l[2:]
			add = AddNode(ng, 'Add')
			SetColor(ng, add, rank)
			AddEdge(ng, n1, add)
			AddEdge(ng, n2, add)
			l.append(add)
		add = l[0]
		for (src,dst) in nx.edges(ng, n):
			AddEdge(ng, add, dst)
			RemoveEdge(ng, n, dst)
		RemoveNode(ng, n)
		rank += 1


def RemoveDataNodes(ng):
	for n in nx.nodes(ng):
		if ng.nodes[n]['color'] == 'black':
			continue
		if ng.nodes[n]['color'] == 'red':
			continue
		if ng.nodes[n]['color'] == 'blue':
			for (x,y) in nx.edges(ng, n):
				ng.nodes[y]['liveins'] = '1'

		RemoveNode(ng, n)


def Copy0(n):
	return (n + '_0')

def Copy1(n):
	return (n + '_1')

def Uncopy(n):
	return n[0:-2]

def FlipCopy(n):
	ndas = n[:-1] + str(1 - int(n[-1]))
	return ndas


def GetDirectedEdge(ng, seed):
	src,dst = seed
	s = Uncopy(src)
	d = Uncopy(dst)


	if (s,d) in nx.edges(ng, s):
		return (s, d)
	else:
		return (d, s)


def GetEdge(g, s, n):
	for  e in nx.all_neighbors(g, Copy0(n)):
		(u,v) = (Copy0(n),e)
		if (u,v) in s:
			return (u,v)
		else:
			return (v,u)
	for  e in nx.all_neighbors(g, Copy1(n)):
		(u,v) = (Copy1(n),e)
		if (u,v) in s:
			return (u,v)
		else:
			return (v,u)
	return (None, None)
	

def GetMate(g, n):
	for e in nx.all_neighbors(g, n):
		return e
	return None

def SetChain(dag, seed, cid):
	u,v = seed
	global unmatched

	dag.nodes[u]['cid'] = cid
	dag.nodes[v]['cid'] = cid
	dag[u][v]['cid'] = cid
	unmatched = unmatched - 1 
	#print 'unmatched = ', unmatched

def ExtendChain(srcdst, uv, chain, chains, s, g, dag):
	src, dst = srcdst
	u, v = uv
	s.remove((u,v))
	RemoveEdge(g, u, v)
	# if src == '1':
		# return
	#print src, dst
	chain.append((src,dst))
	SetColor(dag, src, len(chains))
	SetColor(dag, dst, len(chains))
	# SetEdgeColor(dag, (src,dst), len(chains))
	SetChain(dag, (src,dst), len(chains))
	

def ExtendForward(g, dag, s, seed, chain, chains):
	(src, dst) = GetDirectedEdge(dag, seed)
	ExtendChain((src,dst), seed, chain, chains, s, g, dag)

	while dst != None:
		seed = GetEdge(g, s, dst)
		if seed == (None, None):
			break
		(src1,dst1) = GetDirectedEdge(dag, seed)
		if (src1 != dst):
			break
		(src,dst) = (src1,dst1)
		ExtendChain((src,dst), seed, chain, chains, s, g, dag)
		


def ExtendBackward(g, dag, s, seed, chain, chains):
	(src, dst) = GetDirectedEdge(dag, seed)

	while dst != None:
		seed = GetEdge(g, s, src)
		if seed == (None, None):
			break
		(src1,dst1) = GetDirectedEdge(dag, seed)
		if (dst1 != src):
			break
		(src,dst) = (src1,dst1)
		ExtendChain((src,dst), seed, chain, chains, s, g, dag)
	



def Dilworth(ng):
    global unmatched
    
    g = nx.Graph()
    gdas = nx.Graph() 
    
    # Creating bipirate graph from a partial order, and partitioning into chains according to a matching
    for n in nx.topological_sort(ng):
        g.add_node(Copy0(n))
        g.nodes[Copy0(n)]['label'] = Copy0(ng.nodes[n]['label'])
        g.add_node(Copy1(n))
        g.nodes[Copy1(n)]['label'] = Copy1(ng.nodes[n]['label'])     
        gdas.add_node(Copy0(n))
        gdas.add_node(Copy1(n))
        
    for n in nx.nodes(ng):
        for (u,v) in nx.edges(ng, n):
            g.add_edge(Copy0(n), Copy1(v))

    # Running matching algorithm
    s = nx.maximal_matching(g)
    gdas.add_edges_from(s)
    unmatched = nx.number_of_nodes(ng)
    # print 'unmatched = ', unmatched
    # nx.drawing.nx_pydot.write_dot(gdas,'sample.dot')
    # print s
    
    chains = []
    while len(s) > 0:
        # print '********', len(s)
        seed = next(iter(s))	
        chain = []
        
        ExtendForward(gdas, ng, s, seed, chain, chains)
                # print '///////////'
        ExtendBackward(gdas, ng, s, seed, chain, chains)
                # print len(s)
        # print len(chain) + 1
        
        unmatched = unmatched - 1
        chains.append(chain)

    # for x in range(0,unmatched):
        # print 1 


def CheckStartNode(ng):
    count = 0
    for n in nx.nodes(ng):
        if ng.nodes[n]['opcode'] == 'BB':
            count += 1
        if count > 1:
            return 1
    return 0


def BreakHeadNode(ng):

    # Getting list of ch IDs
    chdic = defaultdict(list)
    for n in nx.nodes(ng):
        chdic[ng.nodes[n]['cid']].append(n)
        if ng.nodes[n]['opcode'] == 'BB':
            headnode = n
            headcid  = ng.nodes[n]['cid']

    # Updating ch IDs
    lastcid = len(chdic)
    for n in chdic[headcid]:
        if n != headnode:
            ng.nodes[n]['cid'] = lastcid
            SetColor(ng, n, lastcid)
