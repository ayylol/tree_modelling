#include "tree/skeleton.h"
#include <iostream>

#define PRINT_POS(pos)  \
    std::cout<<pos.x<<" "<<pos.y<<" "<<pos.z<<std::endl \

// TODO: I feel like this is messy
Skeleton::Skeleton(const char* filename){
    // Initialize file stream, and string token
    std::ifstream in(filename);
    std::string token;

    #define GET_NEXT(token) std::getline(in,token, ' ')
        
    #define PARSE_POINT(token, pos)     \
        for (int i = 0; i<3; i++){      \
            GET_NEXT(token);            \
            pos[i] = std::stof(token);  \
        }                               \
        GET_NEXT(token);                \
        if(token.find(")")==std::string::npos)    \
            throw std::invalid_argument( "Incorrect file format: position parentheses not closed" )    \
    
    // Initialize root node
    root = std::make_shared<Node>(Node{});
    // Get root start
    GET_NEXT(token);
    if (token.find("(")==std::string::npos)
        throw std::invalid_argument( "Incorrect file format: Opening parentheses for position not found" );
    PARSE_POINT(token, root->start);
    // Get root end
    GET_NEXT(token);
    if (token.find("(")==std::string::npos)
        throw std::invalid_argument( "Incorrect file format: Opening parentheses for position not found" );
    PARSE_POINT(token, root->end);

    std::stack<std::shared_ptr<Node>> last_split;
    last_split.push(root);
    std::shared_ptr<Node> last_node=root;

    // TODO CLEAN UP
    for (std::string token; GET_NEXT(token);) 
    {
        // Branch starting
        if(token.find("[")!=std::string::npos){
            std::cout<<"branch starting"<<std::endl;
        }
        // Branch ending
        if(token.find("]")!=std::string::npos){
            std::cout<<"branch ending"<<std::endl;
        }
        // Point being specified
        if(token.find("(")!=std::string::npos){
            glm::vec3 end;
            PARSE_POINT(token,end);
            std::shared_ptr<Node> next(Node{});
            //PRINT_POS(next->start);
            //next->end=end;
        }
    }
}

void Skeleton::shitpiss(){
    //std::cout<<root->start.x<<std::endl;
}
