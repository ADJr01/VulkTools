//
// Created by adnan on 6/30/25.
//

#include "VulkContext.h"
#include <iostream>
#include "VulkDebug.h"
#include "VulkDevice.h"
#include "VulkInfoInstance.h"
#include "../Util/diagnostic/InstanceInitializationError.h"

VulkContext::VulkContext(const VulkConf& vulk_conf)  {
        this->useValidation = vulk_conf.build_mode == BuildMode::DEV;
        this->appName = vulk_conf.app_name;
        this->engineName = vulk_conf.engine_name;
        this->displayAdapter.Window = vulk_conf.window;
        this->displayAdapter.surface = VK_NULL_HANDLE;
        this->displayAdapter.swapchain = VK_NULL_HANDLE;
        this->context.Instance = VK_NULL_HANDLE;
        this->context.Device.physicalDevice = VK_NULL_HANDLE;
        this->context.Device.logicalDevice = VK_NULL_HANDLE;
        this->extensionAdapter.validationLayers = { "VK_LAYER_KHRONOS_validation"};
        this->extensionAdapter.extensions = vulk_conf.extensions;
}



VulkContext::~VulkContext() {
    this->dropContext();
}


void VulkContext::createContext() {
    if (this->appName.empty() || this->engineName.empty() || this->extensionAdapter.extensions.empty()) throw InstanceInitializationError(VULK_INSTANCE_INITIALIZATION_ERROR("Required Info Missing to create Vulk Context"));
    auto appInfo = createAppInfo(this->appName, this->engineName);
    const VkInstanceCreateInfo instanceInfo = createInstanceInfo(appInfo, this->extensionAdapter,this->useValidation);

    if (vkCreateInstance(&instanceInfo,nullptr,&this->context.Instance)!=VK_SUCCESS) {
        throw InstanceInitializationError(VULK_INSTANCE_INITIALIZATION_ERROR("Failed to create Vulk Context"));
    };
    // ? case if validation layer is on
    if (this->useValidation ) {
        //*attach validation layer
        VulkanDebugMessenger debugMessenger;
        //!enable all serverity and types
        debugMessenger.enableAllDebugLayers();

        const auto createInfo = debugMessenger.getCreateInfo();
        if (debugMessenger.CreateDebugUtilsMessengerEXT(this->context.Instance,&createInfo,nullptr,&this->context.debugMessenger)!=VK_SUCCESS) {
            throw std::runtime_error(VULK_RUNTIME_ERROR("Failed To Setup Debug Layer"));
        }
    }

    const auto device = pickSuitablePhysicalDevice(getPhysicalDeviceList(this->context.Instance)); //* picking suitable physical  device
    if (device==VK_NULL_HANDLE) throw std::runtime_error(VULK_INSTANCE_INITIALIZATION_ERROR("Failed to create Vulk Context"));
    auto DeviceQueueFamilyList = getQueueFamilies(device); //* getting queue family list from our physical device
    auto Indices = getGraphicsQueueFamilyIndices(DeviceQueueFamilyList);//* selecting graphics queue index from queue family list
    if (!Indices.isValidGraphicsFamily()) {
        this->dropContext();
        throw std::runtime_error(VULK_RUNTIME_ERROR("Failed to pick Graphics Queue From  Context."));
    }
    //? holding reference
    this->context.Device.physicalDevice = device;
    this->context.queueFamilyIndices = Indices;
    this->context.Device.deviceFeatures = getPhysicalDeviceFeatures(this->context.Device.physicalDevice);
    //?Create Logical Device
    auto deviceQueueInfo = createDeviceQueueInfo(Indices); // * getting device queue info
    const auto logicalDeviceCreateInfo = createLogicalDeviceInfo(deviceQueueInfo,this->context.Device,this->useValidation,this->extensionAdapter.validationLayers); //* Creating logical device create info
    if (vkCreateDevice(this->context.Device.physicalDevice,&logicalDeviceCreateInfo,nullptr,&this->context.Device.logicalDevice)!=VK_SUCCESS) {
        this->dropContext();
        throw std::runtime_error(VULK_RUNTIME_ERROR("Logical Device Creation Failed"));
    }
    vkGetDeviceQueue(this->context.Device.logicalDevice,this->context.queueFamilyIndices.graphicsFamilyIndex,0,&this->queueList.graphicsQueue); // * storing reference of graphics queue create by logical device

}








void VulkContext::dropContext() {
    if (this->extensionAdapter.extensions.empty() && this->extensionAdapter.validationLayers.empty()) return;
    this->extensionAdapter.extensions.clear();
    this->extensionAdapter.validationLayers.clear();
    this->appName.clear();
    this->engineName.clear();
    if (this->useValidation) {
        VulkanDebugMessenger::DestroyDebugUtilsMessengerEXT(this->context.Instance, this->context.debugMessenger, nullptr);
        this->useValidation=false;
    }
    if (this->context.Device.logicalDevice!=VK_NULL_HANDLE)vkDestroyDevice(this->context.Device.logicalDevice,nullptr);
    if (this->context.Instance!=VK_NULL_HANDLE)vkDestroyInstance(this->context.Instance,nullptr);
    this->context.Instance=VK_NULL_HANDLE;
}
