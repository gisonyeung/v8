// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines internal versions of the public API structs. These should
// all be tidy and simple classes which maintain proper ownership (unique_ptr)
// of each other. Each contains an instance of its corresponding public type,
// which can be filled out with GetPublicView.

#ifndef V8_TOOLS_DEBUG_HELPER_DEBUG_HELPER_INTERNAL_H_
#define V8_TOOLS_DEBUG_HELPER_DEBUG_HELPER_INTERNAL_H_

#include <memory>
#include <string>
#include <vector>

#include "debug-helper.h"
#include "src/objects/instance-type.h"

namespace d = v8::debug_helper;

namespace v8_debug_helper_internal {

// A value that was read from the debuggee's memory.
template <typename TValue>
struct Value {
  d::MemoryAccessResult validity;
  TValue value;
};

class ObjectProperty {
 public:
  inline ObjectProperty(std::string name, std::string type,
                        std::string decompressed_type, uintptr_t address,
                        size_t num_values = 1,
                        d::PropertyKind kind = d::PropertyKind::kSingle)
      : name_(name),
        type_(type),
        decompressed_type_(decompressed_type),
        address_(address),
        num_values_(num_values),
        kind_(kind) {}

  inline d::ObjectProperty* GetPublicView() {
    public_view_.name = name_.c_str();
    public_view_.type = type_.c_str();
    public_view_.decompressed_type = decompressed_type_.c_str();
    public_view_.address = address_;
    public_view_.num_values = num_values_;
    public_view_.kind = kind_;
    return &public_view_;
  }

 private:
  std::string name_;
  std::string type_;
  std::string decompressed_type_;
  uintptr_t address_;
  size_t num_values_;
  d::PropertyKind kind_;

  d::ObjectProperty public_view_;
};

class ObjectPropertiesResult;
using ObjectPropertiesResultInternal = ObjectPropertiesResult;

struct ObjectPropertiesResultExtended : public d::ObjectPropertiesResult {
  ObjectPropertiesResultInternal* base;  // Back reference for cleanup
};

class ObjectPropertiesResult {
 public:
  inline ObjectPropertiesResult(
      d::TypeCheckResult type_check_result, std::string brief, std::string type,
      std::vector<std::unique_ptr<ObjectProperty>> properties)
      : type_check_result_(type_check_result),
        brief_(brief),
        type_(type),
        properties_(std::move(properties)) {}

  inline void Prepend(const char* prefix) { brief_ = prefix + brief_; }

  inline d::ObjectPropertiesResult* GetPublicView() {
    public_view_.type_check_result = type_check_result_;
    public_view_.brief = brief_.c_str();
    public_view_.type = type_.c_str();
    public_view_.num_properties = properties_.size();
    properties_raw_.resize(0);
    for (const auto& property : properties_) {
      properties_raw_.push_back(property->GetPublicView());
    }
    public_view_.properties = properties_raw_.data();
    public_view_.base = this;
    return &public_view_;
  }

 private:
  d::TypeCheckResult type_check_result_;
  std::string brief_;
  std::string type_;
  std::vector<std::unique_ptr<ObjectProperty>> properties_;

  ObjectPropertiesResultExtended public_view_;
  std::vector<d::ObjectProperty*> properties_raw_;
};

class TqObjectVisitor;

// Base class representing a V8 object in the debuggee's address space.
// Subclasses for specific object types are generated by the Torque compiler.
class TqObject {
 public:
  inline TqObject(uintptr_t address) : address_(address) {}
  virtual ~TqObject() = default;
  virtual std::vector<std::unique_ptr<ObjectProperty>> GetProperties(
      d::MemoryAccessor accessor) const;
  virtual const char* GetName() const;
  virtual void Visit(TqObjectVisitor* visitor) const;

 protected:
  uintptr_t address_;
};

bool IsPointerCompressed(uintptr_t address);
uintptr_t Decompress(uintptr_t address, uintptr_t any_uncompressed_address);
d::PropertyKind GetArrayKind(d::MemoryAccessResult mem_result);

}  // namespace v8_debug_helper_internal

#endif
