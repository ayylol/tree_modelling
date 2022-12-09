#pragma once

#include <vector>
#include <stack>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <stdexcept>
#include <regex>

#include <glm/glm.hpp>

#include "util/fileContents.h"

class Skeleton{
    public:
       Skeleton(const char* filename); 
       void shitpiss();

    private:
        struct Node{
            // Defined if parent is null, otherwise equals parent->end
            glm::vec3 start; 
            // Defined
            glm::vec3 end;
            // Relation to other nodes
            std::shared_ptr<Node> parent; 
            std::vector<std::shared_ptr<Node>>children;
        }; 
        std::shared_ptr<Node> root;
        std::vector<std::shared_ptr<Node>> leafs;
};
