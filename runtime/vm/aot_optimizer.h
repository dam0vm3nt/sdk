// Copyright (c) 2012, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef RUNTIME_VM_AOT_OPTIMIZER_H_
#define RUNTIME_VM_AOT_OPTIMIZER_H_

#include "vm/intermediate_language.h"
#include "vm/flow_graph.h"

namespace dart {

class CSEInstructionMap;
template <typename T> class GrowableArray;
class ParsedFunction;
class RawBool;

class AotOptimizer : public FlowGraphVisitor {
 public:
  AotOptimizer(FlowGraph* flow_graph,
               bool use_speculative_inlining,
               GrowableArray<intptr_t>* inlining_black_list);
  virtual ~AotOptimizer() {}

  FlowGraph* flow_graph() const { return flow_graph_; }

  // Add ICData to InstanceCalls, so that optimizations can be run on them.
  // TODO(srdjan): StaticCals as well?
  void PopulateWithICData();

  // Use ICData to optimize, replace or eliminate instructions.
  void ApplyICData();

  // Use propagated class ids to optimize, replace or eliminate instructions.
  void ApplyClassIds();

  void ReplaceArrayBoundChecks();

  virtual void VisitStaticCall(StaticCallInstr* instr);
  virtual void VisitInstanceCall(InstanceCallInstr* instr);
  virtual void VisitPolymorphicInstanceCall(
      PolymorphicInstanceCallInstr* instr);
  virtual void VisitLoadCodeUnits(LoadCodeUnitsInstr* instr);

  void InsertBefore(Instruction* next,
                    Instruction* instr,
                    Environment* env,
                    FlowGraph::UseKind use_kind) {
    flow_graph_->InsertBefore(next, instr, env, use_kind);
  }

 private:
  // Attempt to build ICData for call using propagated class-ids.
  bool TryCreateICData(InstanceCallInstr* call);
  const ICData& TrySpecializeICData(const ICData& ic_data, intptr_t cid);

  bool TryReplaceWithIndexedOp(InstanceCallInstr* call);

  bool TryReplaceWithBinaryOp(InstanceCallInstr* call, Token::Kind op_kind);
  bool TryReplaceWithUnaryOp(InstanceCallInstr* call, Token::Kind op_kind);

  bool TryReplaceWithEqualityOp(InstanceCallInstr* call, Token::Kind op_kind);
  bool TryReplaceWithRelationalOp(InstanceCallInstr* call, Token::Kind op_kind);

  bool TryInlineFieldAccess(InstanceCallInstr* call);
  bool TryInlineInstanceGetter(InstanceCallInstr* call);
  bool TryInlineInstanceSetter(InstanceCallInstr* call,
                               const ICData& unary_ic_data);

  bool TryInlineInstanceMethod(InstanceCallInstr* call);
  void ReplaceWithInstanceOf(InstanceCallInstr* instr);
  bool TypeCheckAsClassEquality(const AbstractType& type);
  void ReplaceWithTypeCast(InstanceCallInstr* instr);

  bool TryReplaceInstanceCallWithInline(InstanceCallInstr* call);

  // Insert a check of 'to_check' determined by 'unary_checks'.  If the
  // check fails it will deoptimize to 'deopt_id' using the deoptimization
  // environment 'deopt_environment'.  The check is inserted immediately
  // before 'insert_before'.
  void AddCheckClass(Definition* to_check,
                     const ICData& unary_checks,
                     intptr_t deopt_id,
                     Environment* deopt_environment,
                     Instruction* insert_before);
  Instruction* GetCheckClass(Definition* to_check,
                             const ICData& unary_checks,
                             intptr_t deopt_id,
                             TokenPosition token_pos);

  // Insert a Smi check if needed.
  void AddCheckSmi(Definition* to_check,
                   intptr_t deopt_id,
                   Environment* deopt_environment,
                   Instruction* insert_before);

  // Add a class check for a call's first argument immediately before the
  // call, using the call's IC data to determine the check, and the call's
  // deopt ID and deoptimization environment if the check fails.
  void AddReceiverCheck(InstanceCallInstr* call);

  void ReplaceCall(Definition* call, Definition* replacement);

  bool InstanceCallNeedsClassCheck(InstanceCallInstr* call,
                                   RawFunction::Kind kind) const;

  bool InlineFloat32x4BinaryOp(InstanceCallInstr* call,
                               Token::Kind op_kind);
  bool InlineInt32x4BinaryOp(InstanceCallInstr* call,
                              Token::Kind op_kind);
  bool InlineFloat64x2BinaryOp(InstanceCallInstr* call,
                               Token::Kind op_kind);
  bool InlineImplicitInstanceGetter(InstanceCallInstr* call);

  RawBool* InstanceOfAsBool(const ICData& ic_data,
                            const AbstractType& type,
                            ZoneGrowableArray<intptr_t>* results) const;

  void ReplaceWithMathCFunction(InstanceCallInstr* call,
                                MethodRecognizer::Kind recognized_kind);

  bool TryStringLengthOneEquality(InstanceCallInstr* call, Token::Kind op_kind);

  RawField* GetField(intptr_t class_id, const String& field_name);

  Thread* thread() const { return flow_graph_->thread(); }
  Isolate* isolate() const { return flow_graph_->isolate(); }
  Zone* zone() const { return flow_graph_->zone(); }

  const Function& function() const { return flow_graph_->function(); }

  bool IsAllowedForInlining(intptr_t deopt_id);

  FlowGraph* flow_graph_;

  const bool use_speculative_inlining_;

  GrowableArray<intptr_t>* inlining_black_list_;

  bool has_unique_no_such_method_;

  DISALLOW_COPY_AND_ASSIGN(AotOptimizer);
};


}  // namespace dart

#endif  // RUNTIME_VM_AOT_OPTIMIZER_H_
