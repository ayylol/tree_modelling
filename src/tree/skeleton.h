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

       Mesh<VertFlat> get_mesh();

       std::vector<glm::vec3> get_strand(size_t index) const;
       size_t leafs_size() const;
       std::pair<glm::vec3,glm::vec3> get_bounds() const;
       glm::vec3 get_com() const;
       float get_average_length() const;

    private:
        struct Node{
            glm::vec3 position; 
            std::shared_ptr<Node> parent;
            std::vector<std::shared_ptr<Node>>children;
        }; 
        std::shared_ptr<Node> root;
        std::vector<std::shared_ptr<Node>> leafs;

       std::pair<glm::vec3,glm::vec3> bounds;
       glm::vec3 center_of_mass;
       float average_length;
};
