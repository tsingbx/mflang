#include "ast.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Passes/PassBuilder.h" 
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

std::unique_ptr<LLVMContext> ptrTheContext;
std::unique_ptr<IRBuilder<> > ptrBuilder;
std::unique_ptr<Module> ptrTheModule;
std::unique_ptr<FunctionPassManager> ptrTheFPM;
std::unique_ptr<LoopAnalysisManager> ptrTheLAM;
std::unique_ptr<FunctionAnalysisManager> ptrTheFAM;
std::unique_ptr<CGSCCAnalysisManager> ptrTheCGAM;
std::unique_ptr<ModuleAnalysisManager> ptrTheMAM;
std::unique_ptr<PassInstrumentationCallbacks> ptrThePIC;
std::unique_ptr<StandardInstrumentations> ptrTheSI;

static std::map<std::string, Value *> NamedValues;

void InitializeModuleAndPassManager(void) {
  ptrTheContext = std::make_unique<LLVMContext>();
  ptrTheModule = std::make_unique<Module>("mflang", *ptrTheContext);
  //ptrTheModule->setDataLayout(ptrTheJIT->getDataLayout());
  ptrBuilder = std::make_unique<IRBuilder<> >(*ptrTheContext);
  
  // Create new pass and analysis managers.
  ptrTheFPM = std::make_unique<FunctionPassManager>();
  ptrTheLAM = std::make_unique<LoopAnalysisManager>();
  ptrTheFAM = std::make_unique<FunctionAnalysisManager>();
  ptrTheCGAM = std::make_unique<CGSCCAnalysisManager>();
  ptrTheMAM = std::make_unique<ModuleAnalysisManager>();
  ptrThePIC = std::make_unique<PassInstrumentationCallbacks>();
  ptrTheSI = std::make_unique<StandardInstrumentations>(*ptrTheContext, true);
  ptrTheSI->registerCallbacks(*ptrThePIC, ptrTheMAM.get());
  // Add transform passes
  // Do simple "peephole" optimizations and bit-twiddling optzns
  ptrTheFPM->addPass(InstCombinePass());
  // Reassociate expressions
  ptrTheFPM->addPass(ReassociatePass());
  // Eliminate Common SubExpressions.
  ptrTheFPM->addPass(GVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  ptrTheFPM->addPass(SimplifyCFGPass());
  // Register analysis passes used in these transform passes.
  PassBuilder PB;
  PB.registerModuleAnalyses(*ptrTheMAM);
  PB.registerFunctionAnalyses(*ptrTheFAM);
  PB.crossRegisterProxies(*ptrTheLAM, *ptrTheFAM, *ptrTheCGAM, *ptrTheMAM);
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
    ptrTheFPM->run(*theFunction, *ptrTheFAM);
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