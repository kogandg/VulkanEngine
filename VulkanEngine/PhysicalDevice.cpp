#include "PhysicalDevice.h"

void PhysicalDevice::Create()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(Instance::GetInstance(), &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(Instance::GetInstance(), &deviceCount, devices.data());


	for (const auto& currVkDevice : devices)
	{
		PhysicalDevice currDevice;
		currDevice.vkDevice = currVkDevice;

		//get available extensions
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(currVkDevice, nullptr, &extensionCount, nullptr);
		currDevice.extensions.resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(currVkDevice, nullptr, &extensionCount, currDevice.extensions.data());

		//get available families
		uint32_t familyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(currVkDevice, &familyCount, nullptr);
		currDevice.families.resize(familyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(currVkDevice, &familyCount, currDevice.families.data());

		//get family for each queue
		for (int i = 0; i < currDevice.families.size(); i++)
		{
			const auto& family = currDevice.families[i];

			if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				currDevice.graphicsFamily = i;
			}

			VkBool32 present = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(currVkDevice, i, Instance::GetSurface(), &present);
			if (present)
			{
				currDevice.presentFamily = i;
			}

			if (currDevice.presentFamily != -1 && currDevice.graphicsFamily != -1)
			{
				break;
			}
		}

		//max samples
		vkGetPhysicalDeviceProperties(currVkDevice, &currDevice.properties);
		vkGetPhysicalDeviceMemoryProperties(currVkDevice, &currDevice.memoryProperties);

		VkSampleCountFlags counts = currDevice.properties.limits.framebufferColorSampleCounts;
		counts &= currDevice.properties.limits.framebufferDepthSampleCounts;

		currDevice.maxSamples = VK_SAMPLE_COUNT_1_BIT;
		if (counts & VK_SAMPLE_COUNT_2_BIT) { currDevice.maxSamples = VK_SAMPLE_COUNT_2_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { currDevice.maxSamples = VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { currDevice.maxSamples = VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { currDevice.maxSamples = VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { currDevice.maxSamples = VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_64_BIT) { currDevice.maxSamples = VK_SAMPLE_COUNT_64_BIT; }
		

		vkGetPhysicalDeviceFeatures(currVkDevice, &currDevice.features);

		//is device suitable
		bool suitable = true;

		std::set<std::string> requiredExt(requiredExtensions.begin(), requiredExtensions.end());
		for (const auto& extension : currDevice.extensions)
		{
			requiredExt.erase(extension.extensionName);
		}
		suitable &= requiredExt.empty();

		suitable &= currDevice.graphicsFamily != -1;
		suitable &= currDevice.presentFamily != -1;

		currDevice.suitable = suitable;
		allDevices.emplace_back(currDevice);
	}

	UpdateDevice();
	OnSurfaceUpdate();

	dirty = false;
}

void PhysicalDevice::Destroy()
{
	allDevices.clear();
	device = nullptr;
}

void PhysicalDevice::OnSurfaceUpdate()
{
	auto surface = Instance::GetSurface();

	for (auto& currDevice : allDevices)
	{
		auto vkDevice = currDevice.vkDevice;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkDevice, surface, &currDevice.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkDevice, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			currDevice.surfaceFormats.clear();
			currDevice.surfaceFormats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(vkDevice, surface, &formatCount, currDevice.surfaceFormats.data());
		}

		uint32_t modeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(vkDevice, surface, &modeCount, nullptr);
		if (modeCount != 0) 
		{
			currDevice.presentModes.clear();
			currDevice.presentModes.resize(modeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(vkDevice, surface, &modeCount, currDevice.presentModes.data());
		}

		// set this device as not suitable if no surface formats or present modes available
		currDevice.suitable &= (modeCount > 0);
		currDevice.suitable &= (formatCount > 0);
	}

	if (!device->suitable) 
	{
		std::cout << "selected device invalidated after surface update. Trying to find a suitable device" << std::endl;
		UpdateDevice();
	}
}

void PhysicalDevice::UpdateDevice()
{
	if (index == -1 || !allDevices[index].suitable)
	{
		for (int i = 0; i < allDevices.size(); i++)
		{
			if (allDevices[i].suitable)
			{
				index = i;
			}
		}
	}
	device = &(allDevices[index]);

	if (index == -1 || !allDevices[index].suitable)
	{
		throw std::runtime_error("can't find suitable device");
	}
}

uint32_t PhysicalDevice::FindMemoryType(uint32_t type, VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < device->memoryProperties.memoryTypeCount; i++) 
	{
		if (type & (1 << i)) 
		{
			if ((device->memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) 
			{
				return i;
			}
		}
	}

	throw std::runtime_error("failed to find suitable memory type");
}

bool PhysicalDevice::SupportFormat(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(PhysicalDevice::GetVkPhysicalDevice(), format, &props);

	if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) 
	{
		return true;
	}
	else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) 
	{
		return true;
	}

	return false;
}

void PhysicalDevice::OnImgui()
{
    const auto totalSpace = ImGui::GetContentRegionAvail();
    const float totalWidth = totalSpace.x;

    if (ImGui::CollapsingHeader("Physical Device")) {
        for (size_t i = 0; i < allDevices.size(); i++) {
            auto d = allDevices[i];
            ImGui::PushID(d.properties.deviceID);
            if (ImGui::RadioButton("", i == index) && i != index) 
            {
                index = i;
                dirty = true;
            }
            ImGui::PopID();
            ImGui::SameLine();
            if (ImGui::TreeNode(d.properties.deviceName)) 
            {
                ImGui::Dummy(ImVec2(5.0f, .0f));
                ImGui::SameLine();
                ImGui::Text("API Version: %d", d.properties.apiVersion);
                ImGui::Dummy(ImVec2(5.0f, .0f));
                ImGui::SameLine();
                ImGui::Text("Driver Version: %d", d.properties.driverVersion);
                ImGui::Dummy(ImVec2(5.0f, .0f));
                ImGui::SameLine();
                ImGui::Text("Max Samples: %d", d.maxSamples);
                ImGui::Dummy(ImVec2(5.0f, .0f));
                ImGui::SameLine();
                auto minExt = d.capabilities.minImageExtent;
                ImGui::Text("Min extent: {%d, %d}", minExt.width, minExt.height);
                ImGui::Dummy(ImVec2(5.0f, .0f));
                ImGui::SameLine();
                auto maxExt = d.capabilities.maxImageExtent;
                ImGui::Text("Max extent: {%d, %d}", maxExt.width, maxExt.height);
                ImGui::Dummy(ImVec2(5.0f, .0f));
                ImGui::SameLine();
                ImGui::Text("Min image count: %d", d.capabilities.minImageCount);
                ImGui::Dummy(ImVec2(5.0f, .0f));
                ImGui::SameLine();
                ImGui::Text("Max image count: %d", d.capabilities.maxImageCount);
                ImGui::Dummy(ImVec2(5.0f, .0f));
                ImGui::SameLine();
                ImGui::Text("Graphics Family: %d", d.graphicsFamily);
                ImGui::Dummy(ImVec2(5.0f, .0f));
                ImGui::SameLine();
                ImGui::Text("Present Family: %d", d.presentFamily);
                ImGui::Separator();
                if (ImGui::TreeNode("Extensions")) 
                {
                    ImGuiTableFlags flags = ImGuiTableFlags_ScrollY;
                    flags |= ImGuiTableFlags_RowBg;
                    flags |= ImGuiTableFlags_BordersOuter;
                    flags |= ImGuiTableFlags_BordersV;
                    flags |= ImGuiTableFlags_Resizable;
                    ImVec2 maxSize = ImVec2(0.0f, 200.0f);
                    if (ImGui::BeginTable("extTable", 2, flags, maxSize)) 
                    {
                        ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
                        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
                        ImGui::TableSetupColumn("Spec Version", ImGuiTableColumnFlags_None);
                        ImGui::TableHeadersRow();
                        for (auto& ext : d.extensions) 
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%s", ext.extensionName);
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", ext.specVersion);
                        }
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }
                ImGui::Separator();
                if (ImGui::TreeNode("Families")) 
                {
                    ImGuiTableFlags flags = ImGuiTableFlags_RowBg;
                    // flags |= ImGuiTableFlags_ScrollX;
                    flags |= ImGuiTableFlags_BordersOuter;
                    flags |= ImGuiTableFlags_BordersV;
                    if (ImGui::BeginTable("famTable", 8, flags)) 
                    {
                        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_None);
                        ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_None);
                        ImGui::TableSetupColumn("Graphics", ImGuiTableColumnFlags_None);
                        ImGui::TableSetupColumn("Compute", ImGuiTableColumnFlags_None);
                        ImGui::TableSetupColumn("Transfer", ImGuiTableColumnFlags_None);
                        ImGui::TableSetupColumn("Sparse", ImGuiTableColumnFlags_None);
                        ImGui::TableSetupColumn("Protected", ImGuiTableColumnFlags_None);
                        ImGui::TableSetupColumn("Min Granularity", ImGuiTableColumnFlags_None);
                        ImGui::TableHeadersRow();
                        for (size_t j = 0; j < d.families.size(); j++) 
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%zu", j);
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", d.families[j].queueCount);
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%c", (d.families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 'X' : ' ');
                            ImGui::TableSetColumnIndex(3);
                            ImGui::Text("%c", (d.families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) ? 'X' : ' ');
                            ImGui::TableSetColumnIndex(4);
                            ImGui::Text("%c", (d.families[j].queueFlags & VK_QUEUE_TRANSFER_BIT) ? 'X' : ' ');
                            ImGui::TableSetColumnIndex(5);
                            ImGui::Text("%c", (d.families[j].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? 'X' : ' ');
                            ImGui::TableSetColumnIndex(6);
                            ImGui::Text("%c", (d.families[j].queueFlags & VK_QUEUE_PROTECTED_BIT) ? 'X' : ' ');
                            ImGui::TableSetColumnIndex(7);
                            auto min = d.families[j].minImageTransferGranularity;
                            ImGui::Text("%d, %d, %d", min.width, min.height, min.depth);
                        }
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }
                ImGui::Separator();
                if (ImGui::TreeNode("Present Modes")) 
                {
                    ImGuiTableFlags flags = ImGuiTableFlags_RowBg;
                    // flags |= ImGuiTableFlags_ScrollX;
                    flags |= ImGuiTableFlags_BordersOuter;
                    flags |= ImGuiTableFlags_BordersV;
                    if (ImGui::BeginTable("presentModes", 1, flags)) 
                    {
                        for (const auto& p : d.presentModes) 
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            if (p == VK_PRESENT_MODE_FIFO_KHR) 
                            {
                                ImGui::Text("VK_PRESENT_MODE_FIFO_KHR");
                            }
                            else if (p == VK_PRESENT_MODE_FIFO_RELAXED_KHR) 
                            {
                                ImGui::Text("VK_PRESENT_MODE_FIFO_RELAXED_KHR");
                            }
                            else if (p == VK_PRESENT_MODE_IMMEDIATE_KHR) 
                            {
                                ImGui::Text("VK_PRESENT_MODE_IMMEDIATE_KHR");
                            }
                            else if (p == VK_PRESENT_MODE_MAILBOX_KHR) 
                            {
                                ImGui::Text("VK_PRESENT_MODE_MAILBOX_KHR");
                            }
                            else if (p == VK_PRESENT_MODE_MAX_ENUM_KHR) 
                            {
                                ImGui::Text("VK_PRESENT_MODE_MAX_ENUM_KHR");
                            }
                            else if (p == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR) 
                            {
                                ImGui::Text("VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR");
                            }
                            else if (p == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR) 
                            {
                                ImGui::Text("VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR");
                            }
                        }
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
    }
}
