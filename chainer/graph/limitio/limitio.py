import sys
import os
import pygraphviz as pgv
import networkx as nx
from networkx.drawing.nx_pydot import write_dot
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import json
from collections import OrderedDict
import numpy as np
import collections

chainid = 0
nodeid = 0

def Print(flag, s):
	res = 0
	if flag == 'dbgnode':
		res = 0
	elif flag == 'dbgchain':
		res = 0 
	elif flag == 'dbgloops' :
		res = 0
	elif flag == 'dbgcolor' :
		res = 0
	elif flag == 'dbgoutput':
		res = 0
	elif flag == 'dbgbreak':
		res = 0
	elif flag == 'dbgid':
		res = 0

	
	if res == 1:
		print flag, ":", s
	

##create networkx graph
def CreateGraph(filename, attrs):
    B=pgv.AGraph(filename)
    ng = nx.DiGraph(B)	
    for attr in attrs:
        attr_name = attr[0]
        attr_val = attr[1]
        nx.set_node_attributes(ng, attr_name, attr_val)
    return(ng)


def AddChainNode(cg, c, n):
    if c in nx.nodes(cg):
        return
    cg.add_node(c)
	
	

def AddChainNodes(cg, ng):
    for n in nx.nodes(ng):
        try:
            Print('dbgnode', ng.node[n]['cid'])
            AddChainNode(cg, ng.node[n]['cid'], n)
        except Exception, e:
            ng.node[n]['cid'] = 'N_'+n
            AddChainNode(cg, 'N_'+n, 'N'+n)	


def CreateNodeList(ng, c):
	l = []
	Print("dbgchain", "*********")
	Print("dbgchain", c)
	for n in nx.nodes(ng):
		if ng.node[n]['cid'] != c:
			continue
		Print("dbgchain", n)
		pos = 0
		a = nx.ancestors(ng, n)
		for e in l:
			if e in a:
				pos += 1
				continue
			else:
				break

		if pos == 0:
			ldas = []
			ldas.append(n)
			Print("dbgchain", ldas)
			ldas += l[:]
			l = ldas
		else:
			Print("dbgchain", l[0:pos]) 
			ldas = l[0:pos] 
			Print("dbgchain", ldas) 
			ldas.append(n)
			Print("dbgchain", ldas) 
			ldas += l[pos:]
			Print("dbgchain", ldas) 
			l = ldas
		Print("dbgchain", l)
	return l


def CreateChainDict(cg, ng):
	d = {}
	for c in nx.nodes(cg):
		l1 = CreateNodeList(ng, c)
		Print("dbgchain", c)
		Print("dbgchain", l1)
		d[c] = [] 
		for p in l1:
			d[c].append(p)
	Print("dbgchain", d)
	return d


def NewChainId():
	global chainid
	cid = 'CH_' + str(chainid)
	chainid += 1
	return cid


def CheckInEdges(ng):
	for n in nx.nodes(ng):
		x = len(nx.edges(ng, n))
		count = 0
		for y in nx.all_neighbors(ng, n):
			if y != '1':
				count += 1
		try:
			count += ng.node[n]['liveins']
		except Exception, e:
			count += 0
		if count - x > 2:
			if n != '1':
				print 'too many incoming edges', n
def GetNewNode():
	global nodeid
	v = 'V_' + str(nodeid)
	nodeid += 1
	return v

def LimitOutEdges(ng, cg, d):
	for n in nx.nodes(ng):
		if n == '1' :
			continue
		l = []

		cnt = 0
		for (u,v) in nx.edges(ng, n):
			if ng.node[u]['cid'] == ng.node[v]['cid']:
				continue
			l.append((u,v))
			cnt += 1
		if cnt <= 2:
			continue

		Print("dbgoutput", "***too many outputs***")
		Print("dbgoutput", n)
		Print("dbgoutput", nx.edges(ng, n))
		seed = n
		Print("dbgoutput", l)
		for (p,q) in l[1:-1]:
			Print("dbgoutput", (p,q))
			v = GetNewNode()
			ng.add_node(v)
			ng.node[v]['opcode'] = 'Nop'
			ng.node[v]['cid'] = v
			ng.node[v]['label'] = ng.node[v]['opcode'] + '(' + v + ')'
			ng.node[v]['color'] = 'black'

                        ng.add_edge('1',v)
                        ng['1'][v]['style'] = 'dotted'
                        cg.add_node(v)
                        d[v] = []
                        d[v].append(v)

			ng.add_edge(seed, v)
			ng.add_edge(v, q)
			ng.remove_edge(n, q)

			Print("dbgoutput", nx.edges(ng, n))
			Print("dbgoutput", nx.edges(ng, seed))
			Print("dbgoutput", nx.edges(ng, v))

			seed = v

		(p,q) = l[-1]
		ng.add_edge(seed, q)
		ng.remove_edge(n, q)

		Print("dbgoutput", nx.edges(ng, n))
		Print("dbgoutput", nx.edges(ng, seed))


def BreakChain(ng, cg, d, c, pos):
	Print("dbgbreak", "****breaking chains*****")
	Print("dbgbreak", d[c])
	if len(d[c]) == 1:
		return False
	if pos == 0:
		cid = NewChainId()
		cg.add_node(cid)
		d[cid] = []
		for x in d[c][1:]:
			d[cid].append(x)
			ng.node[x]['cid'] = cid
		d[c] = d[c][0:1]

		Print("dbgbreak", d[cid])
	elif pos == len(d[c]) - 1:
		cid = NewChainId()
		cg.add_node(cid)
		d[cid] = []
		d[cid].append(d[c][pos])
		ng.node[d[c][pos]]['cid'] = cid
		d[c] = d[c][0:pos]

		Print("dbgbreak", d[cid]) 

	else:
		#cid = NewChainId()
		#cg.add_node(cid)
		#d[cid] = []
		#d[cid].append(d[c][pos])
		#ng.node[d[c][pos]]['cid'] = cid

		#Print("dbgloops", d[cid]) 

		cid = NewChainId()
		cg.add_node(cid)
		d[cid] = []
		for x in d[c][pos: ]:
			d[cid].append(x)
			ng.node[x]['cid'] = cid

		d[c] = d[c][0:pos]

		Print("dbgbreak", d[cid]) 

	Print("dbgbreak", d[c]) 
	return True
	


def LimitChains(ng, cg, d):
	change = True
	while change == True:
		change = False
		for c in nx.nodes(cg):
			liveins = 0
			liveouts = 0
			pos = 0
			for n in d[c]:
				if n == '1':
					continue
				Print("dbgbreak", n)
				for (u,v) in nx.edges(ng,n):
					if ng.node[n]['cid'] != ng.node[v]['cid']:
						liveouts += 1
						Print("dbgbreak", 'live out') 
				for nbr in nx.all_neighbors(ng, n):
					#Print("dbgbreak", "***Neighbor****")
					#Print("dbgbreak", nbr)
					if (n,nbr) in nx.edges(ng,n):
						continue
					if nbr == '1':
						continue
					if ng.node[n]['cid'] != ng.node[nbr]['cid']:
						liveins += 1
						Print("dbgbreak", '***live in***')
						#Print("dbgbreak", nbr)
				try:
					liveins += int(ng.node[n]['liveins'])
					Print("dbgbreak", 'arg in')
				except Exception, e:
					liveins += 0

				if liveins > 2:
					change = BreakChain(ng, cg, d,c,pos)
					LimitOutEdges(ng, cg, d)

				elif liveouts > 2:
					change = BreakChain(ng, cg, d,c,pos)
					LimitOutEdges(ng, cg, d)
				if change == True:
					break
				pos += 1
			if change == True:
				break
					
					

				
			
						

def SetColor(ng, n, rank):
	r = rank % 4 
	if (r == 0):
		ng.node[n]['style'] = 'filled'
		ng.node[n]['fillcolor'] = 'red'
		ng.node[n]['color'] = 'red'
	if (r == 1):
		ng.node[n]['style'] = 'filled'
		ng.node[n]['fillcolor'] = 'blue'
		ng.node[n]['color'] = 'blue'
	if (r == 2):
		ng.node[n]['style'] = 'filled'
		ng.node[n]['fillcolor'] = 'green'
		ng.node[n]['color'] = 'green'
	if (r == 3):
		ng.node[n]['style'] = 'filled'
		ng.node[n]['fillcolor'] = 'yellow'
		ng.node[n]['color'] = 'yellow'

def SetEdgeColor(ng, u, v, rank):
	r = rank % 4 
	ng[u][v]['penwidth']='5'

	try:
		if (r == 0):
			ng[u][v]['color'] = 'red'
		if (r == 1):
			ng[u][v]['color'] = 'blue'
		if (r == 2):
			ng[u][v]['color'] = 'green'
		if (r == 3):
			ng[u][v]['color'] = 'yellow'
	except Exception, e:
		print 'edge does not exist'
	
	
def ColorGraph(ng, cg, d):

	for n in nx.nodes(ng):
		ng.node[n]['fillcolor'] = ''
		
	for (u,v) in nx.edges(ng):
		ng[u][v]['penwidth'] = ''
		ng[u][v]['cid'] = ''

	rank = 0
	for c in nx.nodes(cg):
		print len(d[c])
		for n in d[c]:
			SetColor(ng, n, rank)
		Print("dbgcolor", d[c])
		for x in range(0, len(d[c]) - 1):
			Print("dbgcolor", x)
			ng[d[c][x]][d[c][x+1]]['cid'] = c
			SetEdgeColor(ng, d[c][x], d[c][x+1], rank)
		rank += 1


def MaxNodeId(ng):
	mx = 0
	for n in nx.nodes(ng):
		try:
			mx = max(mx, int(n))
		except Exception, e:
			mx = mx
	return mx

		

def OutputNodeId(n1, n2):
	Print("dbgid", "***preserve org ids****")
	Print("dbgid", (n1,n2))
	

	try:
		return str(int(n1))
	except Exception, e:
		return str(n2)

def OutputGraph(ng, cg, d):
	g = nx.DiGraph()
	d1 = {}
	nchains = 0
	nnodes = MaxNodeId(ng) + 1
	for c in nx.nodes(cg):
		chain = str(nchains)
		for n in d[c]:
			node = OutputNodeId(n, nnodes)
			Print ("dbgid", node)

			d1[n] = node
			g.add_node(node)
			g.node[node]['ch'] = chain
			g.node[node]['opcode'] = ng.node[n]['opcode']
			g.node[node]['label'] = g.node[node]['opcode'] + '(' + node + ')'
			g.node[node]['color'] = ng.node[n]['color']

			SetColor(g, node, nchains)
			nnodes += 1
		nchains += 1

	for (u,v) in ng.edges():

		if u == '1':
			continue
		g.add_edge(d1[u],d1[v])

		if g.node[d1[u]]['ch'] == g.node[d1[v]]['ch']:
			SetEdgeColor(g, d1[u], d1[v], int(g.node[d1[u]]['ch']))


        for n in nx.nodes(g):
                if n == d1['1']:
                        continue

                g.add_edge(d1['1'], n)
                g[d1['1']][n]['style'] = 'dotted'
                continue


                if n == d1['1']:
                        continue
                x = len(nx.edges(g, n))
                cnt = 0
                for y in nx.all_neighbors(g, n):
                        cnt += 1

                if x == cnt:
                        g.add_edge(d1['1'], n)
                        g[d1['1']][n]['style'] = 'dotted'
                                
		


	return g


# def main(dotfile):
	# ng = CreateGraph(dotfile, [('style', '')]);
	# PrintGraph(ng)


	# cg = nx.DiGraph()
	# AddChainNodes(cg, ng)

	# d = CreateChainDict(cg, ng)

	# CheckInEdges(ng)

	# LimitChains(ng, cg, d)

	# ColorGraph(ng, cg, d)
	# PrintGraph(ng)


        # g = OutputGraph(ng, cg, d)
        # nx.drawing.nx_pydot.write_dot(ng,'sample.dot')
	# PrintGraph(g)

# nargs = len(sys.argv)
# if (nargs != 3):
	# print 'usage: python breakloops.py <dot file> <output file>'
	# sys.exit()

# dotfile = sys.argv[1] 
# fig = sys.argv[2]
# main(dotfile) 
