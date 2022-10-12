/**
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2022 THL A29 Limited, a Tencent company.
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

#include "renderer/tdf/viewnode/view_node.h"

#include <algorithm>
#include <utility>

#include "core/common/color.h"
#include "core/support/text/UTF.h"
#include "core/support/gesture/recognizer/tap_gesture_recognizer.h"
#include "core/support/gesture/recognizer/long_press_gesture_recognizer.h"
#include "dom/node_props.h"
#include "dom/scene.h"
#include "footstone/hippy_value.h"
#include "footstone/logging.h"
#include "renderer/tdf/viewnode/node_attributes_parser.h"
#include "renderer/tdf/viewnode/root_view_node.h"

namespace hippy {
inline namespace render {
inline namespace tdf {

using footstone::check::checked_numeric_cast;
using DomValueArrayType = footstone::value::HippyValue::DomValueArrayType;

ViewNode::ViewNode(const RenderInfo info, std::shared_ptr<tdfcore::View> view)
    : render_info_(info), attached_view_(std::move(view)), corrected_index_(info.index) {
  SetId(info.id);
}

ViewNode::DomStyleMap ViewNode::GenerateStyleInfo(const std::shared_ptr<hippy::DomNode>& dom_node) {
  DomStyleMap dom_style_map;
  auto style_map = dom_node->GetStyleMap();
  for (const auto& it : *style_map) {
    dom_style_map.insert(it);
  }
  auto ext_map = dom_node->GetExtStyle();
  for (const auto& it : *ext_map) {
    dom_style_map.insert(it);
  }
  return dom_style_map;
}

void ViewNode::OnCreate() {
  auto parent = GetRootNode()->FindViewNode(GetRenderInfo().pid);
  parent->AddChildAt(shared_from_this(), render_info_.index);
}

void ViewNode::OnUpdate(hippy::DomNode& dom_node) {
  FOOTSTONE_DCHECK(render_info_.id == dom_node.GetRenderInfo().id);
  if (!IsAttached()) {
    return;
  }
  // only update the different part
  if (GetDomNode()->GetDiffStyle() != nullptr) {
    HandleStyleUpdate(*(GetDomNode()->GetDiffStyle()));
  }
}

void ViewNode::HandleStyleUpdate(const DomStyleMap& dom_style) {
  FOOTSTONE_DCHECK(IsAttached());
  auto view = GetView();
  auto const map_end = dom_style.cend();

  if (auto it = dom_style.find(view::kBackgroundColor); it != map_end) {
    FOOTSTONE_DCHECK(it->second->IsDouble());
    view->SetBackgroundColor(ViewNode::ParseToColor(it->second));
  }

  // Border Width / Border Color / Border Radius ,All in On Method
  util::ParseBorderInfo(*view, dom_style);

  if (auto it = dom_style.find(view::kLinearGradient); it != map_end) {
    util::ParseLinearGradientInfo(*view, it->second->ToObjectChecked());
  }

  // view::kLongclick will will handled in ViewNode::OnAddEventListener

  // kNextFocusDownId  / kNextFocusLeftId  / kNextFocusRightId / kNextFocusUpId

  if (auto it = dom_style.find(hippy::dom::kOpacity); it != map_end) {
    FOOTSTONE_DCHECK(it->second->IsDouble());
    view->SetOpacity(static_cast<float>(it->second->ToDoubleChecked()));
  }

  util::ParseShadowInfo(*view, dom_style);

  // animation
  view->SetTransform(GenerateAnimationTransform(dom_style, view).asM33());

  if (auto it = dom_style.find(hippy::dom::kZIndex); it != map_end) {
    view->SetZIndex(it->second->ToInt32Checked());
  }
}

tdfcore::TM44 ViewNode::GenerateAnimationTransform(const DomStyleMap& dom_style, std::shared_ptr<tdfcore::View> view) {
  auto transform = tdfcore::TM44();
  if (auto it = dom_style.find(kMatrix); it != dom_style.end()) {
    FOOTSTONE_CHECK(it->second->IsArray());
    DomValueArrayType matrix_array;
    auto result = it->second->ToArray(matrix_array);
    if (!result) {
      return transform;
    }
    FOOTSTONE_CHECK(matrix_array.size() == 16);
    for (int i = 0; i < 4; ++i) {
      auto tv4 = tdfcore::TV4();
      tv4.x = static_cast<float>(i + 0);
      tv4.y = static_cast<float>(i + 4);
      tv4.z = static_cast<float>(i + 8);
      tv4.w = static_cast<float>(i + 12);
      transform.setRow(i, tv4);
    }
  }

  if (auto it = dom_style.find(kPerspective); it != dom_style.end()) {
    FOOTSTONE_DCHECK(it->second->IsDouble());
    // M44中 2x3对应的位置就是perspective属性
    transform.setRC(2, 3, static_cast<float>(it->second->ToDoubleChecked()));
  }

  auto tv3 = tdfcore::TV3();
  if (auto it = dom_style.find(kRotateX); it != dom_style.end()) {
    FOOTSTONE_DCHECK(it->second->IsDouble());
    auto radians = static_cast<float>(it->second->ToDoubleChecked());
    tv3.x = 1;
    transform.setRotateUnit(tv3, radians);
  }

  if (auto it = dom_style.find(kRotateY); it != dom_style.end()) {
    FOOTSTONE_DCHECK(it->second->IsDouble());
    auto radians = static_cast<float>(it->second->ToDoubleChecked());
    tv3.y = 1;
    transform.setRotateUnit(tv3, radians);
  }

  if (auto it = dom_style.find(kRotate); it != dom_style.end()) {
    auto radians = static_cast<float>(it->second->ToDoubleChecked());
    tv3.z = 1;
    transform.setRotateUnit(tv3, radians);
  }

  if (auto it = dom_style.find(kRotateZ); it != dom_style.end()) {
    auto radians = static_cast<float>(it->second->ToDoubleChecked());
    tv3.z = 1;
    transform.setRotateUnit(tv3, radians);
  }

  if (auto it = dom_style.find(kScale); it != dom_style.end()) {
    FOOTSTONE_DCHECK(it->second->IsDouble());
    auto scale = static_cast<float>(it->second->ToDoubleChecked());
    transform.setScale(scale, scale);
  }

  if (auto it = dom_style.find(kScaleX); it != dom_style.end()) {
    FOOTSTONE_DCHECK(it->second->IsDouble());
    auto scale = static_cast<float>(it->second->ToDoubleChecked());
    transform.setScale(scale, 0);
  }

  if (auto it = dom_style.find(kScaleY); it != dom_style.end()) {
    FOOTSTONE_DCHECK(it->second->IsDouble());
    auto scale = static_cast<float>(it->second->ToDoubleChecked());
    transform.setScale(0, scale);
  }

  if (auto it = dom_style.find(kTranslate); it != dom_style.end()) {
    FOOTSTONE_DCHECK(it->second->IsDouble());
    DomValueArrayType translation_array;
    auto result = it->second->ToArray(translation_array);
    if (!result) {
      return transform;
    }
    FOOTSTONE_CHECK(translation_array.size() == 3);
    auto translate_x = static_cast<float>(translation_array.at(0).ToDoubleChecked());
    auto translate_y = static_cast<float>(translation_array.at(1).ToDoubleChecked());
    auto translate_z = static_cast<float>(translation_array.at(2).ToDoubleChecked());
    transform.setTranslate(translate_x, translate_y, translate_z);
  }

  if (auto it = dom_style.find(kTranslateX); it != dom_style.end()) {
    FOOTSTONE_DCHECK(it->second->IsDouble());
    auto translate_x = static_cast<float>(it->second->ToDoubleChecked());
    transform.setTranslate(translate_x, 0);
  }

  if (auto it = dom_style.find(kTranslateY); it != dom_style.end()) {
    FOOTSTONE_DCHECK(it->second->IsDouble());
    auto translate_y = static_cast<float>(it->second->ToDoubleChecked());
    transform.setTranslate(0, translate_y);
  }

  if (auto it = dom_style.find(kSkewX); it != dom_style.end()) {
    auto skew_x = static_cast<float>(it->second->ToDoubleChecked());
    transform.setRC(0, 1, skew_x);
  }

  if (auto it = dom_style.find(kSkewY); it != dom_style.end()) {
    auto skew_y = static_cast<float>(it->second->ToDoubleChecked());
    transform.setRC(1, 0, skew_y);
  }
  return transform;
}

void ViewNode::OnDelete() {
  // Do Not Call GetDomNode() here
  // recursively delete children at first(unmount the sub tree)
  for (auto it = children_.rbegin(); it != children_.rend(); it++) {
    (*it)->OnDelete();
  }
  FOOTSTONE_DCHECK(!parent_.expired());
  parent_.lock()->RemoveChild(shared_from_this());
  GetRootNode()->UnregisterViewNode(render_info_.id);
}

void ViewNode::HandleLayoutUpdate(hippy::LayoutResult layout_result) {
  TDF_RENDER_CHECK_ATTACH
  auto new_frame =
      tdfcore::TRect::MakeXYWH(layout_result.left, layout_result.top, layout_result.width, layout_result.height);
  GetView()->SetFrame(new_frame);
  FOOTSTONE_LOG(INFO) << "ViewNode::HandleLayoutUpdate: " << render_info_.id << " |" << new_frame.X() << " | "
                      << new_frame.Y() << " | " << new_frame.Width() << " | " << new_frame.Height() << " |  "
                      << render_info_.pid << " | " << GetViewName();
  layout_listener_.Notify(new_frame);
}

std::shared_ptr<tdfcore::View> ViewNode::CreateView() { return TDF_MAKE_SHARED(tdfcore::View); }

tdfcore::Color ViewNode::ParseToColor(const std::shared_ptr<footstone::HippyValue>& value) {
  // Temp solution: https://km.woa.com/articles/show/526929
  return util::ConversionIntToColor(static_cast<uint32_t>(value->ToDoubleChecked()));
}

void ViewNode::OnAddEventListener(uint32_t id, const std::string& name) {
  if(supported_events_.find(name) == supported_events_.end()) {
    supported_events_.emplace(name);
  }
  TDF_RENDER_CHECK_ATTACH
  HandleEventInfoUpdate();
}

void ViewNode::OnRemoveEventListener(uint32_t id, const std::string& name) {
  supported_events_.erase(name);
  TDF_RENDER_CHECK_ATTACH
  HandleEventInfoUpdate();
}

void ViewNode::HandleEventInfoUpdate() {
  RemoveGestureEvent(hippy::kClickEvent);
  if (supported_events_.find(hippy::kClickEvent) != supported_events_.end() ||
      supported_events_.find(hippy::kPressIn) != supported_events_.end() ||
      supported_events_.find(hippy::kPressOut) != supported_events_.end()) {
    RegisterClickEvent();
  }

  RemoveGestureEvent(hippy::kLongClickEvent);
  if (supported_events_.find(hippy::kLongClickEvent) != supported_events_.end()) {
    RegisterLongClickEvent();
  }
}

void ViewNode::RegisterClickEvent() {
  auto gesture_recognizer = TDF_MAKE_SHARED(tdfcore::TapGestureRecognizer);
  if (supported_events_.find(hippy::kClickEvent) != supported_events_.end()) {
    gesture_recognizer->SetTap([WEAK_THIS]() {
      DEFINE_AND_CHECK_SELF(ViewNode)
      self->SendGestureDomEvent(kClickEvent, nullptr);
    });
  }

  if (supported_events_.find(hippy::kPressIn) != supported_events_.end()) {
    gesture_recognizer->SetTapDown([WEAK_THIS](const tdfcore::TapDetails& detail) {
      DEFINE_AND_CHECK_SELF(ViewNode)
      self->SendGestureDomEvent(kPressIn, nullptr);
    });
  }

  if (supported_events_.find(hippy::kPressOut) != supported_events_.end()) {
    gesture_recognizer->SetTapUp([WEAK_THIS](const tdfcore::TapDetails& detail) {
      DEFINE_AND_CHECK_SELF(ViewNode)
      self->SendGestureDomEvent(kPressOut, nullptr);
    });
    gesture_recognizer->SetTapCancel([WEAK_THIS]() {
      DEFINE_AND_CHECK_SELF(ViewNode)
      self->SendGestureDomEvent(kPressOut, nullptr);
    });
  }

  gestures_map_.emplace(kClickEvent, gesture_recognizer);
  GetView()->AddGesture(gesture_recognizer);
}

void ViewNode::RegisterLongClickEvent() {
  auto gesture_recognizer = TDF_MAKE_SHARED(tdfcore::LongPressGestureRecognizer);
  gesture_recognizer->on_long_press_end_ =
  [WEAK_THIS](const tdfcore::LongPressEndDetails &long_press_end_details) -> void {
    DEFINE_AND_CHECK_SELF(ViewNode)
    self->SendGestureDomEvent(kLongClickEvent, nullptr);
  };
  gestures_map_.emplace(kLongClickEvent, gesture_recognizer);
  GetView()->AddGesture(gesture_recognizer);
}

void ViewNode::RemoveGestureEvent(std::string&& event_type) {
  if (gestures_map_.find(event_type) != gestures_map_.end()) {
    GetView()->RemoveGesture(gestures_map_.find(event_type)->second);
    gestures_map_.erase(event_type);
  }
}

void ViewNode::RemoveAllEventInfo() {
  RemoveGestureEvent(hippy::kClickEvent);
  RemoveGestureEvent(hippy::kLongClickEvent);
}

std::shared_ptr<hippy::DomNode> ViewNode::GetDomNode() const { return GetRootNode()->FindDomNode(render_info_.id); }

void ViewNode::OnChildAdd(const std::shared_ptr<ViewNode>& child, int64_t index) {
  // inherited from parent
  if (!IsAttached()) {
    return;
  }
  child->Attach();
}

void ViewNode::OnChildRemove(const std::shared_ptr<ViewNode>& child) { child->Detach(); }

void ViewNode::SendUIDomEvent(std::string type, const std::shared_ptr<footstone::HippyValue>& value, bool can_capture,
                              bool can_bubble) {
  auto dom_node = GetRootNode()->FindDomNode(GetRenderInfo().id);
  if (!dom_node) {
    return;
  }
  std::transform(type.begin(), type.end(), type.begin(), ::tolower);
  auto event = std::make_shared<hippy::DomEvent>(type, dom_node, can_capture, can_bubble, value);
  std::vector<std::function<void()>> ops = {[dom_node, event] { dom_node->HandleEvent(event); }};
  GetRootNode()->GetDomManager()->PostTask(hippy::Scene(std::move(ops)));
}

std::shared_ptr<RootViewNode> ViewNode::GetRootNode() const {
  FOOTSTONE_DCHECK(!root_node_.expired());
  return root_node_.lock();
}

void ViewNode::AddChildAt(const std::shared_ptr<ViewNode>& child, int32_t index) {
  FOOTSTONE_DCHECK(!child->GetParent());
  FOOTSTONE_DCHECK(index >= 0 && static_cast<uint32_t>(index) <= children_.size());
  // update related filed
  children_.insert(children_.begin() + index, child);
  child->SetParent(shared_from_this());
  // notify the ViewNode
  OnChildAdd(child, index);
}

void ViewNode::RemoveChild(const std::shared_ptr<ViewNode>& child) {
  FOOTSTONE_DCHECK(child != nullptr);
  if (auto result = std::find(children_.begin(), children_.end(), child); result != children_.end()) {
    // notify the ViewNode to sync the tdfcore::View Tree
    OnChildRemove(child);
    // Update related field
    child->SetParent(nullptr);
    children_.erase(result);
  } else {
    FOOTSTONE_DCHECK(false);
  }
}

std::shared_ptr<ViewNode> ViewNode::RemoveChildAt(int32_t index) {
  FOOTSTONE_DCHECK(index >= 0 && static_cast<uint32_t>(index) < children_.size());
  auto child = children_[footstone::checked_numeric_cast<int32_t, unsigned long>(index)];
  FOOTSTONE_DCHECK(child != nullptr);
  OnChildRemove(child);
  child->SetParent(nullptr);
  children_.erase(children_.begin() + index);
  return child;
}

void ViewNode::Attach(const std::shared_ptr<tdfcore::View>& view) {
  FOOTSTONE_DCHECK(!is_attached_);
  FOOTSTONE_DCHECK(!parent_.expired());
  FOOTSTONE_DCHECK(parent_.lock()->IsAttached());
  auto dom_node = GetDomNode();
  if (!dom_node) {
    return;
  }

  is_attached_ = true;
  if (view) {
    view->SetId(render_info_.id);
    attached_view_ = view;
  } else {
    // this should be the only caller of CreateView.
    auto v = CreateView();
    v->SetHitTestBehavior(tdfcore::HitTestBehavior::kTranslucent);
    v->SetId(render_info_.id);
    // must add to parent_, otherwise the view will be freed immediately.
    parent_.lock()->GetView()->AddView(v, GetCorrectedIndex());
    attached_view_ = v;
  }

  // Sync style/listener/etc
  HandleStyleUpdate(GenerateStyleInfo(GetDomNode()));
  HandleLayoutUpdate(dom_node->GetRenderLayoutResult());
  HandleEventInfoUpdate();
  // recursively attach the sub ViewNode tree(sycn the tdfcore::View Tree)
  for (const auto& child : children_) {
    std::shared_ptr<tdfcore::View> child_view = nullptr;
    auto child_index = static_cast<uint32_t >(child->GetRenderInfo().index);
    if (child_index < GetView()->GetChildren().size()) {
      child_view = GetView()->GetChildren()[child_index];
    }
    child->Attach(child_view);
  }

  OnAttach();  // notify the ViewNode
}

void ViewNode::Detach(bool sync_to_view_tree) {
  if (!IsAttached()) {
    return;
  }
  FOOTSTONE_DCHECK(!parent_.expired());
  RemoveAllEventInfo();
  // recursively Detach the children at first
  for (const auto& child : children_) {
    child->Detach(sync_to_view_tree);
  }
  OnDetach();

  if (sync_to_view_tree) {
    parent_.lock()->GetView()->RemoveView(GetView());
  }
  attached_view_.reset();
  is_attached_ = false;
}

}  // namespace tdf
}  // namespace render
}  // namespace hippy