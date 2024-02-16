module;
#include <vulkan/vulkan.h>
#include <fmt/core.h>

export module RenderCore.PipelineBuilder;

import RenderCore.VulkanInitializers;

import <vector>;

export namespace vkUtil
{

    class PipelineBuilder {
    public:
        std::vector<VkPipelineShaderStageCreateInfo> vkShaderStages;

        VkPipelineInputAssemblyStateCreateInfo vkInputAssembly;
        VkPipelineRasterizationStateCreateInfo vkRasterizer;
        VkPipelineColorBlendAttachmentState vkColorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo vkMultisampling;
        VkPipelineLayout vkPipelineLayout;
        VkPipelineDepthStencilStateCreateInfo vkDepthStencil;
        VkPipelineRenderingCreateInfo vkRenderInfo;
        VkFormat vkColorAttachmentFormat;

        PipelineBuilder() { Clear(); }

        void Clear()
        {
            // Clear all of the structs we need back to 0 with their correct stype	

            vkInputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
            vkRasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
            vkColorBlendAttachment = {};
            vkMultisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
            vkPipelineLayout = {};
            vkDepthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
            vkRenderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
            vkShaderStages.clear();
        }

        void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
        {
            vkShaderStages.clear();

            vkShaderStages.push_back(
                vkInit::Pipeline_ShaderStage_CreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));

            vkShaderStages.push_back(
                vkInit::Pipeline_ShaderStage_CreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
        }

        void SetInputTopology(VkPrimitiveTopology topology, bool primitiveRestart = VK_FALSE)
        {
            vkInputAssembly.topology = topology;
            vkInputAssembly.primitiveRestartEnable = primitiveRestart;
        }

        void SetPolygonMode(VkPolygonMode mode, float lineWidth = 1.f)
        {
            vkRasterizer.polygonMode = mode;
            vkRasterizer.lineWidth = lineWidth;
        }
        void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
        {
            vkRasterizer.cullMode = cullMode;
            vkRasterizer.frontFace = frontFace;
        }

        void SetMultisampling_None()
        {
            vkMultisampling.sampleShadingEnable = VK_FALSE;
            // Multisampling defaulted to no multisampling (1 sample per pixel)
            vkMultisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            vkMultisampling.minSampleShading = 1.0f;
            vkMultisampling.pSampleMask = nullptr;
            // No alpha to coverage either
            vkMultisampling.alphaToCoverageEnable = VK_FALSE;
            vkMultisampling.alphaToOneEnable = VK_FALSE;
        }

        void DisableBlending()
        {
            // Default write mask
            vkColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            // No blending
            vkColorBlendAttachment.blendEnable = VK_FALSE;
        }

        void EnableBlendingAdditive()
        {
            vkColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            vkColorBlendAttachment.blendEnable = VK_TRUE;
            vkColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            vkColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
            vkColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            vkColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            vkColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            vkColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        void EnableBlendingAlphaBlend()
        {
            vkColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            vkColorBlendAttachment.blendEnable = VK_TRUE;
            vkColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            vkColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
            vkColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            vkColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            vkColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            vkColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        void SetColorAttachmentFormat(VkFormat format)
        {
            vkColorAttachmentFormat = format;
            // Connect the format to the renderInfo structure
            vkRenderInfo.colorAttachmentCount = 1;
            vkRenderInfo.pColorAttachmentFormats = &vkColorAttachmentFormat;
        }

        void SetDepthFormat(VkFormat format)
        {
            vkRenderInfo.depthAttachmentFormat = format;
        }

        void DisableDepthTest()
        {
            vkDepthStencil.depthTestEnable = VK_FALSE;
            vkDepthStencil.depthWriteEnable = VK_FALSE;
            vkDepthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
            vkDepthStencil.depthBoundsTestEnable = VK_FALSE;
            vkDepthStencil.stencilTestEnable = VK_FALSE;
            vkDepthStencil.front = {};
            vkDepthStencil.back = {};
            vkDepthStencil.minDepthBounds = 0.f;
            vkDepthStencil.maxDepthBounds = 1.f;
        }

        void EnableDepthTest(bool depthWriteEnable, VkCompareOp op)
        {
            vkDepthStencil.depthTestEnable = VK_TRUE;
            vkDepthStencil.depthWriteEnable = depthWriteEnable;
            vkDepthStencil.depthCompareOp = op;
            vkDepthStencil.depthBoundsTestEnable = VK_FALSE;
            vkDepthStencil.stencilTestEnable = VK_FALSE;
            vkDepthStencil.front = {};
            vkDepthStencil.back = {};
            vkDepthStencil.minDepthBounds = 0.f;
            vkDepthStencil.maxDepthBounds = 1.f;
        }

        VkPipeline BuildPipeline(VkDevice device)
        {
            // Make viewport state from our stored viewport and scissor.
            // At the moment we wont support multiple viewports or scissors
            VkPipelineViewportStateCreateInfo viewportState = {};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.pNext = nullptr;

            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            // Setup dummy color blending. We aren't using transparent objects yet
            // The blending is just "no blend", but we do write to the color attachment
            VkPipelineColorBlendStateCreateInfo colorBlending = {};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.pNext = nullptr;

            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &vkColorBlendAttachment;


            // Completely clear VertexInputStateCreateInfo, as we have no need for it
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

            // Build the actual pipeline
            // We now use all of the info structs we have been writing into into this one to create the pipeline
            VkGraphicsPipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
            // Connect the renderInfo to the pNext extension mechanism
            pipelineInfo.pNext = &vkRenderInfo;

            pipelineInfo.stageCount = (uint32_t)vkShaderStages.size();
            pipelineInfo.pStages = vkShaderStages.data();
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &vkInputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &vkRasterizer;
            pipelineInfo.pMultisampleState = &vkMultisampling;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDepthStencilState = &vkDepthStencil;
            pipelineInfo.layout = vkPipelineLayout;

            VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

            VkPipelineDynamicStateCreateInfo dynamicInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
            dynamicInfo.pDynamicStates = &state[0];
            dynamicInfo.dynamicStateCount = 2;

            pipelineInfo.pDynamicState = &dynamicInfo;

            // It's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
            VkPipeline newPipeline;
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                nullptr, &newPipeline)
                != VK_SUCCESS) {
                fmt::println("Failed to create graphics pipeline");
                return VK_NULL_HANDLE; // Failed to create graphics pipeline
            }
            else {
                return newPipeline;
            }
        }
    };
}