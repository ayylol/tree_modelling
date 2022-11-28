#pragma once

#include <vector>
#include <memory>

#include <glm/glm.hpp>

class Skeleton{
    public:
       Skeleton(const char* filepath); 

    private:
        struct Node{
            std::shared_ptr<Node> parent; 
            std::vector<std::shared_ptr<Node>>children;
            // Defined if parent is null, otherwise equals parent->end
            glm::vec3 start; 
            // Defined
            glm::vec3 end;
        }; 
};
