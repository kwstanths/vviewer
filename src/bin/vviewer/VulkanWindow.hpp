#ifndef __VulkanWindow_hpp__
#define __VulkanWindow_hpp__

#include <qvulkanwindow.h>

#include "VulkanRenderer.hpp"

class VulkanWindow : public QVulkanWindow {
public:
    QVulkanWindowRenderer * createRenderer() override {
        return new VulkanRenderer(this);
    }
};

#endif