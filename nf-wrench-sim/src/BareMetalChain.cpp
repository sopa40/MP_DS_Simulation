/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This simulator simulates the execution of a chain workflow, that is, of a workflow
 ** in which each task has at most a single parent and at most a single child:
 **
 **   File #0 -> Task #0 -> File #1 -> Task #1 -> File #2 -> .... -> Task #n-1 -> File #n
 **
 ** The compute platform comprises two hosts, WMSHost and ComputeHost. On WMSHost runs a simple storage
 ** service and a WMS (defined in class NextflowWorkflowExecutor). On ComputeHost runs a bare metal
 ** compute service, that has access to the 10 cores of that host. Once the simulation is done,
 ** the completion time of each workflow task is printed.
 **
 ** Example invocation of the simulator for a 10-task workflow, with no logging:
 **    ./wrench-example-bare-metal-chain 10 ./two_hosts.xml
 **
 ** Example invocation of the simulator for a 10-task workflow, with only WMS logging:
 **    ./wrench-example-bare-metal-chain 10 ./two_hosts.xml --log=custom_wms.threshold=info
 **
 ** Example invocation of the simulator with full logging:
 **    ./wrench-example-bare-metal-chain 5 ./two_hosts.xml --wrench-full-log
 **/


#include <iostream>
#include <fstream>
#include <wrench.h>
#include <fstream>
#include <nlohmann/json.hpp>

#include "NextflowWorkflowExecutor.h"// WMS implementation
#include "JSONWorkflow.h"// WMS implementation


/**
 * @brief The Simulator's main function
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 on success, non-zero otherwise
 */
int main(int argc, char **argv) {


    /*
     * Create a WRENCH simulation object
     */
    auto simulation = wrench::Simulation::createSimulation();

    /* Initialize the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments. */
    simulation->init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <Workflow Trace> <xml platform file> [--log=custom_wms.threshold=info]"
                  << std::endl;
        exit(1);
    }

    nlohmann::json json_file;
    std::ifstream(argv[1]) >> json_file;
    auto workflowDescription = json_file.get<JSONWorkflow::JSONWorkflow>();

    /* Reading and parsing the platform description file, written in XML following the SimGrid-defined DTD,
     * to instantiate the simulated platform */
    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation->instantiatePlatform(argv[2]);


    /* Declare a workflow */
    auto workflow = wrench::Workflow::createWorkflow();


    std::map<std::string, double> files;
    for (const auto &task: workflowDescription.tasks) {
        for (const auto &input_file: task.inputFiles) {
            files[input_file.path] = input_file.size;
        }

        for (const auto &output_file: task.outputFiles) {
            files[output_file.path] = output_file.size;
        }
    }


    for (const auto &kv: files) {
        workflow->addFile(kv.first, kv.second);
    }

    /* Add workflow tasks */
    for (const auto &task: workflowDescription.tasks) {
        // Number of Instructions divided by the average cpu usage:
        // This is supposed to model a task that is only using 10% of its cpu as a task that has to execute ten times more instructions
        // The Number needs to be normalized, because tasks that use multiple CPUS have an average cpu usage higher than 100%
        // Examples
        auto cpu_usage_factor = 1.0 / (task.averageCpuUsage / task.numberOfCores);
        auto wrench_task = workflow->addTask(task.id, cpu_usage_factor * task.numberOfInstructions, task.numberOfCores,
                                             task.numberOfCores, task.memoryRequirement);

        for (const auto &input_file: task.inputFiles) {
            wrench_task->addInputFile(workflow->getFileByID(input_file.path));
        }

        for (const auto &output_file: task.outputFiles) {
            wrench_task->addOutputFile(workflow->getFileByID(output_file.path));
        }
    }

    std::cerr << "Created " << workflowDescription.tasks.size() << " tasks" << endl;

    std::cerr << "Instantiating a SimpleStorageService on WMSHost..." << std::endl;
    auto storage_service = simulation->add(new wrench::SimpleStorageService(
            "WMSHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50000000"}}, {}));


    // Any Host, whose name starts with ComputeHost ís part of the Bare Metal Service
    vector<std::string> compute_hosts;
    for (const auto &host_name: simulation->getHostnameList()) {
        if (host_name.rfind("ComputeHost") == 0) {
            compute_hosts.push_back(host_name);
        }
    }

    std::cerr << "Instantiating a bare-metal compute service on ComputeHost..." << std::endl;
    auto baremetal_service = simulation->add(new wrench::BareMetalComputeService(
            "ComputeHost", compute_hosts, "", {}, {}));

    auto wms = simulation->add(
            new wrench::NextflowWorkflowExecutor(workflow, baremetal_service, storage_service, "WMSHost", 12));

    std::cerr << "Instantiating a FileRegistryService on WMSHost ..." << std::endl;
    auto file_registry_service = new wrench::FileRegistryService("WMSHost");
    simulation->add(file_registry_service);

    std::cerr << "Staging task input files..." << std::endl;
    for (auto const &f: workflow->getInputFiles()) {
        simulation->stageFile(f, storage_service);
    }

    /* Enable some output time stamps */
    simulation->getOutput().enableWorkflowTaskTimestamps(true);

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cerr << "Simulation done!" << std::endl;

    /* Simulation results can be examined via simulation->getOutput(), which provides access to traces
     * of events. In the code below, we print the  retrieve the trace of all task completion events, print how
     * many such events there are, and print some information for the first such event. */
    ofstream LogFile("log.txt");

    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> trace;
    trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    std::cerr << "Number of entries in TaskCompletion trace: " << trace.size() << std::endl;
    unsigned long num_failed_tasks = 0;
    double computation_communication_ratio_average = 0.0;
    for (const auto &item: trace) {
        auto task = item->getContent()->getTask();
        //std::cerr << "Task " + task->getID() + " completed in ";
        //std::cerr << (task->getComputationEndDate() - task->getComputationStartDate()) << " seconds" << std::endl;
        /* duplicate terminal output to the file */
        LogFile << task->getID() + " ";
        LogFile << (task->getComputationEndDate() - task->getComputationStartDate()) << std::endl;
    }

    LogFile.close();

    return 0;
}
