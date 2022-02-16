/*!
 * iOS SDK
 *
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2019 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#import "NativeRenderManager.h"
#import "HippyShadowText.h"
#import "dom/layout_node.h"

using RenderManager = hippy::RenderManager;
using DomNode = hippy::DomNode;
using DomEvent = hippy::DomEvent;
using LayoutResult = hippy::LayoutResult;
using CallFunctionCallback = hippy::CallFunctionCallback;

void NativeRenderManager::CreateRenderNode(std::vector<std::shared_ptr<DomNode>> &&nodes) {
    [uiManager_ createRenderNodes:std::move(nodes)];
}

void NativeRenderManager::UpdateRenderNode(std::vector<std::shared_ptr<DomNode>>&& nodes) {
    [uiManager_ updateRenderNodes:std::move(nodes)];
}

void NativeRenderManager::DeleteRenderNode(std::vector<std::shared_ptr<DomNode>>&& nodes) {
    [uiManager_ deleteRenderNodesIds:std::move(nodes)];
}

void NativeRenderManager::UpdateLayout(const std::vector<std::shared_ptr<DomNode>>& nodes) {
    using DomNodeUpdateInfoTuple = std::tuple<int32_t, hippy::LayoutResult, bool, std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<DomValue>>>>;
    std::vector<DomNodeUpdateInfoTuple> nodes_infos;
    nodes_infos.resize(nodes.size());
    for (auto node : nodes) {
        int32_t tag = node->GetId();
        hippy::LayoutResult layoutResult = node->GetLayoutResult();
        auto extStyle = node->GetExtStyle();
        auto it = extStyle->find("useAnimation");
        bool useAnimation = false;
        if (extStyle->end() != it) {
            auto dom_value = it->second;
            useAnimation = dom_value->ToBoolean();
        }
        DomNodeUpdateInfoTuple nodeUpdateInfo = std::make_tuple(tag, layoutResult, useAnimation, node->GetStyleMap());
        nodes_infos.push_back(nodeUpdateInfo);
    }
    [uiManager_ updateNodesLayout:nodes_infos];
}

void NativeRenderManager::MoveRenderNode(std::vector<int32_t>&& ids,
                                      int32_t pid,
                                      int32_t id) {
    [uiManager_ renderMoveViews:std::move(ids) fromContainer:pid toContainer:id];
}

void NativeRenderManager::EndBatch() {
    [uiManager_ batch];
}

void NativeRenderManager::BeforeLayout() {}

void NativeRenderManager::AfterLayout() {}

void NativeRenderManager::AddEventListener(std::weak_ptr<DomNode> dom_node, const std::string& name) {
    auto node = dom_node.lock();
    if (node) {
        int32_t tag = node->GetId();
        [uiManager_ addEventName:name forDomNodeId:tag];
    }
};

void NativeRenderManager::RemoveEventListener(std::weak_ptr<DomNode> dom_node, const std::string &name) {
    auto node = dom_node.lock();
    if (node) {
        int32_t node_id = node->GetId();
        [uiManager_ removeEventName:name forDomNodeId:node_id];
    }
}

void NativeRenderManager::CallFunction(std::weak_ptr<DomNode> dom_node, const std::string &name,
                                    const DomArgument& param,
                                    uint32_t cb) {
    std::shared_ptr<DomNode> node = dom_node.lock();
    if (node) {
        DomValue dom_value;
        param.ToObject(dom_value);
        [uiManager_ dispatchFunction:name viewName:node->GetViewName()
                             viewTag:node->GetId() params:dom_value
                            callback:node->GetCallback(name, cb)];
    }
    EndBatch();
}