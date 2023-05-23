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
#include <nlohmann/json.hpp>

//#include "util/fileContents.h"
#include "rendering/mesh.h"

#include "util/color.h"

glm::vec3 random_color();
class Skeleton{
    public:
        Skeleton(nlohmann::json& options); 

        Mesh<VertFlat> get_mesh();

        size_t leafs_size() const;
        size_t roots_size() const;
        std::pair<glm::vec3,glm::vec3> get_bounds() const;
        glm::vec3 get_com() const;
        glm::vec3 get_root_pos() const;
        float get_average_length() const;

        enum path_type{
          LEAF,
          ROOT
        };
        std::vector<glm::vec3> get_strand(size_t index, path_type type=LEAF) const;

    private:
        struct Node{
            glm::vec3 position; 
            std::shared_ptr<Node> parent;
            std::vector<std::shared_ptr<Node>>children;
        }; 
        std::shared_ptr<Node> shoot_root;
        std::vector<std::shared_ptr<Node>> leafs;
        std::shared_ptr<Node> root_root;
        std::vector<std::shared_ptr<Node>> root_tips;

        struct ParseInfo{
            std::pair<glm::vec3,glm::vec3> extent;
            glm::vec3 center_of_mass;
            int num_nodes;
            float total_length;
        };
        static ParseInfo parse(std::shared_ptr<Node>& root,  
            std::vector<std::shared_ptr<Node>>& leafs,
            std::string filename);

        std::pair<glm::vec3,glm::vec3> bounds;
        glm::vec3 center_of_mass;
        float average_length;
};
