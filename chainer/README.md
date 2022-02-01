# Chainer
**Chainer** is tools which decomposed input graphs base on Dilworth decomposing algorithm.
Dilworth characterizes the width of any fninite partially ordered set in terms of a partition of the order into a minimum number of chains.
Howerver, Dilworth itself doesn't guarantee that for directed graph, chain graph doesn't have any cycle.
As a result `chainer` has an extra step which guarantees the output chain graph doesn't have any cycle.

## Quickstart

1. Install Ubuntu packages needed:

        $ sudo apt-get install python-pip python-setuptools graphviz libgraphviz-dev pkg-config

2. Clone the tools from the `chainer` GitHub repository:

        $ git clone git@github.com:amsharifian/chainer.git

3. Install `chainer`'s requirments:

        $ cd chainer
        $ pip install -r requirements.txt 

## Running the tools

For running the `chainer` you need to provide `input` and `output` file.
The `input` file format should be the same as `test/new.dot` file.

    $ python run.py test/new.dot output/chain.dot

The `output` file contains a new filed `ch` which indicates chain's ID of the nodes.
