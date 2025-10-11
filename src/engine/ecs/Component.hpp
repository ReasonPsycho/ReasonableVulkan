//
// Created by redkc on 10/10/2025.
//

#ifndef REASONABLEVULKAN_COMPONENT_HPP
#define REASONABLEVULKAN_COMPONENT_HPP

struct Component
{
public:
    virtual ~Component() = default;
    virtual void ImGuiComponent() = 0;
};


#endif //REASONABLEVULKAN_COMPONENT_HPP
