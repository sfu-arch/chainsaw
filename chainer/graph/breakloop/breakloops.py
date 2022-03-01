

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
import re
import collections

fig = "graph.dot"
chainid = 0

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
	
	if res == 1:
		print('*****', flag, '*******')
		print(s)
	

def PrintGraph(g):
	global fig
	a = nx.nx_agraph.to_agraph(g)
	a.layout()
	a.draw(fig)

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
			Print('dbgnode', ng.nodes[n]['cid'])
			AddChainNode(cg, ng.nodes[n]['cid'], n)
		except Exception:
			ng.nodes[n]['cid'] = 'N_'+n
			AddChainNode(cg, 'N_'+n, 'N'+n)	
		PrintGraph(cg)

def CreateNodeList(ng, c):
	l = []
	Print("dbgchain", "*********")
	Print("dbgchain", c)
	for n in nx.nodes(ng):
		if ng.nodes[n]['cid'] != c:
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

	cid = 'C_' + str(chainid)
	chainid += 1

	return cid


def SetNewChain(cg, ng, d, c, pos):

	Print("dbgloops", "breaking chains")
	if pos == 0:
		cid = NewChainId()
		cg.add_node(cid)
		d[cid] = []
		for x in d[c][1:]:
			d[cid].append(x)
			ng.nodes[x]['cid'] = cid
		d[c] = d[c][0:1]

		Print("dbgloops", d[cid])
	elif pos == len(d[c]) - 1:
		cid = NewChainId()
		cg.add_node(cid)
		d[cid] = []
		d[cid].append(d[c][pos])
		ng.nodes[d[c][pos]]['cid'] = cid
		d[c] = d[c][0:pos]

		Print("dbgloops", d[cid]) 

	else:
		#cid = NewChainId()
		#cg.add_node(cid)
		#d[cid] = []
		#d[cid].append(d[c][pos])
		#ng.nodes[d[c][pos]]['cid'] = cid

		#Print("dbgloops", d[cid]) 

		cid = NewChainId()
		cg.add_node(cid)
		d[cid] = []
		for x in d[c][pos: ]:
			d[cid].append(x)
			ng.nodes[x]['cid'] = cid

		d[c] = d[c][0:pos]

		Print("dbgloops", d[cid]) 

	Print("dbgloops", d[c]) 
		
			


def IsolateSrc(cg, ng, d, u, v):
	if len(d[u]) == 1:
		return False
	pos = 0
	for x in d[u]:
		for (src, dst) in nx.edges(ng, x):
			if dst not in d[v]:
				continue

			else:
				Print("dbgloops", (src,dst))
				SetNewChain(cg, ng, d, u, pos)
				return True
		pos += 1

def IsolateDst(cg, ng, d, u, v):
	if len(d[v]) == 1:
		return False
	for x in d[u]:
		for (src,dst) in nx.edges(ng, x):
			if dst not in d[v]:
				continue
			pos = 0
			for y in d[v]:
				if y == dst:
					Print("dbgloops", (src,dst))
					SetNewChain(cg, ng, d, v, pos)
					return True
				else:
					pos += 1


def BreakLoops(ng, cg, d):
	change = True
	while change == True:
		change = False

		cg.remove_edges_from(list(nx.edges(cg)))

		for (u,v) in nx.edges(ng):
			if ng.nodes[u]['cid'] == ng.nodes[v]['cid']:
				continue

			cg.add_edge(ng.nodes[u]['cid'], ng.nodes[v]['cid'])


		for (u,v) in nx.edges(cg):
			d1 = nx.descendants(cg, u)
			d2 = nx.descendants(cg, v)

			if v in d1:
				if u in d2:

					Print("dbgloops", "offending chains and edge")
					Print("dbgloops", d[u])
					Print("dbgloops", d[v])

					if IsolateDst(cg, ng, d, u, v) == True:
						change = True
						break
					elif IsolateSrc(cg, ng, d, u, v) == True:
						change = True
						break


						

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

def SetEdgeColor(ng, u, v, rank):
	r = rank % 6 
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
		if (r == 4):
			ng[u][v]['color'] = 'orange'
		if (r == 5):
			ng[u][v]['color'] = 'violet'
	except Exception:
		print('edge does not exist')


def ColorGraph(ng, cg, d):

	for n in nx.nodes(ng):
		ng.nodes[n]['fillcolor'] = ''
		
	for (u,v) in nx.edges(ng):
		ng[u][v]['penwidth'] = ''
		ng[u][v]['color'] = ''
		ng[u][v]['cid'] = ''

	rank = 0
	for c in nx.nodes(cg):
		print(len(d[c]))
		for n in d[c]:
			SetColor(ng, n, rank)
		Print("dbgcolor", d[c])
		for x in range(0, len(d[c]) - 1):
			Print("dbgcolor", x)
			ng[d[c][x]][d[c][x+1]]['cid'] = rank
			SetEdgeColor(ng, d[c][x], d[c][x+1], rank)
		rank += 1
