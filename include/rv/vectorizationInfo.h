//===- rv/vectorizationInfo.h - vectorizer IR using an overlay object --*- C++ -*-===//
//
// Part of the RV Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//

#ifndef INCLUDE_RV_VECTORIZATIONINFO_H_
#define INCLUDE_RV_VECTORIZATIONINFO_H_

namespace llvm {
  class LLVMContext;
  class BasicBlock;
  class Instruction;
  class Value;
} // namespace llvm

#include "region/Region.h"
#include "rv/Mask.h"
#include "rv/shape/vectorShape.h"
#include "rv/vectorMapping.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <unordered_map>

namespace rv {

class Region;

// provides vectorization information (vector shapes, block predicates) for a
// function
class VectorizationInfo {
  const llvm::DataLayout &DL;

  // TODO infer AVL from guard branch in the future.
  llvm::Value * EntryAVL;

  // analysis context
  Region &region;
  VectorMapping mapping;

  // value, argument and instruction shapes
  std::unordered_map<const llvm::Value *, VectorShape> shapes;

  // detected divergent loops
  std::set<const llvm::Loop *> mDivergentLoops;

  // basic block properties // TODO fuse into struct
  // materialized basic block predicates
  std::unordered_map<const llvm::BasicBlock*, Mask> masks;
  // whether the block is the exit of a divergent loop exit
  std::set<const llvm::BasicBlock *> DivergentLoopExits;
  // whether the block is a join point of disjoint paths from a varying branch
  std::set<const llvm::BasicBlock *> JoinDivergentBlocks;
  // whether the block will receive a non-uniform predicate
  std::map<const llvm::BasicBlock *, bool> VaryingPredicateBlocks;

  // fixed shapes (will be preserved through VA)
  std::set<const llvm::Value *> pinned;

  // internal helper
  Mask& requestMask(const llvm::BasicBlock & block);

public:
  VectorizationInfo(Region &region, VectorMapping _mapping);
  VectorizationInfo(llvm::Function &parentFn, unsigned vectorWidth,
                    Region &region);

  const llvm::DataLayout &getDataLayout() const;
  const VectorMapping &getMapping() const { return mapping; }
  size_t getVectorWidth() const { return mapping.vectorWidth; }

  // region related
  Region &getRegion() const { return region; }
  bool inRegion(const llvm::Instruction &inst) const;
  bool inRegion(const llvm::BasicBlock &block) const;
  llvm::BasicBlock &getEntry() const;

  void setEntryAVL(llvm::Value * NewAVL) { EntryAVL = NewAVL; }
  llvm::Value* getEntryAVL() const { return EntryAVL; }

  // disjoin path divergence
  bool isJoinDivergent(const llvm::BasicBlock &JoinBlock) const {
    return JoinDivergentBlocks.count(&JoinBlock);
  }
  bool addJoinDivergentBlock(const llvm::BasicBlock &JoinBlock) {
    return JoinDivergentBlocks.insert(&JoinBlock).second;
  }

  // loop divergence
  bool addDivergentLoop(const llvm::Loop &divLoop);
  void removeDivergentLoop(const llvm::Loop &divLoop);
  bool isDivergentLoop(const llvm::Loop &loop) const;
  bool isDivergentLoopTopLevel(const llvm::Loop &loop) const;

  // loop exit divergence
  bool isDivergentLoopExit(const llvm::BasicBlock &block) const;
  bool isKillExit(const llvm::BasicBlock &block) const {
    return !isDivergentLoopExit(block);
  }
  bool addDivergentLoopExit(const llvm::BasicBlock &block);
  void removeDivergentLoopExit(const llvm::BasicBlock &block);

  /// Disable recomputation of this value's shape and make it effectvely final
  const decltype(pinned) &pinned_values() const { return pinned; }
  void setPinned(const llvm::Value &);
  void setPinnedShape(const llvm::Value &v, VectorShape shape) {
    setPinned(v);
    setVectorShape(v, shape);
  }
  bool isPinned(const llvm::Value &) const;

  // vector shape
  // get the shape of @val observed at @observerBlock. This will be varying if
  // @val is defined in divergent loop.
  VectorShape getObservedShape(const llvm::LoopInfo &LI,
                               const llvm::BasicBlock &observerBlock,
                               const llvm::Value &val) const;

  // get the shape of @val observerd in the defining block of @val (if it is an
  // instruction).
  VectorShape getVectorShape(const llvm::Value &val) const;
  VectorShape getVectorShape(const Mask &M) const;
  bool hasKnownShape(const llvm::Value &val) const;

  void setVectorShape(const llvm::Value &val, VectorShape shape);
  void setVectorShape(const Mask &M, VectorShape S);
  void dropVectorShape(const llvm::Value &val);

  bool isTemporalDivergent(const llvm::LoopInfo &LI,
                           const llvm::BasicBlock &ObservingBlock,
                           const llvm::Value &Val) const;

  // tentative block predicate shapes (whether the basic block predicate will be
  // varying or uniform)
  // state can be unknown <returns false, flag& unaltered>, varying (returns true, oIsVarying is true or uniform (returns true, oIsVarying is false)
  bool getVaryingPredicateFlag(const llvm::BasicBlock &BB, bool & oIsVarying) const; // TODO rename to setCMaskShape()
  void setVaryingPredicateFlag(const llvm::BasicBlock &, bool toVarying); // TODO rename to getCMaskShape()
  void removeVaryingPredicateFlag(const llvm::BasicBlock &); // TODO rename to dropCMaskShape() == unknown control mask

  // mask == i1 predicate X avl
  bool hasMask(const llvm::BasicBlock & block) const;
  const Mask& getMask(const llvm::BasicBlock & block) const;
  void dropMask(const llvm::BasicBlock &block);

  // actual basic block predicates
  void setMask(const llvm::BasicBlock &block, Mask NewMask);
  llvm::Value *getPredicate(const llvm::BasicBlock &block) const;
  void setPredicate(const llvm::BasicBlock &block, llvm::Value &predicate);
  void remapPredicate(llvm::Value &dest, llvm::Value &old);

  // print
  void dump() const;
  void print(llvm::raw_ostream &out) const;
  void dump(const llvm::Value *val) const;
  void print(const llvm::Value *val, llvm::raw_ostream &) const;
  void printBlockInfo(const llvm::BasicBlock &block, llvm::raw_ostream &) const;
  void dumpBlockInfo(const llvm::BasicBlock &block) const;
  void printArguments(llvm::raw_ostream &) const;
  void dumpArguments() const;

  llvm::LLVMContext &getContext() const;
  llvm::Function &getScalarFunction() { return *mapping.scalarFn; }
  const llvm::Function &getScalarFunction() const { return *mapping.scalarFn; }
  llvm::Function &getVectorFunction() { return *mapping.vectorFn; }
};

} // namespace rv

#endif /* INCLUDE_RV_VECTORIZATIONINFO_H_ */
