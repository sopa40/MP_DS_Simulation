//
// Created by ls on 16.06.22.
//

#include "../include/JSONWorkflow.h"



void JSONWorkflow::from_json(const nlohmann::json &j, JSONWorkflowTaskFile &wftf) {
    j.at("path").get_to(wftf.path);
    j.at("size").get_to(wftf.size);
}
void JSONWorkflow::from_json(const nlohmann::json &j, JSONWorkflowTask &wft) {
    std::string name;
    j.at("name").get_to(wft.name);
    j.at("id").get_to(wft.id);
    j.at("numberOfInstructions").get_to(wft.numberOfInstructions);
    j.at("inputFiles").get_to(wft.inputFiles);
    j.at("outputFiles").get_to(wft.outputFiles);
    j.at("numberOfCores").get_to(wft.numberOfCores);
    j.at("memoryRequirement").get_to(wft.memoryRequirement);
    j.at("averageCpuUsage").get_to(wft.averageCpuUsage);
}

void JSONWorkflow::from_json(const nlohmann::json &j, JSONWorkflow &wf) {

    j.get_to<std::vector<JSONWorkflowTask>>(wf.tasks);
}
