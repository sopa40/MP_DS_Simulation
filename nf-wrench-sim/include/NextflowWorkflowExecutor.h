/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_ONE_TASK_AT_A_TIME_H
#define WRENCH_EXAMPLE_ONE_TASK_AT_A_TIME_H

#include <wrench-dev.h>


namespace wrench {

    class Simulation;

    /**
     *  @brief A Workflow Management System (WMS) implementation
     */
    class NextflowWorkflowExecutor : public ExecutionController {

    public:
        // Constructor
        NextflowWorkflowExecutor(std::shared_ptr<Workflow> workflow,
                                 const std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                                 const std::shared_ptr<SimpleStorageService> &storage_service,
                                 const std::string &hostname, int maxQueue);

    protected:
        // Overridden method
        void processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent>) override;
        void processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent>) override;

    private:
        // main() method of the WMS
        int main() override;

        std::shared_ptr<Workflow> workflow;
        const std::shared_ptr<BareMetalComputeService> bare_metal_compute_service;
        const std::shared_ptr<SimpleStorageService> storage_service;

        // Maximum Number of Tasks that can be submitted to the cluster at the same time
        // This is supposed to simulate the effect of the '-queue-size' parameter
        int max_queue = 12;
        int queue = 0;
    };
}// namespace wrench
#endif//WRENCH_EXAMPLE_ONE_TASK_AT_A_TIME_H
