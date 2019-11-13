//===- rv/vectorizationInfo.h - vectorizer IR using an overlay object --*- C++
//-*-===//
//
// Part of the RV Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// A scalar composite mask holding the vector bitmask and the active vector
// length.
//
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_RV_MASK_H
#define INCLUDE_RV_MASK_H

#include "llvm/IR/ValueHandle.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
class LLVMContext;
class Value;
} // namespace llvm

namespace rv {

class Mask {
  llvm::WeakTrackingVH Predicate;
  llvm::WeakTrackingVH ActiveVectorLength;

public:
  // Accessors
  llvm::Value *getPred() const { return Predicate; }
  void setPred(llvm::Value *NewPred) { Predicate = NewPred; }
  llvm::Value *getAVL() const { return ActiveVectorLength; }
  void setAVL(llvm::Value *NewAVL) { ActiveVectorLength = NewAVL; }

  // Properties (best effort)
  bool knownAllTrue() const;
  bool knownAllFalse() const;

  // Printing
  void print(llvm::raw_ostream &Out) const;
  void dump() const;

  Mask() : Predicate(nullptr), ActiveVectorLength(nullptr) {}
  Mask(llvm::Value *PredVal, llvm::Value *VLVal)
      : Predicate(PredVal), ActiveVectorLength(VLVal) {}

  bool operator==(const Mask &B) const;

  // Best-effort inference from the i1 predicate \p Pred.
  // This may set AVL if \p Pred is an ICmp, for example.
  static Mask inferFromPredicate(llvm::Value &Pred);
  // Construct a mask from the iN avl param \p EVLen.
  static Mask fromVectorLength(llvm::Value &EVLen);
  // All lanes off mask.
  static Mask getAllFalse(llvm::LLVMContext &);
  // All lanes enabled mask.
  static Mask getAllTrue() { return Mask(); }
};

} // namespace rv

namespace llvm {
raw_ostream &operator<<(raw_ostream &, const rv::Mask &M);
}

#endif // INCLUDE_RV_MASK_H
