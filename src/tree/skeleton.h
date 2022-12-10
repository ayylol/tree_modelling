#pragma once

#include <vector>
#include <stack>
#include <array>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <stdexcept>
#include <regex>

#include <glm/glm.hpp>

//#include "util/fileContents.h"
#include "rendering/mesh.h"

#include "util/color.h"

glm::vec3 random_color();

class Skeleton{
    public:
       Skeleton(const char* filename); 

       Mesh get_mesh();

       std::vector<glm::vec3> get_strand(size_t index) const;
       size_t leafs_size() const;

    private:
        struct Node{
            glm::vec3 position; 
            // Relation to other nodes
            std::shared_ptr<Node> parent; // If no parent then this node is root
            std::vector<std::shared_ptr<Node>>children;
        }; 
        std::shared_ptr<Node> root;
        std::vector<std::shared_ptr<Node>> leafs;
};
