// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-utils-gen.h"
#include "src/builtins/builtins.h"
#include "src/codegen/code-stub-assembler.h"
#include "src/utils/utils.h"

namespace v8 {
namespace internal {

// CSA builtins for Shredder
TF_BUILTIN(HTGetField, CodeStubAssembler) {
  Node* context = Parameter(Descriptor::kContext);
  Node* buffer = Parameter(Descriptor::kX);
  Node* k = Parameter(Descriptor::kY);
  Node* const key = TruncateTaggedToWord32(context, k);
  Node* x = LoadObjectField(buffer, JSArrayBuffer::kBackingStoreOffset);
  Node* value = Load(MachineType::Uint32(), IntPtrAdd(x, IntPtrMul(BitcastTaggedToWord(key), IntPtrConstant(4))));
  Node* result = ChangeUint32ToTagged(value);
  Return(result);
}

TF_BUILTIN(HTGet, CodeStubAssembler) {
  Node* context = Parameter(Descriptor::kContext);
  Node* p = Parameter(Descriptor::kP);
  Node* k = Parameter(Descriptor::kKey);
  Node* buffer = Parameter(Descriptor::kBuffer);
  Node* const key = TruncateTaggedToWord32(context, k);
  Node* table = LoadObjectField(p, JSArrayBuffer::kBackingStoreOffset);
  Node* const hash = ChangeInt32ToIntPtr(Int32Mod(ComputeIntegerHash(key, Int32Constant(0)), Int32Constant(1000000)));
  Node* pointer = Load(MachineType::Int64(), IntPtrAdd(table, IntPtrMul(hash, IntPtrConstant(8))));
  VARIABLE(var_p, MachineType::PointerRepresentation(), pointer);
  VARIABLE(var_key, MachineRepresentation::kWord32, key);
  Label loop(this, {&var_p, &var_key}), not_found(this), if_equal(this);
  Goto(&loop);
  BIND(&loop);
  {
    GotoIf(
        WordEqual(ReinterpretCast<WordT>(var_p.value()), IntPtrConstant(0)),
        &not_found);
    Node* x = Load(MachineType::Uint32(), var_p.value());
    GotoIf(Word32Equal(ReinterpretCast<Word32T>(var_key.value()), ReinterpretCast<Word32T>(x)), &if_equal);
    var_p.Bind(Load(MachineType::Int64(),IntPtrAdd(var_p.value(), IntPtrConstant(4))));
    Goto(&loop);
  }

  BIND(&not_found);
  {
    Return(UndefinedConstant());
  }
  BIND(&if_equal);

  Node* data = Load(MachineType::Int64(), IntPtrAdd(var_p.value(), IntPtrConstant(12)));
  StoreObjectField(buffer, JSArrayBuffer::kBackingStoreOffset, data);
  Node* length = Load(MachineType::Uint32(), IntPtrAdd(var_p.value(), IntPtrConstant(20)));
  StoreObjectField(buffer, JSArrayBuffer::kByteLengthOffset, length);

  Return(ChangeUint32ToTagged(length));
}

// ES #sec-isfinite-number
TF_BUILTIN(GlobalIsFinite, CodeStubAssembler) {
  Node* context = Parameter(Descriptor::kContext);

  Label return_true(this), return_false(this);

  // We might need to loop once for ToNumber conversion.
  VARIABLE(var_num, MachineRepresentation::kTagged);
  Label loop(this, &var_num);
  var_num.Bind(Parameter(Descriptor::kNumber));
  Goto(&loop);
  BIND(&loop);
  {
    Node* num = var_num.value();

    // Check if {num} is a Smi or a HeapObject.
    GotoIf(TaggedIsSmi(num), &return_true);

    // Check if {num} is a HeapNumber.
    Label if_numisheapnumber(this),
        if_numisnotheapnumber(this, Label::kDeferred);
    Branch(IsHeapNumber(num), &if_numisheapnumber, &if_numisnotheapnumber);

    BIND(&if_numisheapnumber);
    {
      // Check if {num} contains a finite, non-NaN value.
      TNode<Float64T> num_value = LoadHeapNumberValue(num);
      BranchIfFloat64IsNaN(Float64Sub(num_value, num_value), &return_false,
                           &return_true);
    }

    BIND(&if_numisnotheapnumber);
    {
      // Need to convert {num} to a Number first.
      var_num.Bind(CallBuiltin(Builtins::kNonNumberToNumber, context, num));
      Goto(&loop);
    }
  }

  BIND(&return_true);
  Return(TrueConstant());

  BIND(&return_false);
  Return(FalseConstant());
}

// ES6 #sec-isnan-number
TF_BUILTIN(GlobalIsNaN, CodeStubAssembler) {
  Node* context = Parameter(Descriptor::kContext);

  Label return_true(this), return_false(this);

  // We might need to loop once for ToNumber conversion.
  VARIABLE(var_num, MachineRepresentation::kTagged);
  Label loop(this, &var_num);
  var_num.Bind(Parameter(Descriptor::kNumber));
  Goto(&loop);
  BIND(&loop);
  {
    Node* num = var_num.value();

    // Check if {num} is a Smi or a HeapObject.
    GotoIf(TaggedIsSmi(num), &return_false);

    // Check if {num} is a HeapNumber.
    Label if_numisheapnumber(this),
        if_numisnotheapnumber(this, Label::kDeferred);
    Branch(IsHeapNumber(num), &if_numisheapnumber, &if_numisnotheapnumber);

    BIND(&if_numisheapnumber);
    {
      // Check if {num} contains a NaN.
      TNode<Float64T> num_value = LoadHeapNumberValue(num);
      BranchIfFloat64IsNaN(num_value, &return_true, &return_false);
    }

    BIND(&if_numisnotheapnumber);
    {
      // Need to convert {num} to a Number first.
      var_num.Bind(CallBuiltin(Builtins::kNonNumberToNumber, context, num));
      Goto(&loop);
    }
  }

  BIND(&return_true);
  Return(TrueConstant());

  BIND(&return_false);
  Return(FalseConstant());
}

}  // namespace internal
}  // namespace v8
