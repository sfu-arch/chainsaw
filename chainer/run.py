import pdb
import sys
import networkx as nx
import json
from graph.dilworth import chains as ch
from graph.limitio import limitcheck as check
from graph.breakloop import breakloops as bl


def main(dotfile):

    # Reading JSON config file
    with open ('conf/config.json') as config_file:
        config = json.load(config_file)

    # Reading dot file
    ng = ch.CreateGraph(dotfile, [('style', '')])

    # Check whether the graph has more than one BB node or not
    if ch.CheckStartNode(ng) == 1:
        sys.exit("The graph can not have more than one BB node")

    # Expanding GEP instructions
    ch.ExpandGEP(ng)

    # Removing data nodes
    ch.RemoveDataNodes(ng)

    # Check number of output edeges for each node
    check.checkEdgeOut(ng, config['node-limit'])

    # Check number of input edges for each node
    check.checkEdgeIn(ng, config['node-limit'])

    # Running Dilworth algorithm
    ch.Dilworth(ng)

    # Breaking head node
    ch.BreakHeadNode(ng)


    # Limiting chains' output and input to LIMIT value
    # from config file
    check.limitChainOut(ng, config['limit-out'])
    check.limitChainIn(ng, config['limit-in'])

    # Check the chain graph for having loops and break them
    cg = nx.DiGraph()
    bl.AddChainNodes(cg, ng)
    d = bl.CreateChainDict(cg, ng)
    bl.BreakLoops(ng, cg, d)
   

    bl.ColorGraph(ng, cg, d)
    nx.drawing.nx_pydot.write_dot(ng, 'sample.dot')


nargs = len(sys.argv)
if (nargs != 3):
        print('usage: python run.py <dot file> <output file>')
        sys.exit()


dotfile = sys.argv[1]
fig = sys.argv[2]
if __name__ == "__main__":
    main(dotfile)
