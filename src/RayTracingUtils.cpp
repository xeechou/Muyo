#include "RayTracingUtils.h"

#include <functional>
#include <algorithm>
#include <string>
#include <vulkan/vulkan_core.h>

#include "Geometry.h"
#include "MeshVertex.h"
#include "Scene.h"
#include "VkRenderDevice.h"
#include "RenderResourceManager.h"
#include "Debug.h"

RTBuilder::RTBuilder()
{
    // Get function pointers from Vulkan library
    vkCreateAccelerationStructureKHR =
        (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(
            GetRenderDevice()->GetInstance(),
            "vkCreateAccelerationStructureKHR");

    vkDestroyAccelerationStructureKHR =
        (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(
            GetRenderDevice()->GetInstance(),
            "vkDestroyAccelerationStructureKHR");

    vkCmdBuildAccelerationStructuresKHR =
        (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetInstanceProcAddr(
            GetRenderDevice()->GetInstance(),
            "vkCmdBuildAccelerationStructuresKHR");

    vkCmdWriteAccelerationStructuresPropertiesKHR =
        (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)
            vkGetInstanceProcAddr(
                GetRenderDevice()->GetInstance(),
                "vkCmdWriteAccelerationStructuresPropertiesKHR");

    vkGetAccelerationStructureBuildSizesKHR =
        (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetInstanceProcAddr(
            GetRenderDevice()->GetInstance(),
            "vkGetAccelerationStructureBuildSizesKHR");

    vkCmdCopyAccelerationStructureKHR =
        (PFN_vkCmdCopyAccelerationStructureKHR)vkGetInstanceProcAddr(
            GetRenderDevice()->GetInstance(),
            "vkCmdCopyAccelerationStructureKHR");
}

std::vector<BLASInput> ConstructBLASInputs(const DrawLists& dls)
{
    std::vector<BLASInput> BLASs;
    // Just gather opaque nodes for now
    for (const SceneNode* pSceneNode : dls.m_aDrawLists[DrawLists::DL_OPAQUE])
    {

        const GeometrySceneNode *pGeometryNode = dynamic_cast<const GeometrySceneNode *>(pSceneNode);
        if (pGeometryNode)
        {
            BLASInput blasInput;
            for (const auto &primitive : pGeometryNode->GetGeometry()->getPrimitives())
            {
                VkAccelerationStructureGeometryTrianglesDataKHR triangles = {};
                triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
                // Vertex data
                triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
                triangles.vertexData.deviceAddress = GetRenderDevice()->GetBufferDeviceAddress(primitive->getVertexDeviceBuffer());
                // Index data
                triangles.vertexStride = sizeof(Vertex);
                triangles.indexType = VK_INDEX_TYPE_UINT32;
                triangles.indexData.deviceAddress = GetRenderDevice()->GetBufferDeviceAddress(primitive->getIndexDeviceBuffer());
                // misc
                triangles.transformData = {};
                triangles.maxVertex = primitive->getVertexCount();

                VkAccelerationStructureGeometryKHR geometry = {};
                geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                geometry.pNext = nullptr;
                geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                geometry.geometry.triangles = triangles;

                VkAccelerationStructureBuildRangeInfoKHR range = {};
                range.firstVertex = 0;
                range.primitiveCount = primitive->getIndexCount() / 3;
                range.primitiveOffset = 0;
                range.transformOffset = 0;
                blasInput.m_vGeometries.emplace_back(geometry);
                blasInput.m_vRangeInfo.emplace_back(range);
            }

            if (blasInput.m_vGeometries.size() > 0)
            {
                BLASs.push_back(blasInput);
            }
        }
    }

    return BLASs;
}

void RTBuilder::BuildBLAS(const std::vector<BLASInput> &vBLASInputs, VkBuildAccelerationStructureFlagsKHR flags)
{
    std::vector<BLASInput> m_blas = vBLASInputs;
    uint32_t nNumBlas = (uint32_t)m_blas.size();
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos(nNumBlas);
    std::vector<VkDeviceSize> vOriginalSizes(nNumBlas);
    for (size_t idx = 0; idx < nNumBlas; idx++)
    {
        buildInfos[idx].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfos[idx].flags = flags;
        buildInfos[idx].geometryCount = (uint32_t)m_blas[idx].m_vRangeInfo.size();
        buildInfos[idx].pGeometries = m_blas[idx].m_vGeometries.data();
        buildInfos[idx].mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfos[idx].type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfos[idx].srcAccelerationStructure = VK_NULL_HANDLE;
    }
    VkDeviceSize nMaxScratchSize = 0;
    for (size_t idx = 0; idx < nNumBlas; idx++)
    {
        // Query both the size of the finished acceleration structure and the  amount of scratch memory
        // needed (both written to sizeInfo). The `vkGetAccelerationStructureBuildSizesKHR` function
        // computes the worst case memory requirements based on the user-reported max number of
        // primitives. Later, compaction can fix this potential inefficiency.
        const std::string sAccBufName = "acBuf_" + std::to_string(idx);
        const std::string sAccStructName = "acStruct_" + std::to_string(idx);
        std::vector<uint32_t> maxPrimCount(m_blas[idx].m_vRangeInfo.size());
        for (size_t tt = 0; tt < m_blas[idx].m_vRangeInfo.size(); tt++)
        {
            maxPrimCount[tt] = m_blas[idx].m_vRangeInfo[tt].primitiveCount;  // Number of primitives/triangles
        }
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {};
        sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        vkGetAccelerationStructureBuildSizesKHR(
            GetRenderDevice()->GetDevice(),
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfos[idx],
            maxPrimCount.data(), &sizeInfo);

        // Create Acceleration strucutre
        VkAccelerationStructureCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        createInfo.size = sizeInfo.accelerationStructureSize;
        // Allocate acceleration structure buffer 
        AccelerationStructureBuffer* bufAcc = GetRenderResourceManager()->GetAccelerationStructureBuffer(sAccBufName, createInfo.size);
        createInfo.buffer = bufAcc->buffer();

        vkCreateAccelerationStructureKHR(GetRenderDevice()->GetDevice(),
                                         &createInfo, nullptr,
                                         &m_blas[idx].m_ac);
        // Assign debug name to the acceleration structure
        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(m_blas[idx].m_ac), VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, sAccStructName.c_str());
        buildInfos[idx].dstAccelerationStructure = m_blas[idx].m_ac;

        m_blas[idx].m_flags = flags;
        nMaxScratchSize = std::max(nMaxScratchSize, sizeInfo.buildScratchSize);
        vOriginalSizes[idx] = sizeInfo.accelerationStructureSize;
    }

    // Allocate scratch size to build the structure
    AccelerationStructureBuffer scratchBuffer(nMaxScratchSize);
    VkDeviceAddress scratchAddress = GetRenderDevice()->GetBufferDeviceAddress(scratchBuffer.buffer());

    // Is compaction requested?
    bool bDoCompaction = (flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR)
                        == VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;

    // Allocate a query pool for storing the needed size for every BLAS compaction.
    VkQueryPoolCreateInfo qpci = {};
    qpci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    qpci.queryCount = nNumBlas;
    qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    VkQueryPool queryPool;
    vkCreateQueryPool(GetRenderDevice()->GetDevice(), &qpci, nullptr,
                      &queryPool);

    std::vector<VkCommandBuffer> cmdBuffers;
    for (size_t idx = 0; idx < nNumBlas; idx++)
    {
        const BLASInput &blas = m_blas[idx];
        VkCommandBuffer cmdBuf = GetRenderDevice()->AllocateImmediateCommandBuffer();

        VkCommandBufferBeginInfo cmdBeginInfo = {};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        setDebugUtilsObjectName(reinterpret_cast<uint64_t>(cmdBuf), VK_OBJECT_TYPE_COMMAND_BUFFER, (std::string("[CB] build blas" + std::to_string(idx)).c_str()));

        buildInfos[idx].scratchData.deviceAddress = scratchAddress;
        std::vector<const VkAccelerationStructureBuildRangeInfoKHR *> vpBuildOffsets(blas.m_vRangeInfo.size());
        for (size_t infoIdx = 0; infoIdx < vpBuildOffsets.size(); infoIdx++)
        {
            vpBuildOffsets[infoIdx] = &blas.m_vRangeInfo[infoIdx];
        }

        vkBeginCommandBuffer(cmdBuf, &cmdBeginInfo);
        {
            SCOPED_MARKER(cmdBuf, "[RT] Build Acc Struct");
            vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfos[idx],
                                                vpBuildOffsets.data());

            VkMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            barrier.srcAccessMask =
                VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
            barrier.dstAccessMask =
                VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            vkCmdPipelineBarrier(
                cmdBuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1,
                &barrier, 0, nullptr, 0, nullptr);

            // Write compacted size to query number idx.
            if (bDoCompaction)
            {
                vkCmdWriteAccelerationStructuresPropertiesKHR(
                    cmdBuf, 1, &blas.m_ac,
                    VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
                    queryPool, idx);
            }
        }
        vkEndCommandBuffer(cmdBuf);
        cmdBuffers.push_back(cmdBuf);
    }

    GetRenderDevice()->SubmitCommandBuffersAndWait(cmdBuffers);
    cmdBuffers.clear();

    if (bDoCompaction)
    {
        std::vector<VkDeviceSize> vCompactSizes(nNumBlas);
        vkGetQueryPoolResults(GetRenderDevice()->GetDevice(), queryPool, 0,
                              (uint32_t)vCompactSizes.size(),
                              vCompactSizes.size() * sizeof(VkDeviceSize),
                              vCompactSizes.data(), sizeof(VkDeviceSize),
                              VK_QUERY_RESULT_WAIT_BIT);

        uint32_t nTotalOriginalSizes = 0;
        uint32_t nTotalCompactSizes = 0;
        std::vector<VkAccelerationStructureKHR> vAcToSwap;
        std::vector<AccelerationStructureBuffer*> vBufToSwap;
        for (uint32_t idx = 0; idx < nNumBlas; idx++)
        {

            nTotalOriginalSizes += vOriginalSizes[idx];
            nTotalCompactSizes += vCompactSizes[idx];

            AccelerationStructureBuffer* pBuf = new AccelerationStructureBuffer(vCompactSizes[idx]);
            VkAccelerationStructureKHR acc = VK_NULL_HANDLE;

            // Create compact acceleration structure
            VkAccelerationStructureCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            createInfo.size = vCompactSizes[idx];
            createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            createInfo.buffer = pBuf->buffer();
            vkCreateAccelerationStructureKHR(GetRenderDevice()->GetDevice(),
                                             &createInfo, nullptr,
                                             &acc);

            VkCommandBuffer cmdBuf = GetRenderDevice()->AllocateImmediateCommandBuffer();
            cmdBuffers.push_back(cmdBuf);
            VkCommandBufferBeginInfo cmdBeginInfo = {};
            cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            cmdBeginInfo.pInheritanceInfo = nullptr;
            setDebugUtilsObjectName(reinterpret_cast<uint64_t>(cmdBuf), VK_OBJECT_TYPE_COMMAND_BUFFER, (std::string("[CB] BLAS Compaction " + std::to_string(idx)).c_str()));
            vkBeginCommandBuffer(cmdBuf, &cmdBeginInfo);
            {
                // Copy old acc structure to new acc structure with compaction
                SCOPED_MARKER(cmdBuf, "Acceleration structure compaction" + std::to_string(idx));
                VkCopyAccelerationStructureInfoKHR copyInfo = {};
                copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
                copyInfo.src = m_blas[idx].m_ac;
                copyInfo.dst = acc;
                copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
                vkCmdCopyAccelerationStructureKHR(cmdBuf, &copyInfo);
            }
            vkEndCommandBuffer(cmdBuf);

            // Prepare resources to swap
            vAcToSwap.push_back(acc);
            vBufToSwap.push_back(pBuf);
        }
        GetRenderDevice()->SubmitCommandBuffersAndWait(cmdBuffers);
        cmdBuffers.clear();

        // Swap old blas structure with compacted blas structure

        for (uint32_t idx = 0; idx < nNumBlas; idx++)
        {
            
            vkDestroyAccelerationStructureKHR(GetRenderDevice()->GetDevice(), m_blas[idx].m_ac, nullptr);
            // Make sure they are the same names used above because we want to swap exactly the same buffer
            const std::string sAccBufName = "acBuf_" + std::to_string(idx);
            const std::string sAccStructName = "acStruct_" + std::to_string(idx);
            m_blas[idx].m_ac = vAcToSwap[idx];
            GetRenderResourceManager()->AssignResource(sAccBufName, vBufToSwap[idx]);
            setDebugUtilsObjectName(reinterpret_cast<uint64_t>(m_blas[idx].m_ac), VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, sAccStructName.c_str());
        }
    }
    vkDestroyQueryPool(GetRenderDevice()->GetDevice(), queryPool, nullptr);

}


void RTBuilder::BuildTLAS(const std::vector<Instance> &instances, VkBuildAccelerationStructureFlagsKHR flags, bool bUpdate)
{

}

void RTBuilder::Cleanup(std::vector<BLASInput> &vBLASInputs)
{
    for (auto& blasInput : vBLASInputs)
    {
        vkDestroyAccelerationStructureKHR(GetRenderDevice()->GetDevice(), blasInput.m_ac, nullptr);
    }
}
