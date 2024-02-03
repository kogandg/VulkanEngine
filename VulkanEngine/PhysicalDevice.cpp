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