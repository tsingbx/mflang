#include "ast.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Verifier.h"

std::unique_ptr<LLVMContext> ptrTheContext;
std::unique_ptr<IRBuilder<> > ptrBuilder;
std::unique_ptr<Module> ptrTheModule;
std::unique_ptr<llvm::legacy::FunctionPassManager> ptrTheFPM;

static std::map<std::string, Value *> NamedValues;

void InitializeModule() {
  ptrTheContext = std::make_unique<LLVMContext>();
  ptrBuilder = std::make_unique<IRBuilder<> >(*ptrTheContext);
  ptrTheModule = std::make_unique<Module>("mflang", *ptrTheContext);
}

void InitializeModuleAndPassManager(void) {
    // Create a new pass manager attached to it.
    ptrTheFPM = std::make_unique<llvm::legacy::FunctionPassManager>(ptrTheModule.get());
    // Do simple "peephole" optimizations and bit-twiddling optzns
    ptrTheFPM->add(createInstructionCombiningPass());
    // Reassociate expressions.
    ptrTheFPM->add(createReassociatePass());
    // Eliminate Common SubExpressions.
    ptrTheFPM->add(createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    ptrTheFPM->add(createCFGSimplificationPass());
    ptrTheFPM->doInitialization();
}

void PrintModule() {
  ptrTheModule->print(errs(), nullptr);
}

Value *NumberExprAST::codegen()
{
  return ConstantFP::get(*ptrTheContext, APFloat(_val));
}

Value *VariableExprAST::codegen()
{
  // Look this variable up in the function.
  Value *v = NamedValues[_name];
  if (!v)
    LogErrorV("Unknown variable name");
  return v;
}

BinaryExprAST::BinaryExprAST(char op, std::unique_ptr<ExprAST> lhs,
                             std::unique_ptr<ExprAST> rhs)
    : _op(op), _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

Value *BinaryExprAST::codegen()
{
  Value *l = _lhs->codegen();
  Value *r = _rhs->codegen();
  if (!l || !r)
    return nullptr;

  switch (_op)
  {
  case '+':
    return ptrBuilder->CreateFAdd(l, r, "addtmp");
  case '-':
    return ptrBuilder->CreateFSub(l, r, "subtmp");
  case '*':
    return ptrBuilder->CreateFMul(l, r, "multmp");
  case '/':
    return ptrBuilder->CreateFDiv(l, r, "divtmp");
  case '<':
    l = ptrBuilder->CreateFCmpULT(l, r, "cmpULTtmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return ptrBuilder->CreateUIToFP(l, Type::getDoubleTy(*ptrTheContext),
                                "booltmp");
  case '>':
    l = ptrBuilder->CreateFCmpUGT(l, r, "cmpUGTtmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return ptrBuilder->CreateUIToFP(l, Type::getDoubleTy(*ptrTheContext),
                                "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}

Value *CallExprAST::codegen()
{
  // Look up the name in the global module table.
  Function *calleeF = ptrTheModule->getFunction(_callee);
  if (!calleeF)
    return LogErrorV("Unknown function referenced");

  // If argument mismatch error.
  if (calleeF->arg_size() != _args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> argsV;
  for (unsigned i = 0, e = _args.size(); i != e; ++i)
  {
    argsV.push_back(_args[i]->codegen());
    if (!argsV.back())
      return nullptr;
  }

  return ptrBuilder->CreateCall(calleeF, argsV, "calltmp");
}

Function *PrototypeAST::codegen()
{
  std::vector<Type *> doubles(_args.size(), Type::getDoubleTy(*ptrTheContext));
  FunctionType *ft =
      FunctionType::get(Type::getDoubleTy(*ptrTheContext), doubles, false);
  Function *f = Function::Create(ft, Function::ExternalLinkage, _name, ptrTheModule.get());
  // Set names for all arguments.
  unsigned idx = 0;
  for (auto &arg : f->args())
    arg.setName(_args[idx++]);
  return f;
}

Function *FunctionAST::codegen()
{
  // First, check for an existing function from a previous 'extern' declaration.
  Function *theFunction = ptrTheModule->getFunction(_proto->getName());

  if (!theFunction)
    theFunction = _proto->codegen();

  if (!theFunction)
    return nullptr;

  if (!theFunction->empty())
    return (Function *)LogErrorV("Function cannot be redefined.");

  // Create a new basic block to start insertion into.
  BasicBlock *bb = BasicBlock::Create(*ptrTheContext, "entry", theFunction);
  ptrBuilder->SetInsertPoint(bb);

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  for (auto &arg : theFunction->args())
  {
    NamedValues[arg.getName().str()] = &arg;
  }
  if (Value *retVal = _body->codegen())
  {
    // Finish off the function.
    ptrBuilder->CreateRet(retVal);
    // Validate the generated code, checking for consistency.
    verifyFunction(*theFunction);
    // Optimize the function
    ptrTheFPM->run(*theFunction);
    return theFunction;
  }
  // Error reading body, remove function.
  theFunction->eraseFromParent();
  return nullptr;
}

std::unique_ptr<ExprAST> LogError(const char *str)
{
  fprintf(stderr, "Error: %s\n", str);
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *str)
{
  LogError(str);
  return nullptr;
}

Value *LogErrorV(const char *Str)
{
  LogError(Str);
  return nullptr;
}