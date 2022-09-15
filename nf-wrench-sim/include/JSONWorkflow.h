//
// Created by ls on 16.06.22.
//

#ifndef NF_WRENCH_SIM_JSONWORKFLOW_H
#define NF_WRENCH_SIM_JSONWORKFLOW_H

#include <nlohmann/json.hpp>

namespace JSONWorkflow {
    struct JSONWorkflowTaskFile {
        std::string path;
        double size;
    };

    struct JSONWorkflowTask {
        std::string name;
        std::string id;
        double numberOfInstructions;
        std::vector<JSONWorkflowTaskFile> inputFiles;
        std::vector<JSONWorkflowTaskFile> outputFiles;
        unsigned long numberOfCores;
        double memoryRequirement;
        double averageCpuUsage;
    };

    struct JSONWorkflow {
        std::vector<JSONWorkflowTask> tasks;
    };

    void from_json(const nlohmann::json &j, JSONWorkflowTaskFile &wftf);
    void from_json(const nlohmann::json &j, JSONWorkflowTask &wft);
    void from_json(const nlohmann::json &j, JSONWorkflow &wf);
}


#endif //NF_WRENCH_SIM_JSONWORKFLOW_H
