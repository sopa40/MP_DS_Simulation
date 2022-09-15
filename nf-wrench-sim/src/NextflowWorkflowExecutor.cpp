/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** A Workflow Management System (WMS) implementation that operates as follows:
 **  - While the workflow is not done, repeat:
 **    - Pick a ready task if any
 **    - Submit it to the first available bare-metal compute service as a job in a way that
 **       - Uses 10 cores
 **       - Reads the input file from the StorageService
 **       - Writes the output file from the StorageService
 **/

#include <iostream>

#include "NextflowWorkflowExecutor.h"
#include "boost/lexical_cast.hpp"

WRENCH_LOG_CATEGORY(custom_wms, "Log category for NextflowWorkflowExecutor");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param workflow: the workflow to execute
     * @param batch_compute_service: a bare-metal compute service available to run tasks
     * @param storage_service: a storage service available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    NextflowWorkflowExecutor::NextflowWorkflowExecutor(std::shared_ptr<Workflow> workflow,
                                                       const std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                                                       const std::shared_ptr<SimpleStorageService> &storage_service,
                                                       const std::string &hostname, int maxQueue) : ExecutionController(hostname, "one-task-on-one-node"),
                                                                                      workflow(workflow), bare_metal_compute_service(bare_metal_compute_service), storage_service(storage_service),
                                                                                      max_queue(maxQueue) {}

    /**
     * @brief main method of the NextflowWorkflowExecutor daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int NextflowWorkflowExecutor::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* While the workflow isn't done, repeat the main loop */
        while (not this->workflow->isDone()) {

            while (!this->workflow->getReadyTasks().empty() && queue < max_queue) {
                WRENCH_INFO("Current Queue: %d", queue);
                auto task = this->workflow->getReadyTasks()[0];

                std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;
                for (const auto &file: task->getInputFiles()) {
                    file_locations[file] = FileLocation::LOCATION(storage_service);
                }
                for (const auto &file: task->getOutputFiles()) {
                    file_locations[file] = FileLocation::LOCATION(storage_service);
                }
                auto standard_job = job_manager->createStandardJob(task, file_locations);

                job_manager->submitJob(standard_job, bare_metal_compute_service);
                auto host = job_manager->getNumHosts();
		 std::cerr << bare_metal_compute_service << endl;
 		 std::cerr << host << endl;
                queue++;
            }
            /* Wait for a workflow execution event and process it. In this case we know that
             * the event will be a StandardJobCompletionEvent, which is processed by the method
             * processEventStandardJobCompletion() that this class overrides. */
            this->waitForAndProcessNextEvent();
        }

        WRENCH_INFO("Workflow execution complete");
        return 0;
    }

    /**
     * @brief Process a standard job completion event
     *
     * @param event: the event
     */
    void NextflowWorkflowExecutor::processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's first (and in our case only) task */
        auto task = job->getTasks().at(0);
        WRENCH_INFO("Notified that a standard job has completed task %s", task->getID().c_str());
        queue--;
    }

    /**
     * @brief Process a standard job failure event
     *
     * @param event: the event
     */
    void NextflowWorkflowExecutor::processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's first (and in our case only) task */
        auto task = job->getTasks().at(0);
        /* Print some error message */
        WRENCH_INFO("Notified that a standard job has failed for task %s with error %s",
                    task->getID().c_str(),
                    event->failure_cause->toString().c_str());
        throw std::runtime_error("ABORTING DUE TO JOB FAILURE");
    }



//    StandardJob createJob(const shared_ptr<JobManager>& jobManager) {
//        auto job = jobManager->createCompoundJob("Job Name");
//
//
//
//
//        return (StandardJob) job;
//
//    }


}// namespace wrench
