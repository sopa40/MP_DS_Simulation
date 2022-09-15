# nf-wrench-sim
Wrench simulation of nextflow workflows using the k8s executor.

## How to use

### Build
Firstly, you have to install WRENCH 2.0 on your machine, please follow the guide on `https://wrench-project.org/install`
To build the project: from main directory:
```
cd build
cmake ..
make
```
The binary will then be in the build directory.

### Launch
To launch the simulation: from the build directory: 
`./nf-wrench-sim ../data/workflow.json ../data/3_nodes_cluster.xml`

Use `--wrench-full-log` option to see full logs, for more logging please refer to `https://wrench-project.org/wrench/2.0/wrench_101.html#customizing-logging`

