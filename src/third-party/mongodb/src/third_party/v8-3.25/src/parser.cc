// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "v8.h"

#include "api.h"
#include "ast.h"
#include "bootstrapper.h"
#include "char-predicates-inl.h"
#include "codegen.h"
#include "compiler.h"
#include "messages.h"
#include "parser.h"
#include "platform.h"
#include "preparser.h"
#include "runtime.h"
#include "scanner-character-streams.h"
#include "scopeinfo.h"
#include "string-stream.h"

namespace v8 {
namespace internal {

RegExpBuilder::RegExpBuilder(Zone* zone)
    : zone_(zone),
      pending_empty_(false),
      characters_(NULL),
      terms_(),
      alternatives_()
#ifdef DEBUG
    , last_added_(ADD_NONE)
#endif
  {}


void RegExpBuilder::FlushCharacters() {
  pending_empty_ = false;
  if (characters_ != NULL) {
    RegExpTree* atom = new(zone()) RegExpAtom(characters_->ToConstVector());
    characters_ = NULL;
    text_.Add(atom, zone());
    LAST(ADD_ATOM);
  }
}


void RegExpBuilder::FlushText() {
  FlushCharacters();
  int num_text = text_.length();
  if (num_text == 0) {
    return;
  } else if (num_text == 1) {
    terms_.Add(text_.last(), zone());
  } else {
    RegExpText* text = new(zone()) RegExpText(zone());
    for (int i = 0; i < num_text; i++)
      text_.Get(i)->AppendToText(text, zone());
    terms_.Add(text, zone());
  }
  text_.Clear();
}


void RegExpBuilder::AddCharacter(uc16 c) {
  pending_empty_ = false;
  if (characters_ == NULL) {
    characters_ = new(zone()) ZoneList<uc16>(4, zone());
  }
  characters_->Add(c, zone());
  LAST(ADD_CHAR);
}


void RegExpBuilder::AddEmpty() {
  pending_empty_ = true;
}


void RegExpBuilder::AddAtom(RegExpTree* term) {
  if (term->IsEmpty()) {
    AddEmpty();
    return;
  }
  if (term->IsTextElement()) {
    FlushCharacters();
    text_.Add(term, zone());
  } else {
    FlushText();
    terms_.Add(term, zone());
  }
  LAST(ADD_ATOM);
}


void RegExpBuilder::AddAssertion(RegExpTree* assert) {
  FlushText();
  terms_.Add(assert, zone());
  LAST(ADD_ASSERT);
}


void RegExpBuilder::NewAlternative() {
  FlushTerms();
}


void RegExpBuilder::FlushTerms() {
  FlushText();
  int num_terms = terms_.length();
  RegExpTree* alternative;
  if (num_terms == 0) {
    alternative = RegExpEmpty::GetInstance();
  } else if (num_terms == 1) {
    alternative = terms_.last();
  } else {
    alternative = new(zone()) RegExpAlternative(terms_.GetList(zone()));
  }
  alternatives_.Add(alternative, zone());
  terms_.Clear();
  LAST(ADD_NONE);
}


RegExpTree* RegExpBuilder::ToRegExp() {
  FlushTerms();
  int num_alternatives = alternatives_.length();
  if (num_alternatives == 0) {
    return RegExpEmpty::GetInstance();
  }
  if (num_alternatives == 1) {
    return alternatives_.last();
  }
  return new(zone()) RegExpDisjunction(alternatives_.GetList(zone()));
}


void RegExpBuilder::AddQuantifierToAtom(
    int min, int max, RegExpQuantifier::QuantifierType quantifier_type) {
  if (pending_empty_) {
    pending_empty_ = false;
    return;
  }
  RegExpTree* atom;
  if (characters_ != NULL) {
    ASSERT(last_added_ == ADD_CHAR);
    // Last atom was character.
    Vector<const uc16> char_vector = characters_->ToConstVector();
    int num_chars = char_vector.length();
    if (num_chars > 1) {
      Vector<const uc16> prefix = char_vector.SubVector(0, num_chars - 1);
      text_.Add(new(zone()) RegExpAtom(prefix), zone());
      char_vector = char_vector.SubVector(num_chars - 1, num_chars);
    }
    characters_ = NULL;
    atom = new(zone()) RegExpAtom(char_vector);
    FlushText();
  } else if (text_.length() > 0) {
    ASSERT(last_added_ == ADD_ATOM);
    atom = text_.RemoveLast();
    FlushText();
  } else if (terms_.length() > 0) {
    ASSERT(last_added_ == ADD_ATOM);
    atom = terms_.RemoveLast();
    if (atom->max_match() == 0) {
      // Guaranteed to only match an empty string.
      LAST(ADD_TERM);
      if (min == 0) {
        return;
      }
      terms_.Add(atom, zone());
      return;
    }
  } else {
    // Only call immediately after adding an atom or character!
    UNREACHABLE();
    return;
  }
  terms_.Add(
      new(zone()) RegExpQuantifier(min, max, quantifier_type, atom), zone());
  LAST(ADD_TERM);
}


Handle<String> Parser::LookupCachedSymbol(int symbol_id) {
  // Make sure the cache is large enough to hold the symbol identifier.
  if (symbol_cache_.length() <= symbol_id) {
    // Increase length to index + 1.
    symbol_cache_.AddBlock(Handle<String>::null(),
                           symbol_id + 1 - symbol_cache_.length(), zone());
  }
  Handle<String> result = symbol_cache_.at(symbol_id);
  if (result.is_null()) {
    result = scanner()->AllocateInternalizedString(isolate_);
    ASSERT(!result.is_null());
    symbol_cache_.at(symbol_id) = result;
    return result;
  }
  isolate()->counters()->total_preparse_symbols_skipped()->Increment();
  return result;
}


FunctionEntry ScriptDataImpl::GetFunctionEntry(int start) {
  // The current pre-data entry must be a FunctionEntry with the given
  // start position.
  if ((function_index_ + FunctionEntry::kSize <= store_.length())
      && (static_cast<int>(store_[function_index_]) == start)) {
    int index = function_index_;
    function_index_ += FunctionEntry::kSize;
    return FunctionEntry(store_.SubVector(index,
                                          index + FunctionEntry::kSize));
  }
  return FunctionEntry();
}


int ScriptDataImpl::GetSymbolIdentifier() {
  return ReadNumber(&symbol_data_);
}


bool ScriptDataImpl::SanityCheck() {
  // Check that the header data is valid and doesn't specify
  // point to positions outside the store.
  if (store_.length() < PreparseDataConstants::kHeaderSize) return false;
  if (magic() != PreparseDataConstants::kMagicNumber) return false;
  if (version() != PreparseDataConstants::kCurrentVersion) return false;
  if (has_error()) {
    // Extra sane sanity check for error message encoding.
    if (store_.length() <= PreparseDataConstants::kHeaderSize
                         + PreparseDataConstants::kMessageTextPos) {
      return false;
    }
    if (Read(PreparseDataConstants::kMessageStartPos) >
        Read(PreparseDataConstants::kMessageEndPos)) {
      return false;
    }
    unsigned arg_count = Read(PreparseDataConstants::kMessageArgCountPos);
    int pos = PreparseDataConstants::kMessageTextPos;
    for (unsigned int i = 0; i <= arg_count; i++) {
      if (store_.length() <= PreparseDataConstants::kHeaderSize + pos) {
        return false;
      }
      int length = static_cast<int>(Read(pos));
      if (length < 0) return false;
      pos += 1 + length;
    }
    if (store_.length() < PreparseDataConstants::kHeaderSize + pos) {
      return false;
    }
    return true;
  }
  // Check that the space allocated for function entries is sane.
  int functions_size =
      static_cast<int>(store_[PreparseDataConstants::kFunctionsSizeOffset]);
  if (functions_size < 0) return false;
  if (functions_size % FunctionEntry::kSize != 0) return false;
  // Check that the count of symbols is non-negative.
  int symbol_count =
      static_cast<int>(store_[PreparseDataConstants::kSymbolCountOffset]);
  if (symbol_count < 0) return false;
  // Check that the total size has room for header and function entries.
  int minimum_size =
      PreparseDataConstants::kHeaderSize + functions_size;
  if (store_.length() < minimum_size) return false;
  return true;
}



const char* ScriptDataImpl::ReadString(unsigned* start, int* chars) {
  int length = start[0];
  char* result = NewArray<char>(length + 1);
  for (int i = 0; i < length; i++) {
    result[i] = start[i + 1];
  }
  result[length] = '\0';
  if (chars != NULL) *chars = length;
  return result;
}


Scanner::Location ScriptDataImpl::MessageLocation() {
  int beg_pos = Read(PreparseDataConstants::kMessageStartPos);
  int end_pos = Read(PreparseDataConstants::kMessageEndPos);
  return Scanner::Location(beg_pos, end_pos);
}


const char* ScriptDataImpl::BuildMessage() {
  unsigned* start = ReadAddress(PreparseDataConstants::kMessageTextPos);
  return ReadString(start, NULL);
}


Vector<const char*> ScriptDataImpl::BuildArgs() {
  int arg_count = Read(PreparseDataConstants::kMessageArgCountPos);
  const char** array = NewArray<const char*>(arg_count);
  // Position after text found by skipping past length field and
  // length field content words.
  int pos = PreparseDataConstants::kMessageTextPos + 1
      + Read(PreparseDataConstants::kMessageTextPos);
  for (int i = 0; i < arg_count; i++) {
    int count = 0;
    array[i] = ReadString(ReadAddress(pos), &count);
    pos += count + 1;
  }
  return Vector<const char*>(array, arg_count);
}


unsigned ScriptDataImpl::Read(int position) {
  return store_[PreparseDataConstants::kHeaderSize + position];
}


unsigned* ScriptDataImpl::ReadAddress(int position) {
  return &store_[PreparseDataConstants::kHeaderSize + position];
}


Scope* Parser::NewScope(Scope* parent, ScopeType scope_type) {
  Scope* result = new(zone()) Scope(parent, scope_type, zone());
  result->Initialize();
  return result;
}


// ----------------------------------------------------------------------------
// Target is a support class to facilitate manipulation of the
// Parser's target_stack_ (the stack of potential 'break' and
// 'continue' statement targets). Upon construction, a new target is
// added; it is removed upon destruction.

class Target BASE_EMBEDDED {
 public:
  Target(Target** variable, AstNode* node)
      : variable_(variable), node_(node), previous_(*variable) {
    *variable = this;
  }

  ~Target() {
    *variable_ = previous_;
  }

  Target* previous() { return previous_; }
  AstNode* node() { return node_; }

 private:
  Target** variable_;
  AstNode* node_;
  Target* previous_;
};


class TargetScope BASE_EMBEDDED {
 public:
  explicit TargetScope(Target** variable)
      : variable_(variable), previous_(*variable) {
    *variable = NULL;
  }

  ~TargetScope() {
    *variable_ = previous_;
  }

 private:
  Target** variable_;
  Target* previous_;
};


// ----------------------------------------------------------------------------
// The CHECK_OK macro is a convenient macro to enforce error
// handling for functions that may fail (by returning !*ok).
//
// CAUTION: This macro appends extra statements after a call,
// thus it must never be used where only a single statement
// is correct (e.g. an if statement branch w/o braces)!

#define CHECK_OK  ok);   \
  if (!*ok) return NULL; \
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define CHECK_FAILED  /**/);   \
  if (failed_) return NULL; \
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

// ----------------------------------------------------------------------------
// Implementation of Parser

bool ParserTraits::IsEvalOrArguments(Handle<String> identifier) const {
  Factory* factory = parser_->isolate()->factory();
  return identifier.is_identical_to(factory->eval_string())
      || identifier.is_identical_to(factory->arguments_string());
}


bool ParserTraits::IsThisProperty(Expression* expression) {
  ASSERT(expression != NULL);
  Property* property = expression->AsProperty();
  return property != NULL &&
      property->obj()->AsVariableProxy() != NULL &&
      property->obj()->AsVariableProxy()->is_this();
}


bool ParserTraits::IsIdentifier(Expression* expression) {
  VariableProxy* operand = expression->AsVariableProxy();
  return operand != NULL && !operand->is_this();
}


void ParserTraits::PushPropertyName(FuncNameInferrer* fni,
                                    Expression* expression) {
  if (expression->IsPropertyName()) {
    fni->PushLiteralName(expression->AsLiteral()->AsPropertyName());
  } else {
    fni->PushLiteralName(
        parser_->isolate()->factory()->anonymous_function_string());
  }
}


void ParserTraits::CheckAssigningFunctionLiteralToProperty(Expression* left,
                                                           Expression* right) {
  ASSERT(left != NULL);
  if (left->AsProperty() != NULL &&
      right->AsFunctionLiteral() != NULL) {
    right->AsFunctionLiteral()->set_pretenure();
  }
}


void ParserTraits::CheckPossibleEvalCall(Expression* expression,
                                         Scope* scope) {
  VariableProxy* callee = expression->AsVariableProxy();
  if (callee != NULL &&
      callee->IsVariable(parser_->isolate()->factory()->eval_string())) {
    scope->DeclarationScope()->RecordEvalCall();
  }
}


Expression* ParserTraits::MarkExpressionAsLValue(Expression* expression) {
  VariableProxy* proxy = expression != NULL
      ? expression->AsVariableProxy()
      : NULL;
  if (proxy != NULL) proxy->MarkAsLValue();
  return expression;
}


bool ParserTraits::ShortcutNumericLiteralBinaryExpression(
    Expression** x, Expression* y, Token::Value op, int pos,
    AstNodeFactory<AstConstructionVisitor>* factory) {
  if ((*x)->AsLiteral() && (*x)->AsLiteral()->value()->IsNumber() &&
      y->AsLiteral() && y->AsLiteral()->value()->IsNumber()) {
    double x_val = (*x)->AsLiteral()->value()->Number();
    double y_val = y->AsLiteral()->value()->Number();
    switch (op) {
      case Token::ADD:
        *x = factory->NewNumberLiteral(x_val + y_val, pos);
        return true;
      case Token::SUB:
        *x = factory->NewNumberLiteral(x_val - y_val, pos);
        return true;
      case Token::MUL:
        *x = factory->NewNumberLiteral(x_val * y_val, pos);
        return true;
      case Token::DIV:
        *x = factory->NewNumberLiteral(x_val / y_val, pos);
        return true;
      case Token::BIT_OR: {
        int value = DoubleToInt32(x_val) | DoubleToInt32(y_val);
        *x = factory->NewNumberLiteral(value, pos);
        return true;
      }
      case Token::BIT_AND: {
        int value = DoubleToInt32(x_val) & DoubleToInt32(y_val);
        *x = factory->NewNumberLiteral(value, pos);
        return true;
      }
      case Token::BIT_XOR: {
        int value = DoubleToInt32(x_val) ^ DoubleToInt32(y_val);
        *x = factory->NewNumberLiteral(value, pos);
        return true;
      }
      case Token::SHL: {
        int value = DoubleToInt32(x_val) << (DoubleToInt32(y_val) & 0x1f);
        *x = factory->NewNumberLiteral(value, pos);
        return true;
      }
      case Token::SHR: {
        uint32_t shift = DoubleToInt32(y_val) & 0x1f;
        uint32_t value = DoubleToUint32(x_val) >> shift;
        *x = factory->NewNumberLiteral(value, pos);
        return true;
      }
      case Token::SAR: {
        uint32_t shift = DoubleToInt32(y_val) & 0x1f;
        int value = ArithmeticShiftRight(DoubleToInt32(x_val), shift);
        *x = factory->NewNumberLiteral(value, pos);
        return true;
      }
      default:
        break;
    }
  }
  return false;
}


Expression* ParserTraits::BuildUnaryExpression(
    Expression* expression, Token::Value op, int pos,
    AstNodeFactory<AstConstructionVisitor>* factory) {
  ASSERT(expression != NULL);
  if (expression->AsLiteral() != NULL) {
    Handle<Object> literal = expression->AsLiteral()->value();
    if (op == Token::NOT) {
      // Convert the literal to a boolean condition and negate it.
      bool condition = literal->BooleanValue();
      Handle<Object> result =
          parser_->isolate()->factory()->ToBoolean(!condition);
      return factory->NewLiteral(result, pos);
    } else if (literal->IsNumber()) {
      // Compute some expressions involving only number literals.
      double value = literal->Number();
      switch (op) {
        case Token::ADD:
          return expression;
        case Token::SUB:
          return factory->NewNumberLiteral(-value, pos);
        case Token::BIT_NOT:
          return factory->NewNumberLiteral(~DoubleToInt32(value), pos);
        default:
          break;
      }
    }
  }
  // Desugar '+foo' => 'foo*1'
  if (op == Token::ADD) {
    return factory->NewBinaryOperation(
        Token::MUL, expression, factory->NewNumberLiteral(1, pos), pos);
  }
  // The same idea for '-foo' => 'foo*(-1)'.
  if (op == Token::SUB) {
    return factory->NewBinaryOperation(
        Token::MUL, expression, factory->NewNumberLiteral(-1, pos), pos);
  }
  // ...and one more time for '~foo' => 'foo^(~0)'.
  if (op == Token::BIT_NOT) {
    return factory->NewBinaryOperation(
        Token::BIT_XOR, expression, factory->NewNumberLiteral(~0, pos), pos);
  }
  return factory->NewUnaryOperation(op, expression, pos);
}


Expression* ParserTraits::NewThrowReferenceError(
    const char* message, int pos) {
  return NewThrowError(
      parser_->isolate()->factory()->MakeReferenceError_string(),
      message, HandleVector<Object>(NULL, 0), pos);
}


Expression* ParserTraits::NewThrowSyntaxError(
    const char* message, Handle<Object> arg, int pos) {
  int argc = arg.is_null() ? 0 : 1;
  Vector< Handle<Object> > arguments = HandleVector<Object>(&arg, argc);
  return NewThrowError(
      parser_->isolate()->factory()->MakeSyntaxError_string(),
      message, arguments, pos);
}


Expression* ParserTraits::NewThrowTypeError(
    const char* message, Handle<Object> arg1, Handle<Object> arg2, int pos) {
  ASSERT(!arg1.is_null() && !arg2.is_null());
  Handle<Object> elements[] = { arg1, arg2 };
  Vector< Handle<Object> > arguments =
      HandleVector<Object>(elements, ARRAY_SIZE(elements));
  return NewThrowError(
      parser_->isolate()->factory()->MakeTypeError_string(),
      message, arguments, pos);
}


Expression* ParserTraits::NewThrowError(
    Handle<String> constructor, const char* message,
    Vector<Handle<Object> > arguments, int pos) {
  Zone* zone = parser_->zone();
  Factory* factory = parser_->isolate()->factory();
  int argc = arguments.length();
  Handle<FixedArray> elements = factory->NewFixedArray(argc, TENURED);
  for (int i = 0; i < argc; i++) {
    Handle<Object> element = arguments[i];
    if (!element.is_null()) {
      elements->set(i, *element);
    }
  }
  Handle<JSArray> array =
      factory->NewJSArrayWithElements(elements, FAST_ELEMENTS, TENURED);

  ZoneList<Expression*>* args = new(zone) ZoneList<Expression*>(2, zone);
  Handle<String> type = factory->InternalizeUtf8String(message);
  args->Add(parser_->factory()->NewLiteral(type, pos), zone);
  args->Add(parser_->factory()->NewLiteral(array, pos), zone);
  CallRuntime* call_constructor =
      parser_->factory()->NewCallRuntime(constructor, NULL, args, pos);
  return parser_->factory()->NewThrow(call_constructor, pos);
}


void ParserTraits::ReportMessageAt(Scanner::Location source_location,
                                   const char* message,
                                   Vector<const char*> args,
                                   bool is_reference_error) {
  if (parser_->stack_overflow()) {
    // Suppress the error message (syntax error or such) in the presence of a
    // stack overflow. The isolate allows only one pending exception at at time
    // and we want to report the stack overflow later.
    return;
  }
  MessageLocation location(parser_->script_,
                           source_location.beg_pos,
                           source_location.end_pos);
  Factory* factory = parser_->isolate()->factory();
  Handle<FixedArray> elements = factory->NewFixedArray(args.length());
  for (int i = 0; i < args.length(); i++) {
    Handle<String> arg_string = factory->NewStringFromUtf8(CStrVector(args[i]));
    ASSERT(!arg_string.is_null());
    elements->set(i, *arg_string);
  }
  Handle<JSArray> array = factory->NewJSArrayWithElements(elements);
  Handle<Object> result = is_reference_error
      ? factory->NewReferenceError(message, array)
      : factory->NewSyntaxError(message, array);
  parser_->isolate()->Throw(*result, &location);
}


void ParserTraits::ReportMessage(const char* message,
                                 Vector<Handle<String> > args,
                                 bool is_reference_error) {
  Scanner::Location source_location = parser_->scanner()->location();
  ReportMessageAt(source_location, message, args, is_reference_error);
}


void ParserTraits::ReportMessageAt(Scanner::Location source_location,
                                   const char* message,
                                   Vector<Handle<String> > args,
                                   bool is_reference_error) {
  if (parser_->stack_overflow()) {
    // Suppress the error message (syntax error or such) in the presence of a
    // stack overflow. The isolate allows only one pending exception at at time
    // and we want to report the stack overflow later.
    return;
  }
  MessageLocation location(parser_->script_,
                           source_location.beg_pos,
                           source_location.end_pos);
  Factory* factory = parser_->isolate()->factory();
  Handle<FixedArray> elements = factory->NewFixedArray(args.length());
  for (int i = 0; i < args.length(); i++) {
    elements->set(i, *args[i]);
  }
  Handle<JSArray> array = factory->NewJSArrayWithElements(elements);
  Handle<Object> result = is_reference_error
      ? factory->NewReferenceError(message, array)
      : factory->NewSyntaxError(message, array);
  parser_->isolate()->Throw(*result, &location);
}


Handle<String> ParserTraits::GetSymbol(Scanner* scanner) {
  if (parser_->cached_data_mode() == CONSUME_CACHED_DATA) {
    int symbol_id = (*parser_->cached_data())->GetSymbolIdentifier();
    // If there is no symbol data, -1 will be returned.
    if (symbol_id >= 0 &&
        symbol_id < (*parser_->cached_data())->symbol_count()) {
      return parser_->LookupCachedSymbol(symbol_id);
    }
  } else if (parser_->cached_data_mode() == PRODUCE_CACHED_DATA) {
    if (parser_->log_->ShouldLogSymbols()) {
      parser_->scanner()->LogSymbol(parser_->log_, parser_->position());
    }
  }
  Handle<String> result =
      parser_->scanner()->AllocateInternalizedString(parser_->isolate());
  ASSERT(!result.is_null());
  return result;
}


Handle<String> ParserTraits::NextLiteralString(Scanner* scanner,
                                               PretenureFlag tenured) {
  return scanner->AllocateNextLiteralString(parser_->isolate(), tenured);
}


Expression* ParserTraits::ThisExpression(
    Scope* scope,
    AstNodeFactory<AstConstructionVisitor>* factory) {
  return factory->NewVariableProxy(scope->receiver());
}


Literal* ParserTraits::ExpressionFromLiteral(
    Token::Value token, int pos,
    Scanner* scanner,
    AstNodeFactory<AstConstructionVisitor>* factory) {
  Factory* isolate_factory = parser_->isolate()->factory();
  switch (token) {
    case Token::NULL_LITERAL:
      return factory->NewLiteral(isolate_factory->null_value(), pos);
    case Token::TRUE_LITERAL:
      return factory->NewLiteral(isolate_factory->true_value(), pos);
    case Token::FALSE_LITERAL:
      return factory->NewLiteral(isolate_factory->false_value(), pos);
    case Token::NUMBER: {
      double value = scanner->DoubleValue();
      return factory->NewNumberLiteral(value, pos);
    }
    default:
      ASSERT(false);
  }
  return NULL;
}


Expression* ParserTraits::ExpressionFromIdentifier(
    Handle<String> name, int pos, Scope* scope,
    AstNodeFactory<AstConstructionVisitor>* factory) {
  if (parser_->fni_ != NULL) parser_->fni_->PushVariableName(name);
  // The name may refer to a module instance object, so its type is unknown.
#ifdef DEBUG
  if (FLAG_print_interface_details)
    PrintF("# Variable %s ", name->ToAsciiArray());
#endif
  Interface* interface = Interface::NewUnknown(parser_->zone());
  return scope->NewUnresolved(factory, name, interface, pos);
}


Expression* ParserTraits::ExpressionFromString(
    int pos, Scanner* scanner,
    AstNodeFactory<AstConstructionVisitor>* factory) {
  Handle<String> symbol = GetSymbol(scanner);
  if (parser_->fni_ != NULL) parser_->fni_->PushLiteralName(symbol);
  return factory->NewLiteral(symbol, pos);
}


Literal* ParserTraits::GetLiteralTheHole(
    int position, AstNodeFactory<AstConstructionVisitor>* factory) {
  return factory->NewLiteral(parser_->isolate()->factory()->the_hole_value(),
                             RelocInfo::kNoPosition);
}


Expression* ParserTraits::ParseV8Intrinsic(bool* ok) {
  return parser_->ParseV8Intrinsic(ok);
}


FunctionLiteral* ParserTraits::ParseFunctionLiteral(
    Handle<String> name,
    Scanner::Location function_name_location,
    bool name_is_strict_reserved,
    bool is_generator,
    int function_token_position,
    FunctionLiteral::FunctionType type,
    bool* ok) {
  return parser_->ParseFunctionLiteral(name, function_name_location,
                                       name_is_strict_reserved, is_generator,
                                       function_token_position, type, ok);
}


Parser::Parser(CompilationInfo* info)
    : ParserBase<ParserTraits>(&scanner_,
                               info->isolate()->stack_guard()->real_climit(),
                               info->extension(),
                               NULL,
                               info->zone(),
                               this),
      isolate_(info->isolate()),
      symbol_cache_(0, info->zone()),
      script_(info->script()),
      scanner_(isolate_->unicode_cache()),
      reusable_preparser_(NULL),
      original_scope_(NULL),
      target_stack_(NULL),
      cached_data_(NULL),
      cached_data_mode_(NO_CACHED_DATA),
      info_(info) {
  ASSERT(!script_.is_null());
  isolate_->set_ast_node_id(0);
  set_allow_harmony_scoping(!info->is_native() && FLAG_harmony_scoping);
  set_allow_modules(!info->is_native() && FLAG_harmony_modules);
  set_allow_natives_syntax(FLAG_allow_natives_syntax || info->is_native());
  set_allow_lazy(false);  // Must be explicitly enabled.
  set_allow_generators(FLAG_harmony_generators);
  set_allow_for_of(FLAG_harmony_iteration);
  set_allow_harmony_numeric_literals(FLAG_harmony_numeric_literals);
}


FunctionLiteral* Parser::ParseProgram() {
  // TODO(bmeurer): We temporarily need to pass allow_nesting = true here,
  // see comment for HistogramTimerScope class.
  HistogramTimerScope timer_scope(isolate()->counters()->parse(), true);
  Handle<String> source(String::cast(script_->source()));
  isolate()->counters()->total_parse_size()->Increment(source->length());
  ElapsedTimer timer;
  if (FLAG_trace_parse) {
    timer.Start();
  }
  fni_ = new(zone()) FuncNameInferrer(isolate(), zone());

  // Initialize parser state.
  CompleteParserRecorder recorder;
  if (cached_data_mode_ == PRODUCE_CACHED_DATA) {
    log_ = &recorder;
  } else if (cached_data_mode_ == CONSUME_CACHED_DATA) {
    (*cached_data_)->Initialize();
  }

  source->TryFlatten();
  FunctionLiteral* result;
  if (source->IsExternalTwoByteString()) {
    // Notice that the stream is destroyed at the end of the branch block.
    // The last line of the blocks can't be moved outside, even though they're
    // identical calls.
    ExternalTwoByteStringUtf16CharacterStream stream(
        Handle<ExternalTwoByteString>::cast(source), 0, source->length());
    scanner_.Initialize(&stream);
    result = DoParseProgram(info(), source);
  } else {
    GenericStringUtf16CharacterStream stream(source, 0, source->length());
    scanner_.Initialize(&stream);
    result = DoParseProgram(info(), source);
  }

  if (FLAG_trace_parse && result != NULL) {
    double ms = timer.Elapsed().InMillisecondsF();
    if (info()->is_eval()) {
      PrintF("[parsing eval");
    } else if (info()->script()->name()->IsString()) {
      String* name = String::cast(info()->script()->name());
      SmartArrayPointer<char> name_chars = name->ToCString();
      PrintF("[parsing script: %s", name_chars.get());
    } else {
      PrintF("[parsing script");
    }
    PrintF(" - took %0.3f ms]\n", ms);
  }
  if (cached_data_mode_ == PRODUCE_CACHED_DATA) {
    Vector<unsigned> store = recorder.ExtractData();
    *cached_data_ = new ScriptDataImpl(store);
    log_ = NULL;
  }
  return result;
}


FunctionLiteral* Parser::DoParseProgram(CompilationInfo* info,
                                        Handle<String> source) {
  ASSERT(scope_ == NULL);
  ASSERT(target_stack_ == NULL);

  Handle<String> no_name = isolate()->factory()->empty_string();

  FunctionLiteral* result = NULL;
  { Scope* scope = NewScope(scope_, GLOBAL_SCOPE);
    info->SetGlobalScope(scope);
    if (!info->context().is_null()) {
      scope = Scope::DeserializeScopeChain(*info->context(), scope, zone());
    }
    original_scope_ = scope;
    if (info->is_eval()) {
      if (!scope->is_global_scope() || info->strict_mode() == STRICT) {
        scope = NewScope(scope, EVAL_SCOPE);
      }
    } else if (info->is_global()) {
      scope = NewScope(scope, GLOBAL_SCOPE);
    }
    scope->set_start_position(0);
    scope->set_end_position(source->length());

    // Compute the parsing mode.
    Mode mode = (FLAG_lazy && allow_lazy()) ? PARSE_LAZILY : PARSE_EAGERLY;
    if (allow_natives_syntax() ||
        extension_ != NULL ||
        scope->is_eval_scope()) {
      mode = PARSE_EAGERLY;
    }
    ParsingModeScope parsing_mode(this, mode);

    // Enters 'scope'.
    FunctionState function_state(&function_state_, &scope_, scope, zone());

    scope_->SetStrictMode(info->strict_mode());
    ZoneList<Statement*>* body = new(zone()) ZoneList<Statement*>(16, zone());
    bool ok = true;
    int beg_pos = scanner()->location().beg_pos;
    ParseSourceElements(body, Token::EOS, info->is_eval(), true, &ok);
    if (ok && strict_mode() == STRICT) {
      CheckOctalLiteral(beg_pos, scanner()->location().end_pos, &ok);
    }

    if (ok && allow_harmony_scoping() && strict_mode() == STRICT) {
      CheckConflictingVarDeclarations(scope_, &ok);
    }

    if (ok && info->parse_restriction() == ONLY_SINGLE_FUNCTION_LITERAL) {
      if (body->length() != 1 ||
          !body->at(0)->IsExpressionStatement() ||
          !body->at(0)->AsExpressionStatement()->
              expression()->IsFunctionLiteral()) {
        ReportMessage("single_function_literal", Vector<const char*>::empty());
        ok = false;
      }
    }

    if (ok) {
      result = factory()->NewFunctionLiteral(
          no_name,
          scope_,
          body,
          function_state.materialized_literal_count(),
          function_state.expected_property_count(),
          function_state.handler_count(),
          0,
          FunctionLiteral::kNoDuplicateParameters,
          FunctionLiteral::ANONYMOUS_EXPRESSION,
          FunctionLiteral::kGlobalOrEval,
          FunctionLiteral::kNotParenthesized,
          FunctionLiteral::kNotGenerator,
          0);
      result->set_ast_properties(factory()->visitor()->ast_properties());
      result->set_slot_processor(factory()->visitor()->slot_processor());
      result->set_dont_optimize_reason(
          factory()->visitor()->dont_optimize_reason());
    } else if (stack_overflow()) {
      isolate()->StackOverflow();
    }
  }

  // Make sure the target stack is empty.
  ASSERT(target_stack_ == NULL);

  return result;
}


FunctionLiteral* Parser::ParseLazy() {
  HistogramTimerScope timer_scope(isolate()->counters()->parse_lazy());
  Handle<String> source(String::cast(script_->source()));
  isolate()->counters()->total_parse_size()->Increment(source->length());
  ElapsedTimer timer;
  if (FLAG_trace_parse) {
    timer.Start();
  }
  Handle<SharedFunctionInfo> shared_info = info()->shared_info();

  // Initialize parser state.
  source->TryFlatten();
  FunctionLiteral* result;
  if (source->IsExternalTwoByteString()) {
    ExternalTwoByteStringUtf16CharacterStream stream(
        Handle<ExternalTwoByteString>::cast(source),
        shared_info->start_position(),
        shared_info->end_position());
    result = ParseLazy(&stream);
  } else {
    GenericStringUtf16CharacterStream stream(source,
                                             shared_info->start_position(),
                                             shared_info->end_position());
    result = ParseLazy(&stream);
  }

  if (FLAG_trace_parse && result != NULL) {
    double ms = timer.Elapsed().InMillisecondsF();
    SmartArrayPointer<char> name_chars = result->debug_name()->ToCString();
    PrintF("[parsing function: %s - took %0.3f ms]\n", name_chars.get(), ms);
  }
  return result;
}


FunctionLiteral* Parser::ParseLazy(Utf16CharacterStream* source) {
  Handle<SharedFunctionInfo> shared_info = info()->shared_info();
  scanner_.Initialize(source);
  ASSERT(scope_ == NULL);
  ASSERT(target_stack_ == NULL);

  Handle<String> name(String::cast(shared_info->name()));
  fni_ = new(zone()) FuncNameInferrer(isolate(), zone());
  fni_->PushEnclosingName(name);

  ParsingModeScope parsing_mode(this, PARSE_EAGERLY);

  // Place holder for the result.
  FunctionLiteral* result = NULL;

  {
    // Parse the function literal.
    Scope* scope = NewScope(scope_, GLOBAL_SCOPE);
    info()->SetGlobalScope(scope);
    if (!info()->closure().is_null()) {
      scope = Scope::DeserializeScopeChain(info()->closure()->context(), scope,
                                           zone());
    }
    original_scope_ = scope;
    FunctionState function_state(&function_state_, &scope_, scope, zone());
    ASSERT(scope->strict_mode() == SLOPPY || info()->strict_mode() == STRICT);
    ASSERT(info()->strict_mode() == shared_info->strict_mode());
    scope->SetStrictMode(shared_info->strict_mode());
    FunctionLiteral::FunctionType function_type = shared_info->is_expression()
        ? (shared_info->is_anonymous()
              ? FunctionLiteral::ANONYMOUS_EXPRESSION
              : FunctionLiteral::NAMED_EXPRESSION)
        : FunctionLiteral::DECLARATION;
    bool ok = true;
    result = ParseFunctionLiteral(name,
                                  Scanner::Location::invalid(),
                                  false,  // Strict mode name already checked.
                                  shared_info->is_generator(),
                                  RelocInfo::kNoPosition,
                                  function_type,
                                  &ok);
    // Make sure the results agree.
    ASSERT(ok == (result != NULL));
  }

  // Make sure the target stack is empty.
  ASSERT(target_stack_ == NULL);

  if (result == NULL) {
    if (stack_overflow()) isolate()->StackOverflow();
  } else {
    Handle<String> inferred_name(shared_info->inferred_name());
    result->set_inferred_name(inferred_name);
  }
  return result;
}


void* Parser::ParseSourceElements(ZoneList<Statement*>* processor,
                                  int end_token,
                                  bool is_eval,
                                  bool is_global,
                                  bool* ok) {
  // SourceElements ::
  //   (ModuleElement)* <end_token>

  // Allocate a target stack to use for this set of source
  // elements. This way, all scripts and functions get their own
  // target stack thus avoiding illegal breaks and continues across
  // functions.
  TargetScope scope(&this->target_stack_);

  ASSERT(processor != NULL);
  bool directive_prologue = true;     // Parsing directive prologue.

  while (peek() != end_token) {
    if (directive_prologue && peek() != Token::STRING) {
      directive_prologue = false;
    }

    Scanner::Location token_loc = scanner()->peek_location();
    Statement* stat;
    if (is_global && !is_eval) {
      stat = ParseModuleElement(NULL, CHECK_OK);
    } else {
      stat = ParseBlockElement(NULL, CHECK_OK);
    }
    if (stat == NULL || stat->IsEmpty()) {
      directive_prologue = false;   // End of directive prologue.
      continue;
    }

    if (directive_prologue) {
      // A shot at a directive.
      ExpressionStatement* e_stat;
      Literal* literal;
      // Still processing directive prologue?
      if ((e_stat = stat->AsExpressionStatement()) != NULL &&
          (literal = e_stat->expression()->AsLiteral()) != NULL &&
          literal->value()->IsString()) {
        Handle<String> directive = Handle<String>::cast(literal->value());

        // Check "use strict" directive (ES5 14.1).
        if (strict_mode() == SLOPPY &&
            directive->Equals(isolate()->heap()->use_strict_string()) &&
            token_loc.end_pos - token_loc.beg_pos ==
              isolate()->heap()->use_strict_string()->length() + 2) {
          // TODO(mstarzinger): Global strict eval calls, need their own scope
          // as specified in ES5 10.4.2(3). The correct fix would be to always
          // add this scope in DoParseProgram(), but that requires adaptations
          // all over the code base, so we go with a quick-fix for now.
          // In the same manner, we have to patch the parsing mode.
          if (is_eval && !scope_->is_eval_scope()) {
            ASSERT(scope_->is_global_scope());
            Scope* scope = NewScope(scope_, EVAL_SCOPE);
            scope->set_start_position(scope_->start_position());
            scope->set_end_position(scope_->end_position());
            scope_ = scope;
            mode_ = PARSE_EAGERLY;
          }
          scope_->SetStrictMode(STRICT);
          // "use strict" is the only directive for now.
          directive_prologue = false;
        }
      } else {
        // End of the directive prologue.
        directive_prologue = false;
      }
    }

    processor->Add(stat, zone());
  }

  return 0;
}


Statement* Parser::ParseModuleElement(ZoneStringList* labels,
                                      bool* ok) {
  // (Ecma 262 5th Edition, clause 14):
  // SourceElement:
  //    Statement
  //    FunctionDeclaration
  //
  // In harmony mode we allow additionally the following productions
  // ModuleElement:
  //    LetDeclaration
  //    ConstDeclaration
  //    ModuleDeclaration
  //    ImportDeclaration
  //    ExportDeclaration
  //    GeneratorDeclaration

  switch (peek()) {
    case Token::FUNCTION:
      return ParseFunctionDeclaration(NULL, ok);
    case Token::LET:
    case Token::CONST:
      return ParseVariableStatement(kModuleElement, NULL, ok);
    case Token::IMPORT:
      return ParseImportDeclaration(ok);
    case Token::EXPORT:
      return ParseExportDeclaration(ok);
    default: {
      Statement* stmt = ParseStatement(labels, CHECK_OK);
      // Handle 'module' as a context-sensitive keyword.
      if (FLAG_harmony_modules &&
          peek() == Token::IDENTIFIER &&
          !scanner()->HasAnyLineTerminatorBeforeNext() &&
          stmt != NULL) {
        ExpressionStatement* estmt = stmt->AsExpressionStatement();
        if (estmt != NULL &&
            estmt->expression()->AsVariableProxy() != NULL &&
            estmt->expression()->AsVariableProxy()->name()->Equals(
                isolate()->heap()->module_string()) &&
            !scanner()->literal_contains_escapes()) {
          return ParseModuleDeclaration(NULL, ok);
        }
      }
      return stmt;
    }
  }
}


Statement* Parser::ParseModuleDeclaration(ZoneStringList* names, bool* ok) {
  // ModuleDeclaration:
  //    'module' Identifier Module

  int pos = peek_position();
  Handle<String> name = ParseIdentifier(kDontAllowEvalOrArguments, CHECK_OK);

#ifdef DEBUG
  if (FLAG_print_interface_details)
    PrintF("# Module %s...\n", name->ToAsciiArray());
#endif

  Module* module = ParseModule(CHECK_OK);
  VariableProxy* proxy = NewUnresolved(name, MODULE, module->interface());
  Declaration* declaration =
      factory()->NewModuleDeclaration(proxy, module, scope_, pos);
  Declare(declaration, true, CHECK_OK);

#ifdef DEBUG
  if (FLAG_print_interface_details)
    PrintF("# Module %s.\n", name->ToAsciiArray());

  if (FLAG_print_interfaces) {
    PrintF("module %s : ", name->ToAsciiArray());
    module->interface()->Print();
  }
#endif

  if (names) names->Add(name, zone());
  if (module->body() == NULL)
    return factory()->NewEmptyStatement(pos);
  else
    return factory()->NewModuleStatement(proxy, module->body(), pos);
}


Module* Parser::ParseModule(bool* ok) {
  // Module:
  //    '{' ModuleElement '}'
  //    '=' ModulePath ';'
  //    'at' String ';'

  switch (peek()) {
    case Token::LBRACE:
      return ParseModuleLiteral(ok);

    case Token::ASSIGN: {
      Expect(Token::ASSIGN, CHECK_OK);
      Module* result = ParseModulePath(CHECK_OK);
      ExpectSemicolon(CHECK_OK);
      return result;
    }

    default: {
      ExpectContextualKeyword(CStrVector("at"), CHECK_OK);
      Module* result = ParseModuleUrl(CHECK_OK);
      ExpectSemicolon(CHECK_OK);
      return result;
    }
  }
}


Module* Parser::ParseModuleLiteral(bool* ok) {
  // Module:
  //    '{' ModuleElement '}'

  int pos = peek_position();
  // Construct block expecting 16 statements.
  Block* body = factory()->NewBlock(NULL, 16, false, RelocInfo::kNoPosition);
#ifdef DEBUG
  if (FLAG_print_interface_details) PrintF("# Literal ");
#endif
  Scope* scope = NewScope(scope_, MODULE_SCOPE);

  Expect(Token::LBRACE, CHECK_OK);
  scope->set_start_position(scanner()->location().beg_pos);
  scope->SetStrictMode(STRICT);

  {
    BlockState block_state(&scope_, scope);
    TargetCollector collector(zone());
    Target target(&this->target_stack_, &collector);
    Target target_body(&this->target_stack_, body);

    while (peek() != Token::RBRACE) {
      Statement* stat = ParseModuleElement(NULL, CHECK_OK);
      if (stat && !stat->IsEmpty()) {
        body->AddStatement(stat, zone());
      }
    }
  }

  Expect(Token::RBRACE, CHECK_OK);
  scope->set_end_position(scanner()->location().end_pos);
  body->set_scope(scope);

  // Check that all exports are bound.
  Interface* interface = scope->interface();
  for (Interface::Iterator it = interface->iterator();
       !it.done(); it.Advance()) {
    if (scope->LocalLookup(it.name()) == NULL) {
      Handle<String> name(it.name());
      ParserTraits::ReportMessage("module_export_undefined",
                                  Vector<Handle<String> >(&name, 1));
      *ok = false;
      return NULL;
    }
  }

  interface->MakeModule(ok);
  ASSERT(*ok);
  interface->Freeze(ok);
  ASSERT(*ok);
  return factory()->NewModuleLiteral(body, interface, pos);
}


Module* Parser::ParseModulePath(bool* ok) {
  // ModulePath:
  //    Identifier
  //    ModulePath '.' Identifier

  int pos = peek_position();
  Module* result = ParseModuleVariable(CHECK_OK);
  while (Check(Token::PERIOD)) {
    Handle<String> name = ParseIdentifierName(CHECK_OK);
#ifdef DEBUG
    if (FLAG_print_interface_details)
      PrintF("# Path .%s ", name->ToAsciiArray());
#endif
    Module* member = factory()->NewModulePath(result, name, pos);
    result->interface()->Add(name, member->interface(), zone(), ok);
    if (!*ok) {
#ifdef DEBUG
      if (FLAG_print_interfaces) {
        PrintF("PATH TYPE ERROR at '%s'\n", name->ToAsciiArray());
        PrintF("result: ");
        result->interface()->Print();
        PrintF("member: ");
        member->interface()->Print();
      }
#endif
      ParserTraits::ReportMessage("invalid_module_path",
                                  Vector<Handle<String> >(&name, 1));
      return NULL;
    }
    result = member;
  }

  return result;
}


Module* Parser::ParseModuleVariable(bool* ok) {
  // ModulePath:
  //    Identifier

  int pos = peek_position();
  Handle<String> name = ParseIdentifier(kDontAllowEvalOrArguments, CHECK_OK);
#ifdef DEBUG
  if (FLAG_print_interface_details)
    PrintF("# Module variable %s ", name->ToAsciiArray());
#endif
  VariableProxy* proxy = scope_->NewUnresolved(
      factory(), name, Interface::NewModule(zone()),
      scanner()->location().beg_pos);

  return factory()->NewModuleVariable(proxy, pos);
}


Module* Parser::ParseModuleUrl(bool* ok) {
  // Module:
  //    String

  int pos = peek_position();
  Expect(Token::STRING, CHECK_OK);
  Handle<String> symbol = GetSymbol();

  // TODO(ES6): Request JS resource from environment...

#ifdef DEBUG
  if (FLAG_print_interface_details) PrintF("# Url ");
#endif

  // Create an empty literal as long as the feature isn't finished.
  USE(symbol);
  Scope* scope = NewScope(scope_, MODULE_SCOPE);
  Block* body = factory()->NewBlock(NULL, 1, false, RelocInfo::kNoPosition);
  body->set_scope(scope);
  Interface* interface = scope->interface();
  Module* result = factory()->NewModuleLiteral(body, interface, pos);
  interface->Freeze(ok);
  ASSERT(*ok);
  interface->Unify(scope->interface(), zone(), ok);
  ASSERT(*ok);
  return result;
}


Module* Parser::ParseModuleSpecifier(bool* ok) {
  // ModuleSpecifier:
  //    String
  //    ModulePath

  if (peek() == Token::STRING) {
    return ParseModuleUrl(ok);
  } else {
    return ParseModulePath(ok);
  }
}


Block* Parser::ParseImportDeclaration(bool* ok) {
  // ImportDeclaration:
  //    'import' IdentifierName (',' IdentifierName)* 'from' ModuleSpecifier ';'
  //
  // TODO(ES6): implement destructuring ImportSpecifiers

  int pos = peek_position();
  Expect(Token::IMPORT, CHECK_OK);
  ZoneStringList names(1, zone());

  Handle<String> name = ParseIdentifierName(CHECK_OK);
  names.Add(name, zone());
  while (peek() == Token::COMMA) {
    Consume(Token::COMMA);
    name = ParseIdentifierName(CHECK_OK);
    names.Add(name, zone());
  }

  ExpectContextualKeyword(CStrVector("from"), CHECK_OK);
  Module* module = ParseModuleSpecifier(CHECK_OK);
  ExpectSemicolon(CHECK_OK);

  // Generate a separate declaration for each identifier.
  // TODO(ES6): once we implement destructuring, make that one declaration.
  Block* block = factory()->NewBlock(NULL, 1, true, RelocInfo::kNoPosition);
  for (int i = 0; i < names.length(); ++i) {
#ifdef DEBUG
    if (FLAG_print_interface_details)
      PrintF("# Import %s ", names[i]->ToAsciiArray());
#endif
    Interface* interface = Interface::NewUnknown(zone());
    module->interface()->Add(names[i], interface, zone(), ok);
    if (!*ok) {
#ifdef DEBUG
      if (FLAG_print_interfaces) {
        PrintF("IMPORT TYPE ERROR at '%s'\n", names[i]->ToAsciiArray());
        PrintF("module: ");
        module->interface()->Print();
      }
#endif
      ParserTraits::ReportMessage("invalid_module_path",
                                  Vector<Handle<String> >(&name, 1));
      return NULL;
    }
    VariableProxy* proxy = NewUnresolved(names[i], LET, interface);
    Declaration* declaration =
        factory()->NewImportDeclaration(proxy, module, scope_, pos);
    Declare(declaration, true, CHECK_OK);
  }

  return block;
}


Statement* Parser::ParseExportDeclaration(bool* ok) {
  // ExportDeclaration:
  //    'export' Identifier (',' Identifier)* ';'
  //    'export' VariableDeclaration
  //    'export' FunctionDeclaration
  //    'export' GeneratorDeclaration
  //    'export' ModuleDeclaration
  //
  // TODO(ES6): implement structuring ExportSpecifiers

  Expect(Token::EXPORT, CHECK_OK);

  Statement* result = NULL;
  ZoneStringList names(1, zone());
  switch (peek()) {
    case Token::IDENTIFIER: {
      int pos = position();
      Handle<String> name =
          ParseIdentifier(kDontAllowEvalOrArguments, CHECK_OK);
      // Handle 'module' as a context-sensitive keyword.
      if (!name->IsOneByteEqualTo(STATIC_ASCII_VECTOR("module"))) {
        names.Add(name, zone());
        while (peek() == Token::COMMA) {
          Consume(Token::COMMA);
          name = ParseIdentifier(kDontAllowEvalOrArguments, CHECK_OK);
          names.Add(name, zone());
        }
        ExpectSemicolon(CHECK_OK);
        result = factory()->NewEmptyStatement(pos);
      } else {
        result = ParseModuleDeclaration(&names, CHECK_OK);
      }
      break;
    }

    case Token::FUNCTION:
      result = ParseFunctionDeclaration(&names, CHECK_OK);
      break;

    case Token::VAR:
    case Token::LET:
    case Token::CONST:
      result = ParseVariableStatement(kModuleElement, &names, CHECK_OK);
      break;

    default:
      *ok = false;
      ReportUnexpectedToken(scanner()->current_token());
      return NULL;
  }

  // Extract declared names into export declarations and interface.
  Interface* interface = scope_->interface();
  for (int i = 0; i < names.length(); ++i) {
#ifdef DEBUG
    if (FLAG_print_interface_details)
      PrintF("# Export %s ", names[i]->ToAsciiArray());
#endif
    Interface* inner = Interface::NewUnknown(zone());
    interface->Add(names[i], inner, zone(), CHECK_OK);
    if (!*ok)
      return NULL;
    VariableProxy* proxy = NewUnresolved(names[i], LET, inner);
    USE(proxy);
    // TODO(rossberg): Rethink whether we actually need to store export
    // declarations (for compilation?).
    // ExportDeclaration* declaration =
    //     factory()->NewExportDeclaration(proxy, scope_, position);
    // scope_->AddDeclaration(declaration);
  }

  ASSERT(result != NULL);
  return result;
}


Statement* Parser::ParseBlockElement(ZoneStringList* labels,
                                     bool* ok) {
  // (Ecma 262 5th Edition, clause 14):
  // SourceElement:
  //    Statement
  //    FunctionDeclaration
  //
  // In harmony mode we allow additionally the following productions
  // BlockElement (aka SourceElement):
  //    LetDeclaration
  //    ConstDeclaration
  //    GeneratorDeclaration

  switch (peek()) {
    case Token::FUNCTION:
      return ParseFunctionDeclaration(NULL, ok);
    case Token::LET:
    case Token::CONST:
      return ParseVariableStatement(kModuleElement, NULL, ok);
    default:
      return ParseStatement(labels, ok);
  }
}


Statement* Parser::ParseStatement(ZoneStringList* labels, bool* ok) {
  // Statement ::
  //   Block
  //   VariableStatement
  //   EmptyStatement
  //   ExpressionStatement
  //   IfStatement
  //   IterationStatement
  //   ContinueStatement
  //   BreakStatement
  //   ReturnStatement
  //   WithStatement
  //   LabelledStatement
  //   SwitchStatement
  //   ThrowStatement
  //   TryStatement
  //   DebuggerStatement

  // Note: Since labels can only be used by 'break' and 'continue'
  // statements, which themselves are only valid within blocks,
  // iterations or 'switch' statements (i.e., BreakableStatements),
  // labels can be simply ignored in all other cases; except for
  // trivial labeled break statements 'label: break label' which is
  // parsed into an empty statement.
  switch (peek()) {
    case Token::LBRACE:
      return ParseBlock(labels, ok);

    case Token::CONST:  // fall through
    case Token::LET:
    case Token::VAR:
      return ParseVariableStatement(kStatement, NULL, ok);

    case Token::SEMICOLON:
      Next();
      return factory()->NewEmptyStatement(RelocInfo::kNoPosition);

    case Token::IF:
      return ParseIfStatement(labels, ok);

    case Token::DO:
      return ParseDoWhileStatement(labels, ok);

    case Token::WHILE:
      return ParseWhileStatement(labels, ok);

    case Token::FOR:
      return ParseForStatement(labels, ok);

    case Token::CONTINUE:
      return ParseContinueStatement(ok);

    case Token::BREAK:
      return ParseBreakStatement(labels, ok);

    case Token::RETURN:
      return ParseReturnStatement(ok);

    case Token::WITH:
      return ParseWithStatement(labels, ok);

    case Token::SWITCH:
      return ParseSwitchStatement(labels, ok);

    case Token::THROW:
      return ParseThrowStatement(ok);

    case Token::TRY: {
      // NOTE: It is somewhat complicated to have labels on
      // try-statements. When breaking out of a try-finally statement,
      // one must take great care not to treat it as a
      // fall-through. It is much easier just to wrap the entire
      // try-statement in a statement block and put the labels there
      Block* result =
          factory()->NewBlock(labels, 1, false, RelocInfo::kNoPosition);
      Target target(&this->target_stack_, result);
      TryStatement* statement = ParseTryStatement(CHECK_OK);
      if (result) result->AddStatement(statement, zone());
      return result;
    }

    case Token::FUNCTION: {
      // FunctionDeclaration is only allowed in the context of SourceElements
      // (Ecma 262 5th Edition, clause 14):
      // SourceElement:
      //    Statement
      //    FunctionDeclaration
      // Common language extension is to allow function declaration in place
      // of any statement. This language extension is disabled in strict mode.
      //
      // In Harmony mode, this case also handles the extension:
      // Statement:
      //    GeneratorDeclaration
      if (strict_mode() == STRICT) {
        ReportMessageAt(scanner()->peek_location(), "strict_function");
        *ok = false;
        return NULL;
      }
      return ParseFunctionDeclaration(NULL, ok);
    }

    case Token::DEBUGGER:
      return ParseDebuggerStatement(ok);

    default:
      return ParseExpressionOrLabelledStatement(labels, ok);
  }
}


VariableProxy* Parser::NewUnresolved(
    Handle<String> name, VariableMode mode, Interface* interface) {
  // If we are inside a function, a declaration of a var/const variable is a
  // truly local variable, and the scope of the variable is always the function
  // scope.
  // Let/const variables in harmony mode are always added to the immediately
  // enclosing scope.
  return DeclarationScope(mode)->NewUnresolved(
      factory(), name, interface, position());
}


void Parser::Declare(Declaration* declaration, bool resolve, bool* ok) {
  VariableProxy* proxy = declaration->proxy();
  Handle<String> name = proxy->name();
  VariableMode mode = declaration->mode();
  Scope* declaration_scope = DeclarationScope(mode);
  Variable* var = NULL;

  // If a suitable scope exists, then we can statically declare this
  // variable and also set its mode. In any case, a Declaration node
  // will be added to the scope so that the declaration can be added
  // to the corresponding activation frame at runtime if necessary.
  // For instance declarations inside an eval scope need to be added
  // to the calling function context.
  // Similarly, strict mode eval scope does not leak variable declarations to
  // the caller's scope so we declare all locals, too.
  if (declaration_scope->is_function_scope() ||
      declaration_scope->is_strict_eval_scope() ||
      declaration_scope->is_block_scope() ||
      declaration_scope->is_module_scope() ||
      declaration_scope->is_global_scope()) {
    // Declare the variable in the declaration scope.
    // For the global scope, we have to check for collisions with earlier
    // (i.e., enclosing) global scopes, to maintain the illusion of a single
    // global scope.
    var = declaration_scope->is_global_scope()
        ? declaration_scope->Lookup(name)
        : declaration_scope->LocalLookup(name);
    if (var == NULL) {
      // Declare the name.
      var = declaration_scope->DeclareLocal(
          name, mode, declaration->initialization(), proxy->interface());
    } else if ((mode != VAR || var->mode() != VAR) &&
               (!declaration_scope->is_global_scope() ||
                IsLexicalVariableMode(mode) ||
                IsLexicalVariableMode(var->mode()))) {
      // The name was declared in this scope before; check for conflicting
      // re-declarations. We have a conflict if either of the declarations is
      // not a var (in the global scope, we also have to ignore legacy const for
      // compatibility). There is similar code in runtime.cc in the Declare
      // functions. The function CheckNonConflictingScope checks for conflicting
      // var and let bindings from different scopes whereas this is a check for
      // conflicting declarations within the same scope. This check also covers
      // the special case
      //
      // function () { let x; { var x; } }
      //
      // because the var declaration is hoisted to the function scope where 'x'
      // is already bound.
      ASSERT(IsDeclaredVariableMode(var->mode()));
      if (allow_harmony_scoping() && strict_mode() == STRICT) {
        // In harmony we treat re-declarations as early errors. See
        // ES5 16 for a definition of early errors.
        SmartArrayPointer<char> c_string = name->ToCString(DISALLOW_NULLS);
        const char* elms[2] = { "Variable", c_string.get() };
        Vector<const char*> args(elms, 2);
        ReportMessage("redeclaration", args);
        *ok = false;
        return;
      }
      Handle<String> message_string =
          isolate()->factory()->InternalizeOneByteString(
              STATIC_ASCII_VECTOR("Variable"));
      Expression* expression =
          NewThrowTypeError("redeclaration",
                            message_string, name, declaration->position());
      declaration_scope->SetIllegalRedeclaration(expression);
    }
  }

  // We add a declaration node for every declaration. The compiler
  // will only generate code if necessary. In particular, declarations
  // for inner local variables that do not represent functions won't
  // result in any generated code.
  //
  // Note that we always add an unresolved proxy even if it's not
  // used, simply because we don't know in this method (w/o extra
  // parameters) if the proxy is needed or not. The proxy will be
  // bound during variable resolution time unless it was pre-bound
  // below.
  //
  // WARNING: This will lead to multiple declaration nodes for the
  // same variable if it is declared several times. This is not a
  // semantic issue as long as we keep the source order, but it may be
  // a performance issue since it may lead to repeated
  // RuntimeHidden_DeclareContextSlot calls.
  declaration_scope->AddDeclaration(declaration);

  if (mode == CONST_LEGACY && declaration_scope->is_global_scope()) {
    // For global const variables we bind the proxy to a variable.
    ASSERT(resolve);  // should be set by all callers
    Variable::Kind kind = Variable::NORMAL;
    var = new(zone()) Variable(
        declaration_scope, name, mode, true, kind,
        kNeedsInitialization, proxy->interface());
  } else if (declaration_scope->is_eval_scope() &&
             declaration_scope->strict_mode() == SLOPPY) {
    // For variable declarations in a sloppy eval scope the proxy is bound
    // to a lookup variable to force a dynamic declaration using the
    // DeclareContextSlot runtime function.
    Variable::Kind kind = Variable::NORMAL;
    var = new(zone()) Variable(
        declaration_scope, name, mode, true, kind,
        declaration->initialization(), proxy->interface());
    var->AllocateTo(Variable::LOOKUP, -1);
    resolve = true;
  }

  // If requested and we have a local variable, bind the proxy to the variable
  // at parse-time. This is used for functions (and consts) declared inside
  // statements: the corresponding function (or const) variable must be in the
  // function scope and not a statement-local scope, e.g. as provided with a
  // 'with' statement:
  //
  //   with (obj) {
  //     function f() {}
  //   }
  //
  // which is translated into:
  //
  //   with (obj) {
  //     // in this case this is not: 'var f; f = function () {};'
  //     var f = function () {};
  //   }
  //
  // Note that if 'f' is accessed from inside the 'with' statement, it
  // will be allocated in the context (because we must be able to look
  // it up dynamically) but it will also be accessed statically, i.e.,
  // with a context slot index and a context chain length for this
  // initialization code. Thus, inside the 'with' statement, we need
  // both access to the static and the dynamic context chain; the
  // runtime needs to provide both.
  if (resolve && var != NULL) {
    proxy->BindTo(var);

    if (FLAG_harmony_modules) {
      bool ok;
#ifdef DEBUG
      if (FLAG_print_interface_details)
        PrintF("# Declare %s\n", var->name()->ToAsciiArray());
#endif
      proxy->interface()->Unify(var->interface(), zone(), &ok);
      if (!ok) {
#ifdef DEBUG
        if (FLAG_print_interfaces) {
          PrintF("DECLARE TYPE ERROR\n");
          PrintF("proxy: ");
          proxy->interface()->Print();
          PrintF("var: ");
          var->interface()->Print();
        }
#endif
        ParserTraits::ReportMessage("module_type_error",
                                    Vector<Handle<String> >(&name, 1));
      }
    }
  }
}


// Language extension which is only enabled for source files loaded
// through the API's extension mechanism.  A native function
// declaration is resolved by looking up the function through a
// callback provided by the extension.
Statement* Parser::ParseNativeDeclaration(bool* ok) {
  int pos = peek_position();
  Expect(Token::FUNCTION, CHECK_OK);
  // Allow "eval" or "arguments" for backward compatibility.
  Handle<String> name = ParseIdentifier(kAllowEvalOrArguments, CHECK_OK);
  Expect(Token::LPAREN, CHECK_OK);
  bool done = (peek() == Token::RPAREN);
  while (!done) {
    ParseIdentifier(kAllowEvalOrArguments, CHECK_OK);
    done = (peek() == Token::RPAREN);
    if (!done) {
      Expect(Token::COMMA, CHECK_OK);
    }
  }
  Expect(Token::RPAREN, CHECK_OK);
  Expect(Token::SEMICOLON, CHECK_OK);

  // Make sure that the function containing the native declaration
  // isn't lazily compiled. The extension structures are only
  // accessible while parsing the first time not when reparsing
  // because of lazy compilation.
  DeclarationScope(VAR)->ForceEagerCompilation();

  // TODO(1240846): It's weird that native function declarations are
  // introduced dynamically when we meet their declarations, whereas
  // other functions are set up when entering the surrounding scope.
  VariableProxy* proxy = NewUnresolved(name, VAR, Interface::NewValue());
  Declaration* declaration =
      factory()->NewVariableDeclaration(proxy, VAR, scope_, pos);
  Declare(declaration, true, CHECK_OK);
  NativeFunctionLiteral* lit = factory()->NewNativeFunctionLiteral(
      name, extension_, RelocInfo::kNoPosition);
  return factory()->NewExpressionStatement(
      factory()->NewAssignment(
          Token::INIT_VAR, proxy, lit, RelocInfo::kNoPosition),
      pos);
}


Statement* Parser::ParseFunctionDeclaration(ZoneStringList* names, bool* ok) {
  // FunctionDeclaration ::
  //   'function' Identifier '(' FormalParameterListopt ')' '{' FunctionBody '}'
  // GeneratorDeclaration ::
  //   'function' '*' Identifier '(' FormalParameterListopt ')'
  //      '{' FunctionBody '}'
  Expect(Token::FUNCTION, CHECK_OK);
  int pos = position();
  bool is_generator = allow_generators() && Check(Token::MUL);
  bool is_strict_reserved = false;
  Handle<String> name = ParseIdentifierOrStrictReservedWord(
      &is_strict_reserved, CHECK_OK);
  FunctionLiteral* fun = ParseFunctionLiteral(name,
                                              scanner()->location(),
                                              is_strict_reserved,
                                              is_generator,
                                              pos,
                                              FunctionLiteral::DECLARATION,
                                              CHECK_OK);
  // Even if we're not at the top-level of the global or a function
  // scope, we treat it as such and introduce the function with its
  // initial value upon entering the corresponding scope.
  // In extended mode, a function behaves as a lexical binding, except in the
  // global scope.
  VariableMode mode =
      allow_harmony_scoping() &&
      strict_mode() == STRICT && !scope_->is_global_scope() ? LET : VAR;
  VariableProxy* proxy = NewUnresolved(name, mode, Interface::NewValue());
  Declaration* declaration =
      factory()->NewFunctionDeclaration(proxy, mode, fun, scope_, pos);
  Declare(declaration, true, CHECK_OK);
  if (names) names->Add(name, zone());
  return factory()->NewEmptyStatement(RelocInfo::kNoPosition);
}


Block* Parser::ParseBlock(ZoneStringList* labels, bool* ok) {
  if (allow_harmony_scoping() && strict_mode() == STRICT) {
    return ParseScopedBlock(labels, ok);
  }

  // Block ::
  //   '{' Statement* '}'

  // Note that a Block does not introduce a new execution scope!
  // (ECMA-262, 3rd, 12.2)
  //
  // Construct block expecting 16 statements.
  Block* result =
      factory()->NewBlock(labels, 16, false, RelocInfo::kNoPosition);
  Target target(&this->target_stack_, result);
  Expect(Token::LBRACE, CHECK_OK);
  while (peek() != Token::RBRACE) {
    Statement* stat = ParseStatement(NULL, CHECK_OK);
    if (stat && !stat->IsEmpty()) {
      result->AddStatement(stat, zone());
    }
  }
  Expect(Token::RBRACE, CHECK_OK);
  return result;
}


Block* Parser::ParseScopedBlock(ZoneStringList* labels, bool* ok) {
  // The harmony mode uses block elements instead of statements.
  //
  // Block ::
  //   '{' BlockElement* '}'

  // Construct block expecting 16 statements.
  Block* body =
      factory()->NewBlock(labels, 16, false, RelocInfo::kNoPosition);
  Scope* block_scope = NewScope(scope_, BLOCK_SCOPE);

  // Parse the statements and collect escaping labels.
  Expect(Token::LBRACE, CHECK_OK);
  block_scope->set_start_position(scanner()->location().beg_pos);
  { BlockState block_state(&scope_, block_scope);
    TargetCollector collector(zone());
    Target target(&this->target_stack_, &collector);
    Target target_body(&this->target_stack_, body);

    while (peek() != Token::RBRACE) {
      Statement* stat = ParseBlockElement(NULL, CHECK_OK);
      if (stat && !stat->IsEmpty()) {
        body->AddStatement(stat, zone());
      }
    }
  }
  Expect(Token::RBRACE, CHECK_OK);
  block_scope->set_end_position(scanner()->location().end_pos);
  block_scope = block_scope->FinalizeBlockScope();
  body->set_scope(block_scope);
  return body;
}


Block* Parser::ParseVariableStatement(VariableDeclarationContext var_context,
                                      ZoneStringList* names,
                                      bool* ok) {
  // VariableStatement ::
  //   VariableDeclarations ';'

  Handle<String> ignore;
  Block* result =
      ParseVariableDeclarations(var_context, NULL, names, &ignore, CHECK_OK);
  ExpectSemicolon(CHECK_OK);
  return result;
}


// If the variable declaration declares exactly one non-const
// variable, then *out is set to that variable. In all other cases,
// *out is untouched; in particular, it is the caller's responsibility
// to initialize it properly. This mechanism is used for the parsing
// of 'for-in' loops.
Block* Parser::ParseVariableDeclarations(
    VariableDeclarationContext var_context,
    VariableDeclarationProperties* decl_props,
    ZoneStringList* names,
    Handle<String>* out,
    bool* ok) {
  // VariableDeclarations ::
  //   ('var' | 'const' | 'let') (Identifier ('=' AssignmentExpression)?)+[',']
  //
  // The ES6 Draft Rev3 specifies the following grammar for const declarations
  //
  // ConstDeclaration ::
  //   const ConstBinding (',' ConstBinding)* ';'
  // ConstBinding ::
  //   Identifier '=' AssignmentExpression
  //
  // TODO(ES6):
  // ConstBinding ::
  //   BindingPattern '=' AssignmentExpression

  int pos = peek_position();
  VariableMode mode = VAR;
  // True if the binding needs initialization. 'let' and 'const' declared
  // bindings are created uninitialized by their declaration nodes and
  // need initialization. 'var' declared bindings are always initialized
  // immediately by their declaration nodes.
  bool needs_init = false;
  bool is_const = false;
  Token::Value init_op = Token::INIT_VAR;
  if (peek() == Token::VAR) {
    Consume(Token::VAR);
  } else if (peek() == Token::CONST) {
    // TODO(ES6): The ES6 Draft Rev4 section 12.2.2 reads:
    //
    // ConstDeclaration : const ConstBinding (',' ConstBinding)* ';'
    //
    // * It is a Syntax Error if the code that matches this production is not
    //   contained in extended code.
    //
    // However disallowing const in sloppy mode will break compatibility with
    // existing pages. Therefore we keep allowing const with the old
    // non-harmony semantics in sloppy mode.
    Consume(Token::CONST);
    switch (strict_mode()) {
      case SLOPPY:
        mode = CONST_LEGACY;
        init_op = Token::INIT_CONST_LEGACY;
        break;
      case STRICT:
        if (allow_harmony_scoping()) {
          if (var_context == kStatement) {
            // In strict mode 'const' declarations are only allowed in source
            // element positions.
            ReportMessage("unprotected_const", Vector<const char*>::empty());
            *ok = false;
            return NULL;
          }
          mode = CONST;
          init_op = Token::INIT_CONST;
        } else {
          ReportMessage("strict_const", Vector<const char*>::empty());
          *ok = false;
          return NULL;
        }
    }
    is_const = true;
    needs_init = true;
  } else if (peek() == Token::LET) {
    // ES6 Draft Rev4 section 12.2.1:
    //
    // LetDeclaration : let LetBindingList ;
    //
    // * It is a Syntax Error if the code that matches this production is not
    //   contained in extended code.
    //
    // TODO(rossberg): make 'let' a legal identifier in sloppy mode.
    if (!allow_harmony_scoping() || strict_mode() == SLOPPY) {
      ReportMessage("illegal_let", Vector<const char*>::empty());
      *ok = false;
      return NULL;
    }
    Consume(Token::LET);
    if (var_context == kStatement) {
      // Let declarations are only allowed in source element positions.
      ReportMessage("unprotected_let", Vector<const char*>::empty());
      *ok = false;
      return NULL;
    }
    mode = LET;
    needs_init = true;
    init_op = Token::INIT_LET;
  } else {
    UNREACHABLE();  // by current callers
  }

  Scope* declaration_scope = DeclarationScope(mode);

  // The scope of a var/const declared variable anywhere inside a function
  // is the entire function (ECMA-262, 3rd, 10.1.3, and 12.2). Thus we can
  // transform a source-level var/const declaration into a (Function)
  // Scope declaration, and rewrite the source-level initialization into an
  // assignment statement. We use a block to collect multiple assignments.
  //
  // We mark the block as initializer block because we don't want the
  // rewriter to add a '.result' assignment to such a block (to get compliant
  // behavior for code such as print(eval('var x = 7')), and for cosmetic
  // reasons when pretty-printing. Also, unless an assignment (initialization)
  // is inside an initializer block, it is ignored.
  //
  // Create new block with one expected declaration.
  Block* block = factory()->NewBlock(NULL, 1, true, pos);
  int nvars = 0;  // the number of variables declared
  Handle<String> name;
  do {
    if (fni_ != NULL) fni_->Enter();

    // Parse variable name.
    if (nvars > 0) Consume(Token::COMMA);
    name = ParseIdentifier(kDontAllowEvalOrArguments, CHECK_OK);
    if (fni_ != NULL) fni_->PushVariableName(name);

    // Declare variable.
    // Note that we *always* must treat the initial value via a separate init
    // assignment for variables and constants because the value must be assigned
    // when the variable is encountered in the source. But the variable/constant
    // is declared (and set to 'undefined') upon entering the function within
    // which the variable or constant is declared. Only function variables have
    // an initial value in the declaration (because they are initialized upon
    // entering the function).
    //
    // If we have a const declaration, in an inner scope, the proxy is always
    // bound to the declared variable (independent of possibly surrounding with
    // statements).
    // For let/const declarations in harmony mode, we can also immediately
    // pre-resolve the proxy because it resides in the same scope as the
    // declaration.
    Interface* interface =
        is_const ? Interface::NewConst() : Interface::NewValue();
    VariableProxy* proxy = NewUnresolved(name, mode, interface);
    Declaration* declaration =
        factory()->NewVariableDeclaration(proxy, mode, scope_, pos);
    Declare(declaration, mode != VAR, CHECK_OK);
    nvars++;
    if (declaration_scope->num_var_or_const() > kMaxNumFunctionLocals) {
      ReportMessageAt(scanner()->location(), "too_many_variables");
      *ok = false;
      return NULL;
    }
    if (names) names->Add(name, zone());

    // Parse initialization expression if present and/or needed. A
    // declaration of the form:
    //
    //    var v = x;
    //
    // is syntactic sugar for:
    //
    //    var v; v = x;
    //
    // In particular, we need to re-lookup 'v' (in scope_, not
    // declaration_scope) as it may be a different 'v' than the 'v' in the
    // declaration (e.g., if we are inside a 'with' statement or 'catch'
    // block).
    //
    // However, note that const declarations are different! A const
    // declaration of the form:
    //
    //   const c = x;
    //
    // is *not* syntactic sugar for:
    //
    //   const c; c = x;
    //
    // The "variable" c initialized to x is the same as the declared
    // one - there is no re-lookup (see the last parameter of the
    // Declare() call above).

    Scope* initialization_scope = is_const ? declaration_scope : scope_;
    Expression* value = NULL;
    int pos = -1;
    // Harmony consts have non-optional initializers.
    if (peek() == Token::ASSIGN || mode == CONST) {
      Expect(Token::ASSIGN, CHECK_OK);
      pos = position();
      value = ParseAssignmentExpression(var_context != kForStatement, CHECK_OK);
      // Don't infer if it is "a = function(){...}();"-like expression.
      if (fni_ != NULL &&
          value->AsCall() == NULL &&
          value->AsCallNew() == NULL) {
        fni_->Infer();
      } else {
        fni_->RemoveLastFunction();
      }
      if (decl_props != NULL) *decl_props = kHasInitializers;
    }

    // Record the end position of the initializer.
    if (proxy->var() != NULL) {
      proxy->var()->set_initializer_position(position());
    }

    // Make sure that 'const x' and 'let x' initialize 'x' to undefined.
    if (value == NULL && needs_init) {
      value = GetLiteralUndefined(position());
    }

    // Global variable declarations must be compiled in a specific
    // way. When the script containing the global variable declaration
    // is entered, the global variable must be declared, so that if it
    // doesn't exist (on the global object itself, see ES5 errata) it
    // gets created with an initial undefined value. This is handled
    // by the declarations part of the function representing the
    // top-level global code; see Runtime::DeclareGlobalVariable. If
    // it already exists (in the object or in a prototype), it is
    // *not* touched until the variable declaration statement is
    // executed.
    //
    // Executing the variable declaration statement will always
    // guarantee to give the global object a "local" variable; a
    // variable defined in the global object and not in any
    // prototype. This way, global variable declarations can shadow
    // properties in the prototype chain, but only after the variable
    // declaration statement has been executed. This is important in
    // browsers where the global object (window) has lots of
    // properties defined in prototype objects.
    if (initialization_scope->is_global_scope() &&
        !IsLexicalVariableMode(mode)) {
      // Compute the arguments for the runtime call.
      ZoneList<Expression*>* arguments =
          new(zone()) ZoneList<Expression*>(3, zone());
      // We have at least 1 parameter.
      arguments->Add(factory()->NewLiteral(name, pos), zone());
      CallRuntime* initialize;

      if (is_const) {
        arguments->Add(value, zone());
        value = NULL;  // zap the value to avoid the unnecessary assignment

        // Construct the call to Runtime_InitializeConstGlobal
        // and add it to the initialization statement block.
        // Note that the function does different things depending on
        // the number of arguments (1 or 2).
        initialize = factory()->NewCallRuntime(
            isolate()->factory()->InitializeConstGlobal_string(),
            Runtime::FunctionForId(Runtime::kHiddenInitializeConstGlobal),
            arguments, pos);
      } else {
        // Add strict mode.
        // We may want to pass singleton to avoid Literal allocations.
        StrictMode strict_mode = initialization_scope->strict_mode();
        arguments->Add(factory()->NewNumberLiteral(strict_mode, pos), zone());

        // Be careful not to assign a value to the global variable if
        // we're in a with. The initialization value should not
        // necessarily be stored in the global object in that case,
        // which is why we need to generate a separate assignment node.
        if (value != NULL && !inside_with()) {
          arguments->Add(value, zone());
          value = NULL;  // zap the value to avoid the unnecessary assignment
        }

        // Construct the call to Runtime_InitializeVarGlobal
        // and add it to the initialization statement block.
        // Note that the function does different things depending on
        // the number of arguments (2 or 3).
        initialize = factory()->NewCallRuntime(
            isolate()->factory()->InitializeVarGlobal_string(),
            Runtime::FunctionForId(Runtime::kInitializeVarGlobal),
            arguments, pos);
      }

      block->AddStatement(
          factory()->NewExpressionStatement(initialize, RelocInfo::kNoPosition),
          zone());
    } else if (needs_init) {
      // Constant initializations always assign to the declared constant which
      // is always at the function scope level. This is only relevant for
      // dynamically looked-up variables and constants (the start context for
      // constant lookups is always the function context, while it is the top
      // context for var declared variables). Sigh...
      // For 'let' and 'const' declared variables in harmony mode the
      // initialization also always assigns to the declared variable.
      ASSERT(proxy != NULL);
      ASSERT(proxy->var() != NULL);
      ASSERT(value != NULL);
      Assignment* assignment =
          factory()->NewAssignment(init_op, proxy, value, pos);
      block->AddStatement(
          factory()->NewExpressionStatement(assignment, RelocInfo::kNoPosition),
          zone());
      value = NULL;
    }

    // Add an assignment node to the initialization statement block if we still
    // have a pending initialization value.
    if (value != NULL) {
      ASSERT(mode == VAR);
      // 'var' initializations are simply assignments (with all the consequences
      // if they are inside a 'with' statement - they may change a 'with' object
      // property).
      VariableProxy* proxy =
          initialization_scope->NewUnresolved(factory(), name, interface);
      Assignment* assignment =
          factory()->NewAssignment(init_op, proxy, value, pos);
      block->AddStatement(
          factory()->NewExpressionStatement(assignment, RelocInfo::kNoPosition),
          zone());
    }

    if (fni_ != NULL) fni_->Leave();
  } while (peek() == Token::COMMA);

  // If there was a single non-const declaration, return it in the output
  // parameter for possible use by for/in.
  if (nvars == 1 && !is_const) {
    *out = name;
  }

  return block;
}


static bool ContainsLabel(ZoneStringList* labels, Handle<String> label) {
  ASSERT(!label.is_null());
  if (labels != NULL)
    for (int i = labels->length(); i-- > 0; )
      if (labels->at(i).is_identical_to(label))
        return true;

  return false;
}


Statement* Parser::ParseExpressionOrLabelledStatement(ZoneStringList* labels,
                                                      bool* ok) {
  // ExpressionStatement | LabelledStatement ::
  //   Expression ';'
  //   Identifier ':' Statement
  int pos = peek_position();
  bool starts_with_idenfifier = peek_any_identifier();
  Expression* expr = ParseExpression(true, CHECK_OK);
  if (peek() == Token::COLON && starts_with_idenfifier && expr != NULL &&
      expr->AsVariableProxy() != NULL &&
      !expr->AsVariableProxy()->is_this()) {
    // Expression is a single identifier, and not, e.g., a parenthesized
    // identifier.
    VariableProxy* var = expr->AsVariableProxy();
    Handle<String> label = var->name();
    // TODO(1240780): We don't check for redeclaration of labels
    // during preparsing since keeping track of the set of active
    // labels requires nontrivial changes to the way scopes are
    // structured.  However, these are probably changes we want to
    // make later anyway so we should go back and fix this then.
    if (ContainsLabel(labels, label) || TargetStackContainsLabel(label)) {
      SmartArrayPointer<char> c_string = label->ToCString(DISALLOW_NULLS);
      const char* elms[2] = { "Label", c_string.get() };
      Vector<const char*> args(elms, 2);
      ReportMessage("redeclaration", args);
      *ok = false;
      return NULL;
    }
    if (labels == NULL) {
      labels = new(zone()) ZoneStringList(4, zone());
    }
    labels->Add(label, zone());
    // Remove the "ghost" variable that turned out to be a label
    // from the top scope. This way, we don't try to resolve it
    // during the scope processing.
    scope_->RemoveUnresolved(var);
    Expect(Token::COLON, CHECK_OK);
    return ParseStatement(labels, ok);
  }

  // If we have an extension, we allow a native function declaration.
  // A native function declaration starts with "native function" with
  // no line-terminator between the two words.
  if (extension_ != NULL &&
      peek() == Token::FUNCTION &&
      !scanner()->HasAnyLineTerminatorBeforeNext() &&
      expr != NULL &&
      expr->AsVariableProxy() != NULL &&
      expr->AsVariableProxy()->name()->Equals(
          isolate()->heap()->native_string()) &&
      !scanner()->literal_contains_escapes()) {
    return ParseNativeDeclaration(ok);
  }

  // Parsed expression statement, or the context-sensitive 'module' keyword.
  // Only expect semicolon in the former case.
  if (!FLAG_harmony_modules ||
      peek() != Token::IDENTIFIER ||
      scanner()->HasAnyLineTerminatorBeforeNext() ||
      expr->AsVariableProxy() == NULL ||
      !expr->AsVariableProxy()->name()->Equals(
          isolate()->heap()->module_string()) ||
      scanner()->literal_contains_escapes()) {
    ExpectSemicolon(CHECK_OK);
  }
  return factory()->NewExpressionStatement(expr, pos);
}


IfStatement* Parser::ParseIfStatement(ZoneStringList* labels, bool* ok) {
  // IfStatement ::
  //   'if' '(' Expression ')' Statement ('else' Statement)?

  int pos = peek_position();
  Expect(Token::IF, CHECK_OK);
  Expect(Token::LPAREN, CHECK_OK);
  Expression* condition = ParseExpression(true, CHECK_OK);
  Expect(Token::RPAREN, CHECK_OK);
  Statement* then_statement = ParseStatement(labels, CHECK_OK);
  Statement* else_statement = NULL;
  if (peek() == Token::ELSE) {
    Next();
    else_statement = ParseStatement(labels, CHECK_OK);
  } else {
    else_statement = factory()->NewEmptyStatement(RelocInfo::kNoPosition);
  }
  return factory()->NewIfStatement(
      condition, then_statement, else_statement, pos);
}


Statement* Parser::ParseContinueStatement(bool* ok) {
  // ContinueStatement ::
  //   'continue' Identifier? ';'

  int pos = peek_position();
  Expect(Token::CONTINUE, CHECK_OK);
  Handle<String> label = Handle<String>::null();
  Token::Value tok = peek();
  if (!scanner()->HasAnyLineTerminatorBeforeNext() &&
      tok != Token::SEMICOLON && tok != Token::RBRACE && tok != Token::EOS) {
    // ECMA allows "eval" or "arguments" as labels even in strict mode.
    label = ParseIdentifier(kAllowEvalOrArguments, CHECK_OK);
  }
  IterationStatement* target = NULL;
  target = LookupContinueTarget(label, CHECK_OK);
  if (target == NULL) {
    // Illegal continue statement.
    const char* message = "illegal_continue";
    Vector<Handle<String> > args;
    if (!label.is_null()) {
      message = "unknown_label";
      args = Vector<Handle<String> >(&label, 1);
    }
    ParserTraits::ReportMessageAt(scanner()->location(), message, args);
    *ok = false;
    return NULL;
  }
  ExpectSemicolon(CHECK_OK);
  return factory()->NewContinueStatement(target, pos);
}


Statement* Parser::ParseBreakStatement(ZoneStringList* labels, bool* ok) {
  // BreakStatement ::
  //   'break' Identifier? ';'

  int pos = peek_position();
  Expect(Token::BREAK, CHECK_OK);
  Handle<String> label;
  Token::Value tok = peek();
  if (!scanner()->HasAnyLineTerminatorBeforeNext() &&
      tok != Token::SEMICOLON && tok != Token::RBRACE && tok != Token::EOS) {
    // ECMA allows "eval" or "arguments" as labels even in strict mode.
    label = ParseIdentifier(kAllowEvalOrArguments, CHECK_OK);
  }
  // Parse labeled break statements that target themselves into
  // empty statements, e.g. 'l1: l2: l3: break l2;'
  if (!label.is_null() && ContainsLabel(labels, label)) {
    ExpectSemicolon(CHECK_OK);
    return factory()->NewEmptyStatement(pos);
  }
  BreakableStatement* target = NULL;
  target = LookupBreakTarget(label, CHECK_OK);
  if (target == NULL) {
    // Illegal break statement.
    const char* message = "illegal_break";
    Vector<Handle<String> > args;
    if (!label.is_null()) {
      message = "unknown_label";
      args = Vector<Handle<String> >(&label, 1);
    }
    ParserTraits::ReportMessageAt(scanner()->location(), message, args);
    *ok = false;
    return NULL;
  }
  ExpectSemicolon(CHECK_OK);
  return factory()->NewBreakStatement(target, pos);
}


Statement* Parser::ParseReturnStatement(bool* ok) {
  // ReturnStatement ::
  //   'return' Expression? ';'

  // Consume the return token. It is necessary to do that before
  // reporting any errors on it, because of the way errors are
  // reported (underlining).
  Expect(Token::RETURN, CHECK_OK);
  int pos = position();

  Token::Value tok = peek();
  Statement* result;
  Expression* return_value;
  if (scanner()->HasAnyLineTerminatorBeforeNext() ||
      tok == Token::SEMICOLON ||
      tok == Token::RBRACE ||
      tok == Token::EOS) {
    return_value = GetLiteralUndefined(position());
  } else {
    return_value = ParseExpression(true, CHECK_OK);
  }
  ExpectSemicolon(CHECK_OK);
  if (is_generator()) {
    Expression* generator = factory()->NewVariableProxy(
        function_state_->generator_object_variable());
    Expression* yield = factory()->NewYield(
        generator, return_value, Yield::FINAL, pos);
    result = factory()->NewExpressionStatement(yield, pos);
  } else {
    result = factory()->NewReturnStatement(return_value, pos);
  }

  // An ECMAScript program is considered syntactically incorrect if it
  // contains a return statement that is not within the body of a
  // function. See ECMA-262, section 12.9, page 67.
  //
  // To be consistent with KJS we report the syntax error at runtime.
  Scope* declaration_scope = scope_->DeclarationScope();
  if (declaration_scope->is_global_scope() ||
      declaration_scope->is_eval_scope()) {
    Expression* throw_error =
        NewThrowSyntaxError("illegal_return", Handle<Object>::null(), pos);
    return factory()->NewExpressionStatement(throw_error, pos);
  }
  return result;
}


Statement* Parser::ParseWithStatement(ZoneStringList* labels, bool* ok) {
  // WithStatement ::
  //   'with' '(' Expression ')' Statement

  Expect(Token::WITH, CHECK_OK);
  int pos = position();

  if (strict_mode() == STRICT) {
    ReportMessage("strict_mode_with", Vector<const char*>::empty());
    *ok = false;
    return NULL;
  }

  Expect(Token::LPAREN, CHECK_OK);
  Expression* expr = ParseExpression(true, CHECK_OK);
  Expect(Token::RPAREN, CHECK_OK);

  scope_->DeclarationScope()->RecordWithStatement();
  Scope* with_scope = NewScope(scope_, WITH_SCOPE);
  Statement* stmt;
  { BlockState block_state(&scope_, with_scope);
    with_scope->set_start_position(scanner()->peek_location().beg_pos);
    stmt = ParseStatement(labels, CHECK_OK);
    with_scope->set_end_position(scanner()->location().end_pos);
  }
  return factory()->NewWithStatement(with_scope, expr, stmt, pos);
}


CaseClause* Parser::ParseCaseClause(bool* default_seen_ptr, bool* ok) {
  // CaseClause ::
  //   'case' Expression ':' Statement*
  //   'default' ':' Statement*

  Expression* label = NULL;  // NULL expression indicates default case
  if (peek() == Token::CASE) {
    Expect(Token::CASE, CHECK_OK);
    label = ParseExpression(true, CHECK_OK);
  } else {
    Expect(Token::DEFAULT, CHECK_OK);
    if (*default_seen_ptr) {
      ReportMessage("multiple_defaults_in_switch",
                    Vector<const char*>::empty());
      *ok = false;
      return NULL;
    }
    *default_seen_ptr = true;
  }
  Expect(Token::COLON, CHECK_OK);
  int pos = position();
  ZoneList<Statement*>* statements =
      new(zone()) ZoneList<Statement*>(5, zone());
  while (peek() != Token::CASE &&
         peek() != Token::DEFAULT &&
         peek() != Token::RBRACE) {
    Statement* stat = ParseStatement(NULL, CHECK_OK);
    statements->Add(stat, zone());
  }

  return factory()->NewCaseClause(label, statements, pos);
}


SwitchStatement* Parser::ParseSwitchStatement(ZoneStringList* labels,
                                              bool* ok) {
  // SwitchStatement ::
  //   'switch' '(' Expression ')' '{' CaseClause* '}'

  SwitchStatement* statement =
      factory()->NewSwitchStatement(labels, peek_position());
  Target target(&this->target_stack_, statement);

  Expect(Token::SWITCH, CHECK_OK);
  Expect(Token::LPAREN, CHECK_OK);
  Expression* tag = ParseExpression(true, CHECK_OK);
  Expect(Token::RPAREN, CHECK_OK);

  bool default_seen = false;
  ZoneList<CaseClause*>* cases = new(zone()) ZoneList<CaseClause*>(4, zone());
  Expect(Token::LBRACE, CHECK_OK);
  while (peek() != Token::RBRACE) {
    CaseClause* clause = ParseCaseClause(&default_seen, CHECK_OK);
    cases->Add(clause, zone());
  }
  Expect(Token::RBRACE, CHECK_OK);

  if (statement) statement->Initialize(tag, cases);
  return statement;
}


Statement* Parser::ParseThrowStatement(bool* ok) {
  // ThrowStatement ::
  //   'throw' Expression ';'

  Expect(Token::THROW, CHECK_OK);
  int pos = position();
  if (scanner()->HasAnyLineTerminatorBeforeNext()) {
    ReportMessage("newline_after_throw", Vector<const char*>::empty());
    *ok = false;
    return NULL;
  }
  Expression* exception = ParseExpression(true, CHECK_OK);
  ExpectSemicolon(CHECK_OK);

  return factory()->NewExpressionStatement(
      factory()->NewThrow(exception, pos), pos);
}


TryStatement* Parser::ParseTryStatement(bool* ok) {
  // TryStatement ::
  //   'try' Block Catch
  //   'try' Block Finally
  //   'try' Block Catch Finally
  //
  // Catch ::
  //   'catch' '(' Identifier ')' Block
  //
  // Finally ::
  //   'finally' Block

  Expect(Token::TRY, CHECK_OK);
  int pos = position();

  TargetCollector try_collector(zone());
  Block* try_block;

  { Target target(&this->target_stack_, &try_collector);
    try_block = ParseBlock(NULL, CHECK_OK);
  }

  Token::Value tok = peek();
  if (tok != Token::CATCH && tok != Token::FINALLY) {
    ReportMessage("no_catch_or_finally", Vector<const char*>::empty());
    *ok = false;
    return NULL;
  }

  // If we can break out from the catch block and there is a finally block,
  // then we will need to collect escaping targets from the catch
  // block. Since we don't know yet if there will be a finally block, we
  // always collect the targets.
  TargetCollector catch_collector(zone());
  Scope* catch_scope = NULL;
  Variable* catch_variable = NULL;
  Block* catch_block = NULL;
  Handle<String> name;
  if (tok == Token::CATCH) {
    Consume(Token::CATCH);

    Expect(Token::LPAREN, CHECK_OK);
    catch_scope = NewScope(scope_, CATCH_SCOPE);
    catch_scope->set_start_position(scanner()->location().beg_pos);
    name = ParseIdentifier(kDontAllowEvalOrArguments, CHECK_OK);

    Expect(Token::RPAREN, CHECK_OK);

    Target target(&this->target_stack_, &catch_collector);
    VariableMode mode =
        allow_harmony_scoping() && strict_mode() == STRICT ? LET : VAR;
    catch_variable =
        catch_scope->DeclareLocal(name, mode, kCreatedInitialized);

    BlockState block_state(&scope_, catch_scope);
    catch_block = ParseBlock(NULL, CHECK_OK);

    catch_scope->set_end_position(scanner()->location().end_pos);
    tok = peek();
  }

  Block* finally_block = NULL;
  ASSERT(tok == Token::FINALLY || catch_block != NULL);
  if (tok == Token::FINALLY) {
    Consume(Token::FINALLY);
    finally_block = ParseBlock(NULL, CHECK_OK);
  }

  // Simplify the AST nodes by converting:
  //   'try B0 catch B1 finally B2'
  // to:
  //   'try { try B0 catch B1 } finally B2'

  if (catch_block != NULL && finally_block != NULL) {
    // If we have both, create an inner try/catch.
    ASSERT(catch_scope != NULL && catch_variable != NULL);
    int index = function_state_->NextHandlerIndex();
    TryCatchStatement* statement = factory()->NewTryCatchStatement(
        index, try_block, catch_scope, catch_variable, catch_block,
        RelocInfo::kNoPosition);
    statement->set_escaping_targets(try_collector.targets());
    try_block = factory()->NewBlock(NULL, 1, false, RelocInfo::kNoPosition);
    try_block->AddStatement(statement, zone());
    catch_block = NULL;  // Clear to indicate it's been handled.
  }

  TryStatement* result = NULL;
  if (catch_block != NULL) {
    ASSERT(finally_block == NULL);
    ASSERT(catch_scope != NULL && catch_variable != NULL);
    int index = function_state_->NextHandlerIndex();
    result = factory()->NewTryCatchStatement(
        index, try_block, catch_scope, catch_variable, catch_block, pos);
  } else {
    ASSERT(finally_block != NULL);
    int index = function_state_->NextHandlerIndex();
    result = factory()->NewTryFinallyStatement(
        index, try_block, finally_block, pos);
    // Combine the jump targets of the try block and the possible catch block.
    try_collector.targets()->AddAll(*catch_collector.targets(), zone());
  }

  result->set_escaping_targets(try_collector.targets());
  return result;
}


DoWhileStatement* Parser::ParseDoWhileStatement(ZoneStringList* labels,
                                                bool* ok) {
  // DoStatement ::
  //   'do' Statement 'while' '(' Expression ')' ';'

  DoWhileStatement* loop =
      factory()->NewDoWhileStatement(labels, peek_position());
  Target target(&this->target_stack_, loop);

  Expect(Token::DO, CHECK_OK);
  Statement* body = ParseStatement(NULL, CHECK_OK);
  Expect(Token::WHILE, CHECK_OK);
  Expect(Token::LPAREN, CHECK_OK);

  Expression* cond = ParseExpression(true, CHECK_OK);
  Expect(Token::RPAREN, CHECK_OK);

  // Allow do-statements to be terminated with and without
  // semi-colons. This allows code such as 'do;while(0)return' to
  // parse, which would not be the case if we had used the
  // ExpectSemicolon() functionality here.
  if (peek() == Token::SEMICOLON) Consume(Token::SEMICOLON);

  if (loop != NULL) loop->Initialize(cond, body);
  return loop;
}


WhileStatement* Parser::ParseWhileStatement(ZoneStringList* labels, bool* ok) {
  // WhileStatement ::
  //   'while' '(' Expression ')' Statement

  WhileStatement* loop = factory()->NewWhileStatement(labels, peek_position());
  Target target(&this->target_stack_, loop);

  Expect(Token::WHILE, CHECK_OK);
  Expect(Token::LPAREN, CHECK_OK);
  Expression* cond = ParseExpression(true, CHECK_OK);
  Expect(Token::RPAREN, CHECK_OK);
  Statement* body = ParseStatement(NULL, CHECK_OK);

  if (loop != NULL) loop->Initialize(cond, body);
  return loop;
}


bool Parser::CheckInOrOf(bool accept_OF,
                         ForEachStatement::VisitMode* visit_mode) {
  if (Check(Token::IN)) {
    *visit_mode = ForEachStatement::ENUMERATE;
    return true;
  } else if (allow_for_of() && accept_OF &&
             CheckContextualKeyword(CStrVector("of"))) {
    *visit_mode = ForEachStatement::ITERATE;
    return true;
  }
  return false;
}


void Parser::InitializeForEachStatement(ForEachStatement* stmt,
                                        Expression* each,
                                        Expression* subject,
                                        Statement* body) {
  ForOfStatement* for_of = stmt->AsForOfStatement();

  if (for_of != NULL) {
    Factory* heap_factory = isolate()->factory();
    Variable* iterator = scope_->DeclarationScope()->NewTemporary(
        heap_factory->dot_iterator_string());
    Variable* result = scope_->DeclarationScope()->NewTemporary(
        heap_factory->dot_result_string());

    Expression* assign_iterator;
    Expression* next_result;
    Expression* result_done;
    Expression* assign_each;

    // var iterator = iterable;
    {
      Expression* iterator_proxy = factory()->NewVariableProxy(iterator);
      assign_iterator = factory()->NewAssignment(
          Token::ASSIGN, iterator_proxy, subject, RelocInfo::kNoPosition);
    }

    // var result = iterator.next();
    {
      Expression* iterator_proxy = factory()->NewVariableProxy(iterator);
      Expression* next_literal = factory()->NewLiteral(
          heap_factory->next_string(), RelocInfo::kNoPosition);
      Expression* next_property = factory()->NewProperty(
          iterator_proxy, next_literal, RelocInfo::kNoPosition);
      ZoneList<Expression*>* next_arguments =
          new(zone()) ZoneList<Expression*>(0, zone());
      Expression* next_call = factory()->NewCall(
          next_property, next_arguments, RelocInfo::kNoPosition);
      Expression* result_proxy = factory()->NewVariableProxy(result);
      next_result = factory()->NewAssignment(
          Token::ASSIGN, result_proxy, next_call, RelocInfo::kNoPosition);
    }

    // result.done
    {
      Expression* done_literal = factory()->NewLiteral(
          heap_factory->done_string(), RelocInfo::kNoPosition);
      Expression* result_proxy = factory()->NewVariableProxy(result);
      result_done = factory()->NewProperty(
          result_proxy, done_literal, RelocInfo::kNoPosition);
    }

    // each = result.value
    {
      Expression* value_literal = factory()->NewLiteral(
          heap_factory->value_string(), RelocInfo::kNoPosition);
      Expression* result_proxy = factory()->NewVariableProxy(result);
      Expression* result_value = factory()->NewProperty(
          result_proxy, value_literal, RelocInfo::kNoPosition);
      assign_each = factory()->NewAssignment(
          Token::ASSIGN, each, result_value, RelocInfo::kNoPosition);
    }

    for_of->Initialize(each, subject, body,
                       assign_iterator, next_result, result_done, assign_each);
  } else {
    stmt->Initialize(each, subject, body);
  }
}


Statement* Parser::ParseForStatement(ZoneStringList* labels, bool* ok) {
  // ForStatement ::
  //   'for' '(' Expression? ';' Expression? ';' Expression? ')' Statement

  int pos = peek_position();
  Statement* init = NULL;

  // Create an in-between scope for let-bound iteration variables.
  Scope* saved_scope = scope_;
  Scope* for_scope = NewScope(scope_, BLOCK_SCOPE);
  scope_ = for_scope;

  Expect(Token::FOR, CHECK_OK);
  Expect(Token::LPAREN, CHECK_OK);
  for_scope->set_start_position(scanner()->location().beg_pos);
  if (peek() != Token::SEMICOLON) {
    if (peek() == Token::VAR || peek() == Token::CONST) {
      bool is_const = peek() == Token::CONST;
      Handle<String> name;
      VariableDeclarationProperties decl_props = kHasNoInitializers;
      Block* variable_statement =
          ParseVariableDeclarations(kForStatement, &decl_props, NULL, &name,
                                    CHECK_OK);
      bool accept_OF = decl_props == kHasNoInitializers;
      ForEachStatement::VisitMode mode;

      if (!name.is_null() && CheckInOrOf(accept_OF, &mode)) {
        Interface* interface =
            is_const ? Interface::NewConst() : Interface::NewValue();
        ForEachStatement* loop =
            factory()->NewForEachStatement(mode, labels, pos);
        Target target(&this->target_stack_, loop);

        Expression* enumerable = ParseExpression(true, CHECK_OK);
        Expect(Token::RPAREN, CHECK_OK);

        VariableProxy* each =
            scope_->NewUnresolved(factory(), name, interface);
        Statement* body = ParseStatement(NULL, CHECK_OK);
        InitializeForEachStatement(loop, each, enumerable, body);
        Block* result =
            factory()->NewBlock(NULL, 2, false, RelocInfo::kNoPosition);
        result->AddStatement(variable_statement, zone());
        result->AddStatement(loop, zone());
        scope_ = saved_scope;
        for_scope->set_end_position(scanner()->location().end_pos);
        for_scope = for_scope->FinalizeBlockScope();
        ASSERT(for_scope == NULL);
        // Parsed for-in loop w/ variable/const declaration.
        return result;
      } else {
        init = variable_statement;
      }
    } else if (peek() == Token::LET) {
      Handle<String> name;
      VariableDeclarationProperties decl_props = kHasNoInitializers;
      Block* variable_statement =
         ParseVariableDeclarations(kForStatement, &decl_props, NULL, &name,
                                   CHECK_OK);
      bool accept_IN = !name.is_null() && decl_props != kHasInitializers;
      bool accept_OF = decl_props == kHasNoInitializers;
      ForEachStatement::VisitMode mode;

      if (accept_IN && CheckInOrOf(accept_OF, &mode)) {
        // Rewrite a for-in statement of the form
        //
        //   for (let x in e) b
        //
        // into
        //
        //   <let x' be a temporary variable>
        //   for (x' in e) {
        //     let x;
        //     x = x';
        //     b;
        //   }

        // TODO(keuchel): Move the temporary variable to the block scope, after
        // implementing stack allocated block scoped variables.
        Factory* heap_factory = isolate()->factory();
        Handle<String> tempstr =
            heap_factory->NewConsString(heap_factory->dot_for_string(), name);
        RETURN_IF_EMPTY_HANDLE_VALUE(isolate(), tempstr, 0);
        Handle<String> tempname = heap_factory->InternalizeString(tempstr);
        Variable* temp = scope_->DeclarationScope()->NewTemporary(tempname);
        VariableProxy* temp_proxy = factory()->NewVariableProxy(temp);
        ForEachStatement* loop =
            factory()->NewForEachStatement(mode, labels, pos);
        Target target(&this->target_stack_, loop);

        // The expression does not see the loop variable.
        scope_ = saved_scope;
        Expression* enumerable = ParseExpression(true, CHECK_OK);
        scope_ = for_scope;
        Expect(Token::RPAREN, CHECK_OK);

        VariableProxy* each =
            scope_->NewUnresolved(factory(), name, Interface::NewValue());
        Statement* body = ParseStatement(NULL, CHECK_OK);
        Block* body_block =
            factory()->NewBlock(NULL, 3, false, RelocInfo::kNoPosition);
        Assignment* assignment = factory()->NewAssignment(
            Token::ASSIGN, each, temp_proxy, RelocInfo::kNoPosition);
        Statement* assignment_statement = factory()->NewExpressionStatement(
            assignment, RelocInfo::kNoPosition);
        body_block->AddStatement(variable_statement, zone());
        body_block->AddStatement(assignment_statement, zone());
        body_block->AddStatement(body, zone());
        InitializeForEachStatement(loop, temp_proxy, enumerable, body_block);
        scope_ = saved_scope;
        for_scope->set_end_position(scanner()->location().end_pos);
        for_scope = for_scope->FinalizeBlockScope();
        body_block->set_scope(for_scope);
        // Parsed for-in loop w/ let declaration.
        return loop;

      } else {
        init = variable_statement;
      }
    } else {
      Scanner::Location lhs_location = scanner()->peek_location();
      Expression* expression = ParseExpression(false, CHECK_OK);
      ForEachStatement::VisitMode mode;
      bool accept_OF = expression->AsVariableProxy();

      if (CheckInOrOf(accept_OF, &mode)) {
        expression = this->CheckAndRewriteReferenceExpression(
            expression, lhs_location, "invalid_lhs_in_for", CHECK_OK);

        ForEachStatement* loop =
            factory()->NewForEachStatement(mode, labels, pos);
        Target target(&this->target_stack_, loop);

        Expression* enumerable = ParseExpression(true, CHECK_OK);
        Expect(Token::RPAREN, CHECK_OK);

        Statement* body = ParseStatement(NULL, CHECK_OK);
        InitializeForEachStatement(loop, expression, enumerable, body);
        scope_ = saved_scope;
        for_scope->set_end_position(scanner()->location().end_pos);
        for_scope = for_scope->FinalizeBlockScope();
        ASSERT(for_scope == NULL);
        // Parsed for-in loop.
        return loop;

      } else {
        init = factory()->NewExpressionStatement(
            expression, RelocInfo::kNoPosition);
      }
    }
  }

  // Standard 'for' loop
  ForStatement* loop = factory()->NewForStatement(labels, pos);
  Target target(&this->target_stack_, loop);

  // Parsed initializer at this point.
  Expect(Token::SEMICOLON, CHECK_OK);

  Expression* cond = NULL;
  if (peek() != Token::SEMICOLON) {
    cond = ParseExpression(true, CHECK_OK);
  }
  Expect(Token::SEMICOLON, CHECK_OK);

  Statement* next = NULL;
  if (peek() != Token::RPAREN) {
    Expression* exp = ParseExpression(true, CHECK_OK);
    next = factory()->NewExpressionStatement(exp, RelocInfo::kNoPosition);
  }
  Expect(Token::RPAREN, CHECK_OK);

  Statement* body = ParseStatement(NULL, CHECK_OK);
  scope_ = saved_scope;
  for_scope->set_end_position(scanner()->location().end_pos);
  for_scope = for_scope->FinalizeBlockScope();
  if (for_scope != NULL) {
    // Rewrite a for statement of the form
    //
    //   for (let x = i; c; n) b
    //
    // into
    //
    //   {
    //     let x = i;
    //     for (; c; n) b
    //   }
    ASSERT(init != NULL);
    Block* result = factory()->NewBlock(NULL, 2, false, RelocInfo::kNoPosition);
    result->AddStatement(init, zone());
    result->AddStatement(loop, zone());
    result->set_scope(for_scope);
    loop->Initialize(NULL, cond, next, body);
    return result;
  } else {
    loop->Initialize(init, cond, next, body);
    return loop;
  }
}


DebuggerStatement* Parser::ParseDebuggerStatement(bool* ok) {
  // In ECMA-262 'debugger' is defined as a reserved keyword. In some browser
  // contexts this is used as a statement which invokes the debugger as i a
  // break point is present.
  // DebuggerStatement ::
  //   'debugger' ';'

  int pos = peek_position();
  Expect(Token::DEBUGGER, CHECK_OK);
  ExpectSemicolon(CHECK_OK);
  return factory()->NewDebuggerStatement(pos);
}


void Parser::ReportInvalidPreparseData(Handle<String> name, bool* ok) {
  SmartArrayPointer<char> name_string = name->ToCString(DISALLOW_NULLS);
  const char* element[1] = { name_string.get() };
  ReportMessage("invalid_preparser_data",
                Vector<const char*>(element, 1));
  *ok = false;
}


bool CompileTimeValue::IsCompileTimeValue(Expression* expression) {
  if (expression->AsLiteral() != NULL) return true;
  MaterializedLiteral* lit = expression->AsMaterializedLiteral();
  return lit != NULL && lit->is_simple();
}


Handle<FixedArray> CompileTimeValue::GetValue(Isolate* isolate,
                                              Expression* expression) {
  Factory* factory = isolate->factory();
  ASSERT(IsCompileTimeValue(expression));
  Handle<FixedArray> result = factory->NewFixedArray(2, TENURED);
  ObjectLiteral* object_literal = expression->AsObjectLiteral();
  if (object_literal != NULL) {
    ASSERT(object_literal->is_simple());
    if (object_literal->fast_elements()) {
      result->set(kLiteralTypeSlot, Smi::FromInt(OBJECT_LITERAL_FAST_ELEMENTS));
    } else {
      result->set(kLiteralTypeSlot, Smi::FromInt(OBJECT_LITERAL_SLOW_ELEMENTS));
    }
    result->set(kElementsSlot, *object_literal->constant_properties());
  } else {
    ArrayLiteral* array_literal = expression->AsArrayLiteral();
    ASSERT(array_literal != NULL && array_literal->is_simple());
    result->set(kLiteralTypeSlot, Smi::FromInt(ARRAY_LITERAL));
    result->set(kElementsSlot, *array_literal->constant_elements());
  }
  return result;
}


CompileTimeValue::LiteralType CompileTimeValue::GetLiteralType(
    Handle<FixedArray> value) {
  Smi* literal_type = Smi::cast(value->get(kLiteralTypeSlot));
  return static_cast<LiteralType>(literal_type->value());
}


Handle<FixedArray> CompileTimeValue::GetElements(Handle<FixedArray> value) {
  return Handle<FixedArray>(FixedArray::cast(value->get(kElementsSlot)));
}


FunctionLiteral* Parser::ParseFunctionLiteral(
    Handle<String> function_name,
    Scanner::Location function_name_location,
    bool name_is_strict_reserved,
    bool is_generator,
    int function_token_pos,
    FunctionLiteral::FunctionType function_type,
    bool* ok) {
  // Function ::
  //   '(' FormalParameterList? ')' '{' FunctionBody '}'

  int pos = function_token_pos == RelocInfo::kNoPosition
      ? peek_position() : function_token_pos;

  // Anonymous functions were passed either the empty symbol or a null
  // handle as the function name.  Remember if we were passed a non-empty
  // handle to decide whether to invoke function name inference.
  bool should_infer_name = function_name.is_null();

  // We want a non-null handle as the function name.
  if (should_infer_name) {
    function_name = isolate()->factory()->empty_string();
  }

  int num_parameters = 0;
  // Function declarations are function scoped in normal mode, so they are
  // hoisted. In harmony block scoping mode they are block scoped, so they
  // are not hoisted.
  //
  // One tricky case are function declarations in a local sloppy-mode eval:
  // their declaration is hoisted, but they still see the local scope. E.g.,
  //
  // function() {
  //   var x = 0
  //   try { throw 1 } catch (x) { eval("function g() { return x }") }
  //   return g()
  // }
  //
  // needs to return 1. To distinguish such cases, we need to detect
  // (1) whether a function stems from a sloppy eval, and
  // (2) whether it actually hoists across the eval.
  // Unfortunately, we do not represent sloppy eval scopes, so we do not have
  // either information available directly, especially not when lazily compiling
  // a function like 'g'. We hence rely on the following invariants:
  // - (1) is the case iff the innermost scope of the deserialized scope chain
  //   under which we compile is _not_ a declaration scope. This holds because
  //   in all normal cases, function declarations are fully hoisted to a
  //   declaration scope and compiled relative to that.
  // - (2) is the case iff the current declaration scope is still the original
  //   one relative to the deserialized scope chain. Otherwise we must be
  //   compiling a function in an inner declaration scope in the eval, e.g. a
  //   nested function, and hoisting works normally relative to that.
  Scope* declaration_scope = scope_->DeclarationScope();
  Scope* original_declaration_scope = original_scope_->DeclarationScope();
  Scope* scope =
      function_type == FunctionLiteral::DECLARATION &&
      (!allow_harmony_scoping() || strict_mode() == SLOPPY) &&
      (original_scope_ == original_declaration_scope ||
       declaration_scope != original_declaration_scope)
          ? NewScope(declaration_scope, FUNCTION_SCOPE)
          : NewScope(scope_, FUNCTION_SCOPE);
  ZoneList<Statement*>* body = NULL;
  int materialized_literal_count = -1;
  int expected_property_count = -1;
  int handler_count = 0;
  FunctionLiteral::ParameterFlag duplicate_parameters =
      FunctionLiteral::kNoDuplicateParameters;
  FunctionLiteral::IsParenthesizedFlag parenthesized = parenthesized_function_
      ? FunctionLiteral::kIsParenthesized
      : FunctionLiteral::kNotParenthesized;
  FunctionLiteral::IsGeneratorFlag generator = is_generator
      ? FunctionLiteral::kIsGenerator
      : FunctionLiteral::kNotGenerator;
  DeferredFeedbackSlotProcessor* slot_processor;
  AstProperties ast_properties;
  BailoutReason dont_optimize_reason = kNoReason;
  // Parse function body.
  { FunctionState function_state(&function_state_, &scope_, scope, zone());
    scope_->SetScopeName(function_name);

    if (is_generator) {
      // For generators, allocating variables in contexts is currently a win
      // because it minimizes the work needed to suspend and resume an
      // activation.
      scope_->ForceContextAllocation();

      // Calling a generator returns a generator object.  That object is stored
      // in a temporary variable, a definition that is used by "yield"
      // expressions. This also marks the FunctionState as a generator.
      Variable* temp = scope_->DeclarationScope()->NewTemporary(
          isolate()->factory()->dot_generator_object_string());
      function_state.set_generator_object_variable(temp);
    }

    //  FormalParameterList ::
    //    '(' (Identifier)*[','] ')'
    Expect(Token::LPAREN, CHECK_OK);
    scope->set_start_position(scanner()->location().beg_pos);

    // We don't yet know if the function will be strict, so we cannot yet
    // produce errors for parameter names or duplicates. However, we remember
    // the locations of these errors if they occur and produce the errors later.
    Scanner::Location eval_args_error_log = Scanner::Location::invalid();
    Scanner::Location dupe_error_loc = Scanner::Location::invalid();
    Scanner::Location reserved_loc = Scanner::Location::invalid();

    bool done = (peek() == Token::RPAREN);
    while (!done) {
      bool is_strict_reserved = false;
      Handle<String> param_name =
          ParseIdentifierOrStrictReservedWord(&is_strict_reserved, CHECK_OK);

      // Store locations for possible future error reports.
      if (!eval_args_error_log.IsValid() && IsEvalOrArguments(param_name)) {
        eval_args_error_log = scanner()->location();
      }
      if (!reserved_loc.IsValid() && is_strict_reserved) {
        reserved_loc = scanner()->location();
      }
      if (!dupe_error_loc.IsValid() && scope_->IsDeclared(param_name)) {
        duplicate_parameters = FunctionLiteral::kHasDuplicateParameters;
        dupe_error_loc = scanner()->location();
      }

      scope_->DeclareParameter(param_name, VAR);
      num_parameters++;
      if (num_parameters > Code::kMaxArguments) {
        ReportMessageAt(scanner()->location(), "too_many_parameters");
        *ok = false;
        return NULL;
      }
      done = (peek() == Token::RPAREN);
      if (!done) Expect(Token::COMMA, CHECK_OK);
    }
    Expect(Token::RPAREN, CHECK_OK);

    Expect(Token::LBRACE, CHECK_OK);

    // If we have a named function expression, we add a local variable
    // declaration to the body of the function with the name of the
    // function and let it refer to the function itself (closure).
    // NOTE: We create a proxy and resolve it here so that in the
    // future we can change the AST to only refer to VariableProxies
    // instead of Variables and Proxis as is the case now.
    Variable* fvar = NULL;
    Token::Value fvar_init_op = Token::INIT_CONST_LEGACY;
    if (function_type == FunctionLiteral::NAMED_EXPRESSION) {
      if (allow_harmony_scoping() && strict_mode() == STRICT) {
        fvar_init_op = Token::INIT_CONST;
      }
      VariableMode fvar_mode =
          allow_harmony_scoping() && strict_mode() == STRICT ? CONST
                                                             : CONST_LEGACY;
      fvar = new(zone()) Variable(scope_,
         function_name, fvar_mode, true /* is valid LHS */,
         Variable::NORMAL, kCreatedInitialized, Interface::NewConst());
      VariableProxy* proxy = factory()->NewVariableProxy(fvar);
      VariableDeclaration* fvar_declaration = factory()->NewVariableDeclaration(
          proxy, fvar_mode, scope_, RelocInfo::kNoPosition);
      scope_->DeclareFunctionVar(fvar_declaration);
    }

    // Determine if the function can be parsed lazily. Lazy parsing is different
    // from lazy compilation; we need to parse more eagerly than we compile.

    // We can only parse lazily if we also compile lazily. The heuristics for
    // lazy compilation are:
    // - It must not have been prohibited by the caller to Parse (some callers
    //   need a full AST).
    // - The outer scope must allow lazy compilation of inner functions.
    // - The function mustn't be a function expression with an open parenthesis
    //   before; we consider that a hint that the function will be called
    //   immediately, and it would be a waste of time to make it lazily
    //   compiled.
    // These are all things we can know at this point, without looking at the
    // function itself.

    // In addition, we need to distinguish between these cases:
    // (function foo() {
    //   bar = function() { return 1; }
    //  })();
    // and
    // (function foo() {
    //   var a = 1;
    //   bar = function() { return a; }
    //  })();

    // Now foo will be parsed eagerly and compiled eagerly (optimization: assume
    // parenthesis before the function means that it will be called
    // immediately). The inner function *must* be parsed eagerly to resolve the
    // possible reference to the variable in foo's scope. However, it's possible
    // that it will be compiled lazily.

    // To make this additional case work, both Parser and PreParser implement a
    // logic where only top-level functions will be parsed lazily.
    bool is_lazily_parsed = (mode() == PARSE_LAZILY &&
                             scope_->AllowsLazyCompilation() &&
                             !parenthesized_function_);
    parenthesized_function_ = false;  // The bit was set for this function only.

    if (is_lazily_parsed) {
      int function_block_pos = position();
      FunctionEntry entry;
      if (cached_data_mode_ == CONSUME_CACHED_DATA) {
        // If we have cached data, we use it to skip parsing the function body.
        // The data contains the information we need to construct the lazy
        // function.
        entry = (*cached_data())->GetFunctionEntry(function_block_pos);
        if (entry.is_valid()) {
          if (entry.end_pos() <= function_block_pos) {
            // End position greater than end of stream is safe, and hard
            // to check.
            ReportInvalidPreparseData(function_name, CHECK_OK);
          }
          scanner()->SeekForward(entry.end_pos() - 1);

          scope->set_end_position(entry.end_pos());
          Expect(Token::RBRACE, CHECK_OK);
          isolate()->counters()->total_preparse_skipped()->Increment(
              scope->end_position() - function_block_pos);
          materialized_literal_count = entry.literal_count();
          expected_property_count = entry.property_count();
          scope_->SetStrictMode(entry.strict_mode());
        } else {
          // This case happens when we have preparse data but it doesn't contain
          // an entry for the function. As a safety net, fall back to eager
          // parsing. It is unclear whether PreParser's laziness analysis can
          // produce different results than the Parser's laziness analysis (see
          // https://codereview.chromium.org/7565003 ). In this case, we must
          // discard all the preparse data, since the symbol data will be wrong.
          is_lazily_parsed = false;
          cached_data_mode_ = NO_CACHED_DATA;
        }
      } else {
        // With no cached data, we partially parse the function, without
        // building an AST. This gathers the data needed to build a lazy
        // function.
        // FIXME(marja): Now the PreParser doesn't need to log functions /
        // symbols; only errors -> clean that up.
        SingletonLogger logger;
        PreParser::PreParseResult result = LazyParseFunctionLiteral(&logger);
        if (result == PreParser::kPreParseStackOverflow) {
          // Propagate stack overflow.
          set_stack_overflow();
          *ok = false;
          return NULL;
        }
        if (logger.has_error()) {
          const char* arg = logger.argument_opt();
          Vector<const char*> args;
          if (arg != NULL) {
            args = Vector<const char*>(&arg, 1);
          }
          ParserTraits::ReportMessageAt(
              Scanner::Location(logger.start(), logger.end()),
              logger.message(),
              args);
          *ok = false;
          return NULL;
        }
        scope->set_end_position(logger.end());
        Expect(Token::RBRACE, CHECK_OK);
        isolate()->counters()->total_preparse_skipped()->Increment(
            scope->end_position() - function_block_pos);
        materialized_literal_count = logger.literals();
        expected_property_count = logger.properties();
        scope_->SetStrictMode(logger.strict_mode());
        if (cached_data_mode_ == PRODUCE_CACHED_DATA) {
          ASSERT(log_);
          // Position right after terminal '}'.
          int body_end = scanner()->location().end_pos;
          log_->LogFunction(function_block_pos, body_end,
                            materialized_literal_count,
                            expected_property_count,
                            scope_->strict_mode());
        }
      }
    }

    if (!is_lazily_parsed) {
      // Everything inside an eagerly parsed function will be parsed eagerly
      // (see comment above).
      ParsingModeScope parsing_mode(this, PARSE_EAGERLY);
      body = new(zone()) ZoneList<Statement*>(8, zone());
      if (fvar != NULL) {
        VariableProxy* fproxy = scope_->NewUnresolved(
            factory(), function_name, Interface::NewConst());
        fproxy->BindTo(fvar);
        body->Add(factory()->NewExpressionStatement(
            factory()->NewAssignment(fvar_init_op,
                                     fproxy,
                                     factory()->NewThisFunction(pos),
                                     RelocInfo::kNoPosition),
            RelocInfo::kNoPosition), zone());
      }

      // For generators, allocate and yield an iterator on function entry.
      if (is_generator) {
        ZoneList<Expression*>* arguments =
            new(zone()) ZoneList<Expression*>(0, zone());
        CallRuntime* allocation = factory()->NewCallRuntime(
            isolate()->factory()->empty_string(),
            Runtime::FunctionForId(Runtime::kHiddenCreateJSGeneratorObject),
            arguments, pos);
        VariableProxy* init_proxy = factory()->NewVariableProxy(
            function_state_->generator_object_variable());
        Assignment* assignment = factory()->NewAssignment(
            Token::INIT_VAR, init_proxy, allocation, RelocInfo::kNoPosition);
        VariableProxy* get_proxy = factory()->NewVariableProxy(
            function_state_->generator_object_variable());
        Yield* yield = factory()->NewYield(
            get_proxy, assignment, Yield::INITIAL, RelocInfo::kNoPosition);
        body->Add(factory()->NewExpressionStatement(
            yield, RelocInfo::kNoPosition), zone());
      }

      ParseSourceElements(body, Token::RBRACE, false, false, CHECK_OK);

      if (is_generator) {
        VariableProxy* get_proxy = factory()->NewVariableProxy(
            function_state_->generator_object_variable());
        Expression *undefined = factory()->NewLiteral(
            isolate()->factory()->undefined_value(), RelocInfo::kNoPosition);
        Yield* yield = factory()->NewYield(
            get_proxy, undefined, Yield::FINAL, RelocInfo::kNoPosition);
        body->Add(factory()->NewExpressionStatement(
            yield, RelocInfo::kNoPosition), zone());
      }

      materialized_literal_count = function_state.materialized_literal_count();
      expected_property_count = function_state.expected_property_count();
      handler_count = function_state.handler_count();

      Expect(Token::RBRACE, CHECK_OK);
      scope->set_end_position(scanner()->location().end_pos);
    }

    // Validate strict mode. We can do this only after parsing the function,
    // since the function can declare itself strict.
    if (strict_mode() == STRICT) {
      if (IsEvalOrArguments(function_name)) {
        ReportMessageAt(function_name_location, "strict_eval_arguments");
        *ok = false;
        return NULL;
      }
      if (name_is_strict_reserved) {
        ReportMessageAt(function_name_location, "unexpected_strict_reserved");
        *ok = false;
        return NULL;
      }
      if (eval_args_error_log.IsValid()) {
        ReportMessageAt(eval_args_error_log, "strict_eval_arguments");
        *ok = false;
        return NULL;
      }
      if (dupe_error_loc.IsValid()) {
        ReportMessageAt(dupe_error_loc, "strict_param_dupe");
        *ok = false;
        return NULL;
      }
      if (reserved_loc.IsValid()) {
        ReportMessageAt(reserved_loc, "unexpected_strict_reserved");
        *ok = false;
        return NULL;
      }
      CheckOctalLiteral(scope->start_position(),
                        scope->end_position(),
                        CHECK_OK);
    }
    ast_properties = *factory()->visitor()->ast_properties();
    slot_processor = factory()->visitor()->slot_processor();
    dont_optimize_reason = factory()->visitor()->dont_optimize_reason();
  }

  if (allow_harmony_scoping() && strict_mode() == STRICT) {
    CheckConflictingVarDeclarations(scope, CHECK_OK);
  }

  FunctionLiteral* function_literal =
      factory()->NewFunctionLiteral(function_name,
                                    scope,
                                    body,
                                    materialized_literal_count,
                                    expected_property_count,
                                    handler_count,
                                    num_parameters,
                                    duplicate_parameters,
                                    function_type,
                                    FunctionLiteral::kIsFunction,
                                    parenthesized,
                                    generator,
                                    pos);
  function_literal->set_function_token_position(function_token_pos);
  function_literal->set_ast_properties(&ast_properties);
  function_literal->set_slot_processor(slot_processor);
  function_literal->set_dont_optimize_reason(dont_optimize_reason);

  if (fni_ != NULL && should_infer_name) fni_->AddFunction(function_literal);
  return function_literal;
}


PreParser::PreParseResult Parser::LazyParseFunctionLiteral(
    SingletonLogger* logger) {
  HistogramTimerScope preparse_scope(isolate()->counters()->pre_parse());
  ASSERT_EQ(Token::LBRACE, scanner()->current_token());

  if (reusable_preparser_ == NULL) {
    intptr_t stack_limit = isolate()->stack_guard()->real_climit();
    reusable_preparser_ = new PreParser(&scanner_, NULL, stack_limit);
    reusable_preparser_->set_allow_harmony_scoping(allow_harmony_scoping());
    reusable_preparser_->set_allow_modules(allow_modules());
    reusable_preparser_->set_allow_natives_syntax(allow_natives_syntax());
    reusable_preparser_->set_allow_lazy(true);
    reusable_preparser_->set_allow_generators(allow_generators());
    reusable_preparser_->set_allow_for_of(allow_for_of());
    reusable_preparser_->set_allow_harmony_numeric_literals(
        allow_harmony_numeric_literals());
  }
  PreParser::PreParseResult result =
      reusable_preparser_->PreParseLazyFunction(strict_mode(),
                                                is_generator(),
                                                logger);
  return result;
}


Expression* Parser::ParseV8Intrinsic(bool* ok) {
  // CallRuntime ::
  //   '%' Identifier Arguments

  int pos = peek_position();
  Expect(Token::MOD, CHECK_OK);
  // Allow "eval" or "arguments" for backward compatibility.
  Handle<String> name = ParseIdentifier(kAllowEvalOrArguments, CHECK_OK);
  ZoneList<Expression*>* args = ParseArguments(CHECK_OK);

  if (extension_ != NULL) {
    // The extension structures are only accessible while parsing the
    // very first time not when reparsing because of lazy compilation.
    scope_->DeclarationScope()->ForceEagerCompilation();
  }

  const Runtime::Function* function = Runtime::FunctionForName(name);

  // Check for built-in IS_VAR macro.
  if (function != NULL &&
      function->intrinsic_type == Runtime::RUNTIME &&
      function->function_id == Runtime::kIS_VAR) {
    // %IS_VAR(x) evaluates to x if x is a variable,
    // leads to a parse error otherwise.  Could be implemented as an
    // inline function %_IS_VAR(x) to eliminate this special case.
    if (args->length() == 1 && args->at(0)->AsVariableProxy() != NULL) {
      return args->at(0);
    } else {
      ReportMessage("not_isvar", Vector<const char*>::empty());
      *ok = false;
      return NULL;
    }
  }

  // Check that the expected number of arguments are being passed.
  if (function != NULL &&
      function->nargs != -1 &&
      function->nargs != args->length()) {
    ReportMessage("illegal_access", Vector<const char*>::empty());
    *ok = false;
    return NULL;
  }

  // Check that the function is defined if it's an inline runtime call.
  if (function == NULL && name->Get(0) == '_') {
    ParserTraits::ReportMessage("not_defined",
                                Vector<Handle<String> >(&name, 1));
    *ok = false;
    return NULL;
  }

  // We have a valid intrinsics call or a call to a builtin.
  return factory()->NewCallRuntime(name, function, args, pos);
}


Literal* Parser::GetLiteralUndefined(int position) {
  return factory()->NewLiteral(
      isolate()->factory()->undefined_value(), position);
}


void Parser::CheckConflictingVarDeclarations(Scope* scope, bool* ok) {
  Declaration* decl = scope->CheckConflictingVarDeclarations();
  if (decl != NULL) {
    // In harmony mode we treat conflicting variable bindinds as early
    // errors. See ES5 16 for a definition of early errors.
    Handle<String> name = decl->proxy()->name();
    SmartArrayPointer<char> c_string = name->ToCString(DISALLOW_NULLS);
    const char* elms[2] = { "Variable", c_string.get() };
    Vector<const char*> args(elms, 2);
    int position = decl->proxy()->position();
    Scanner::Location location = position == RelocInfo::kNoPosition
        ? Scanner::Location::invalid()
        : Scanner::Location(position, position + 1);
    ParserTraits::ReportMessageAt(location, "redeclaration", args);
    *ok = false;
  }
}


// ----------------------------------------------------------------------------
// Parser support


bool Parser::TargetStackContainsLabel(Handle<String> label) {
  for (Target* t = target_stack_; t != NULL; t = t->previous()) {
    BreakableStatement* stat = t->node()->AsBreakableStatement();
    if (stat != NULL && ContainsLabel(stat->labels(), label))
      return true;
  }
  return false;
}


BreakableStatement* Parser::LookupBreakTarget(Handle<String> label, bool* ok) {
  bool anonymous = label.is_null();
  for (Target* t = target_stack_; t != NULL; t = t->previous()) {
    BreakableStatement* stat = t->node()->AsBreakableStatement();
    if (stat == NULL) continue;
    if ((anonymous && stat->is_target_for_anonymous()) ||
        (!anonymous && ContainsLabel(stat->labels(), label))) {
      RegisterTargetUse(stat->break_target(), t->previous());
      return stat;
    }
  }
  return NULL;
}


IterationStatement* Parser::LookupContinueTarget(Handle<String> label,
                                                 bool* ok) {
  bool anonymous = label.is_null();
  for (Target* t = target_stack_; t != NULL; t = t->previous()) {
    IterationStatement* stat = t->node()->AsIterationStatement();
    if (stat == NULL) continue;

    ASSERT(stat->is_target_for_anonymous());
    if (anonymous || ContainsLabel(stat->labels(), label)) {
      RegisterTargetUse(stat->continue_target(), t->previous());
      return stat;
    }
  }
  return NULL;
}


void Parser::RegisterTargetUse(Label* target, Target* stop) {
  // Register that a break target found at the given stop in the
  // target stack has been used from the top of the target stack. Add
  // the break target to any TargetCollectors passed on the stack.
  for (Target* t = target_stack_; t != stop; t = t->previous()) {
    TargetCollector* collector = t->node()->AsTargetCollector();
    if (collector != NULL) collector->AddTarget(target, zone());
  }
}


// ----------------------------------------------------------------------------
// Regular expressions


RegExpParser::RegExpParser(FlatStringReader* in,
                           Handle<String>* error,
                           bool multiline,
                           Zone* zone)
    : isolate_(zone->isolate()),
      zone_(zone),
      error_(error),
      captures_(NULL),
      in_(in),
      current_(kEndMarker),
      next_pos_(0),
      capture_count_(0),
      has_more_(true),
      multiline_(multiline),
      simple_(false),
      contains_anchor_(false),
      is_scanned_for_captures_(false),
      failed_(false) {
  Advance();
}


uc32 RegExpParser::Next() {
  if (has_next()) {
    return in()->Get(next_pos_);
  } else {
    return kEndMarker;
  }
}


void RegExpParser::Advance() {
  if (next_pos_ < in()->length()) {
    StackLimitCheck check(isolate());
    if (check.HasOverflowed()) {
      ReportError(CStrVector(Isolate::kStackOverflowMessage));
    } else if (zone()->excess_allocation()) {
      ReportError(CStrVector("Regular expression too large"));
    } else {
      current_ = in()->Get(next_pos_);
      next_pos_++;
    }
  } else {
    current_ = kEndMarker;
    has_more_ = false;
  }
}


void RegExpParser::Reset(int pos) {
  next_pos_ = pos;
  has_more_ = (pos < in()->length());
  Advance();
}


void RegExpParser::Advance(int dist) {
  next_pos_ += dist - 1;
  Advance();
}


bool RegExpParser::simple() {
  return simple_;
}


RegExpTree* RegExpParser::ReportError(Vector<const char> message) {
  failed_ = true;
  *error_ = isolate()->factory()->NewStringFromAscii(message, NOT_TENURED);
  ASSERT(!error_->is_null());
  // Zip to the end to make sure the no more input is read.
  current_ = kEndMarker;
  next_pos_ = in()->length();
  return NULL;
}


// Pattern ::
//   Disjunction
RegExpTree* RegExpParser::ParsePattern() {
  RegExpTree* result = ParseDisjunction(CHECK_FAILED);
  ASSERT(!has_more());
  // If the result of parsing is a literal string atom, and it has the
  // same length as the input, then the atom is identical to the input.
  if (result->IsAtom() && result->AsAtom()->length() == in()->length()) {
    simple_ = true;
  }
  return result;
}


// Disjunction ::
//   Alternative
//   Alternative | Disjunction
// Alternative ::
//   [empty]
//   Term Alternative
// Term ::
//   Assertion
//   Atom
//   Atom Quantifier
RegExpTree* RegExpParser::ParseDisjunction() {
  // Used to store current state while parsing subexpressions.
  RegExpParserState initial_state(NULL, INITIAL, 0, zone());
  RegExpParserState* stored_state = &initial_state;
  // Cache the builder in a local variable for quick access.
  RegExpBuilder* builder = initial_state.builder();
  while (true) {
    switch (current()) {
    case kEndMarker:
      if (stored_state->IsSubexpression()) {
        // Inside a parenthesized group when hitting end of input.
        ReportError(CStrVector("Unterminated group") CHECK_FAILED);
      }
      ASSERT_EQ(INITIAL, stored_state->group_type());
      // Parsing completed successfully.
      return builder->ToRegExp();
    case ')': {
      if (!stored_state->IsSubexpression()) {
        ReportError(CStrVector("Unmatched ')'") CHECK_FAILED);
      }
      ASSERT_NE(INITIAL, stored_state->group_type());

      Advance();
      // End disjunction parsing and convert builder content to new single
      // regexp atom.
      RegExpTree* body = builder->ToRegExp();

      int end_capture_index = captures_started();

      int capture_index = stored_state->capture_index();
      SubexpressionType group_type = stored_state->group_type();

      // Restore previous state.
      stored_state = stored_state->previous_state();
      builder = stored_state->builder();

      // Build result of subexpression.
      if (group_type == CAPTURE) {
        RegExpCapture* capture = new(zone()) RegExpCapture(body, capture_index);
        captures_->at(capture_index - 1) = capture;
        body = capture;
      } else if (group_type != GROUPING) {
        ASSERT(group_type == POSITIVE_LOOKAHEAD ||
               group_type == NEGATIVE_LOOKAHEAD);
        bool is_positive = (group_type == POSITIVE_LOOKAHEAD);
        body = new(zone()) RegExpLookahead(body,
                                   is_positive,
                                   end_capture_index - capture_index,
                                   capture_index);
      }
      builder->AddAtom(body);
      // For compatability with JSC and ES3, we allow quantifiers after
      // lookaheads, and break in all cases.
      break;
    }
    case '|': {
      Advance();
      builder->NewAlternative();
      continue;
    }
    case '*':
    case '+':
    case '?':
      return ReportError(CStrVector("Nothing to repeat"));
    case '^': {
      Advance();
      if (multiline_) {
        builder->AddAssertion(
            new(zone()) RegExpAssertion(RegExpAssertion::START_OF_LINE));
      } else {
        builder->AddAssertion(
            new(zone()) RegExpAssertion(RegExpAssertion::START_OF_INPUT));
        set_contains_anchor();
      }
      continue;
    }
    case '$': {
      Advance();
      RegExpAssertion::AssertionType assertion_type =
          multiline_ ? RegExpAssertion::END_OF_LINE :
                       RegExpAssertion::END_OF_INPUT;
      builder->AddAssertion(new(zone()) RegExpAssertion(assertion_type));
      continue;
    }
    case '.': {
      Advance();
      // everything except \x0a, \x0d, \u2028 and \u2029
      ZoneList<CharacterRange>* ranges =
          new(zone()) ZoneList<CharacterRange>(2, zone());
      CharacterRange::AddClassEscape('.', ranges, zone());
      RegExpTree* atom = new(zone()) RegExpCharacterClass(ranges, false);
      builder->AddAtom(atom);
      break;
    }
    case '(': {
      SubexpressionType subexpr_type = CAPTURE;
      Advance();
      if (current() == '?') {
        switch (Next()) {
          case ':':
            subexpr_type = GROUPING;
            break;
          case '=':
            subexpr_type = POSITIVE_LOOKAHEAD;
            break;
          case '!':
            subexpr_type = NEGATIVE_LOOKAHEAD;
            break;
          default:
            ReportError(CStrVector("Invalid group") CHECK_FAILED);
            break;
        }
        Advance(2);
      } else {
        if (captures_ == NULL) {
          captures_ = new(zone()) ZoneList<RegExpCapture*>(2, zone());
        }
        if (captures_started() >= kMaxCaptures) {
          ReportError(CStrVector("Too many captures") CHECK_FAILED);
        }
        captures_->Add(NULL, zone());
      }
      // Store current state and begin new disjunction parsing.
      stored_state = new(zone()) RegExpParserState(stored_state, subexpr_type,
                                                   captures_started(), zone());
      builder = stored_state->builder();
      continue;
    }
    case '[': {
      RegExpTree* atom = ParseCharacterClass(CHECK_FAILED);
      builder->AddAtom(atom);
      break;
    }
    // Atom ::
    //   \ AtomEscape
    case '\\':
      switch (Next()) {
      case kEndMarker:
        return ReportError(CStrVector("\\ at end of pattern"));
      case 'b':
        Advance(2);
        builder->AddAssertion(
            new(zone()) RegExpAssertion(RegExpAssertion::BOUNDARY));
        continue;
      case 'B':
        Advance(2);
        builder->AddAssertion(
            new(zone()) RegExpAssertion(RegExpAssertion::NON_BOUNDARY));
        continue;
      // AtomEscape ::
      //   CharacterClassEscape
      //
      // CharacterClassEscape :: one of
      //   d D s S w W
      case 'd': case 'D': case 's': case 'S': case 'w': case 'W': {
        uc32 c = Next();
        Advance(2);
        ZoneList<CharacterRange>* ranges =
            new(zone()) ZoneList<CharacterRange>(2, zone());
        CharacterRange::AddClassEscape(c, ranges, zone());
        RegExpTree* atom = new(zone()) RegExpCharacterClass(ranges, false);
        builder->AddAtom(atom);
        break;
      }
      case '1': case '2': case '3': case '4': case '5': case '6':
      case '7': case '8': case '9': {
        int index = 0;
        if (ParseBackReferenceIndex(&index)) {
          RegExpCapture* capture = NULL;
          if (captures_ != NULL && index <= captures_->length()) {
            capture = captures_->at(index - 1);
          }
          if (capture == NULL) {
            builder->AddEmpty();
            break;
          }
          RegExpTree* atom = new(zone()) RegExpBackReference(capture);
          builder->AddAtom(atom);
          break;
        }
        uc32 first_digit = Next();
        if (first_digit == '8' || first_digit == '9') {
          // Treat as identity escape
          builder->AddCharacter(first_digit);
          Advance(2);
          break;
        }
      }
      // FALLTHROUGH
      case '0': {
        Advance();
        uc32 octal = ParseOctalLiteral();
        builder->AddCharacter(octal);
        break;
      }
      // ControlEscape :: one of
      //   f n r t v
      case 'f':
        Advance(2);
        builder->AddCharacter('\f');
        break;
      case 'n':
        Advance(2);
        builder->AddCharacter('\n');
        break;
      case 'r':
        Advance(2);
        builder->AddCharacter('\r');
        break;
      case 't':
        Advance(2);
        builder->AddCharacter('\t');
        break;
      case 'v':
        Advance(2);
        builder->AddCharacter('\v');
        break;
      case 'c': {
        Advance();
        uc32 controlLetter = Next();
        // Special case if it is an ASCII letter.
        // Convert lower case letters to uppercase.
        uc32 letter = controlLetter & ~('a' ^ 'A');
        if (letter < 'A' || 'Z' < letter) {
          // controlLetter is not in range 'A'-'Z' or 'a'-'z'.
          // This is outside the specification. We match JSC in
          // reading the backslash as a literal character instead
          // of as starting an escape.
          builder->AddCharacter('\\');
        } else {
          Advance(2);
          builder->AddCharacter(controlLetter & 0x1f);
        }
        break;
      }
      case 'x': {
        Advance(2);
        uc32 value;
        if (ParseHexEscape(2, &value)) {
          builder->AddCharacter(value);
        } else {
          builder->AddCharacter('x');
        }
        break;
      }
      case 'u': {
        Advance(2);
        uc32 value;
        if (ParseHexEscape(4, &value)) {
          builder->AddCharacter(value);
        } else {
          builder->AddCharacter('u');
        }
        break;
      }
      default:
        // Identity escape.
        builder->AddCharacter(Next());
        Advance(2);
        break;
      }
      break;
    case '{': {
      int dummy;
      if (ParseIntervalQuantifier(&dummy, &dummy)) {
        ReportError(CStrVector("Nothing to repeat") CHECK_FAILED);
      }
      // fallthrough
    }
    default:
      builder->AddCharacter(current());
      Advance();
      break;
    }  // end switch(current())

    int min;
    int max;
    switch (current()) {
    // QuantifierPrefix ::
    //   *
    //   +
    //   ?
    //   {
    case '*':
      min = 0;
      max = RegExpTree::kInfinity;
      Advance();
      break;
    case '+':
      min = 1;
      max = RegExpTree::kInfinity;
      Advance();
      break;
    case '?':
      min = 0;
      max = 1;
      Advance();
      break;
    case '{':
      if (ParseIntervalQuantifier(&min, &max)) {
        if (max < min) {
          ReportError(CStrVector("numbers out of order in {} quantifier.")
                      CHECK_FAILED);
        }
        break;
      } else {
        continue;
      }
    default:
      continue;
    }
    RegExpQuantifier::QuantifierType quantifier_type = RegExpQuantifier::GREEDY;
    if (current() == '?') {
      quantifier_type = RegExpQuantifier::NON_GREEDY;
      Advance();
    } else if (FLAG_regexp_possessive_quantifier && current() == '+') {
      // FLAG_regexp_possessive_quantifier is a debug-only flag.
      quantifier_type = RegExpQuantifier::POSSESSIVE;
      Advance();
    }
    builder->AddQuantifierToAtom(min, max, quantifier_type);
  }
}


#ifdef DEBUG
// Currently only used in an ASSERT.
static bool IsSpecialClassEscape(uc32 c) {
  switch (c) {
    case 'd': case 'D':
    case 's': case 'S':
    case 'w': case 'W':
      return true;
    default:
      return false;
  }
}
#endif


// In order to know whether an escape is a backreference or not we have to scan
// the entire regexp and find the number of capturing parentheses.  However we
// don't want to scan the regexp twice unless it is necessary.  This mini-parser
// is called when needed.  It can see the difference between capturing and
// noncapturing parentheses and can skip character classes and backslash-escaped
// characters.
void RegExpParser::ScanForCaptures() {
  // Start with captures started previous to current position
  int capture_count = captures_started();
  // Add count of captures after this position.
  int n;
  while ((n = current()) != kEndMarker) {
    Advance();
    switch (n) {
      case '\\':
        Advance();
        break;
      case '[': {
        int c;
        while ((c = current()) != kEndMarker) {
          Advance();
          if (c == '\\') {
            Advance();
          } else {
            if (c == ']') break;
          }
        }
        break;
      }
      case '(':
        if (current() != '?') capture_count++;
        break;
    }
  }
  capture_count_ = capture_count;
  is_scanned_for_captures_ = true;
}


bool RegExpParser::ParseBackReferenceIndex(int* index_out) {
  ASSERT_EQ('\\', current());
  ASSERT('1' <= Next() && Next() <= '9');
  // Try to parse a decimal literal that is no greater than the total number
  // of left capturing parentheses in the input.
  int start = position();
  int value = Next() - '0';
  Advance(2);
  while (true) {
    uc32 c = current();
    if (IsDecimalDigit(c)) {
      value = 10 * value + (c - '0');
      if (value > kMaxCaptures) {
        Reset(start);
        return false;
      }
      Advance();
    } else {
      break;
    }
  }
  if (value > captures_started()) {
    if (!is_scanned_for_captures_) {
      int saved_position = position();
      ScanForCaptures();
      Reset(saved_position);
    }
    if (value > capture_count_) {
      Reset(start);
      return false;
    }
  }
  *index_out = value;
  return true;
}


// QuantifierPrefix ::
//   { DecimalDigits }
//   { DecimalDigits , }
//   { DecimalDigits , DecimalDigits }
//
// Returns true if parsing succeeds, and set the min_out and max_out
// values. Values are truncated to RegExpTree::kInfinity if they overflow.
bool RegExpParser::ParseIntervalQuantifier(int* min_out, int* max_out) {
  ASSERT_EQ(current(), '{');
  int start = position();
  Advance();
  int min = 0;
  if (!IsDecimalDigit(current())) {
    Reset(start);
    return false;
  }
  while (IsDecimalDigit(current())) {
    int next = current() - '0';
    if (min > (RegExpTree::kInfinity - next) / 10) {
      // Overflow. Skip past remaining decimal digits and return -1.
      do {
        Advance();
      } while (IsDecimalDigit(current()));
      min = RegExpTree::kInfinity;
      break;
    }
    min = 10 * min + next;
    Advance();
  }
  int max = 0;
  if (current() == '}') {
    max = min;
    Advance();
  } else if (current() == ',') {
    Advance();
    if (current() == '}') {
      max = RegExpTree::kInfinity;
      Advance();
    } else {
      while (IsDecimalDigit(current())) {
        int next = current() - '0';
        if (max > (RegExpTree::kInfinity - next) / 10) {
          do {
            Advance();
          } while (IsDecimalDigit(current()));
          max = RegExpTree::kInfinity;
          break;
        }
        max = 10 * max + next;
        Advance();
      }
      if (current() != '}') {
        Reset(start);
        return false;
      }
      Advance();
    }
  } else {
    Reset(start);
    return false;
  }
  *min_out = min;
  *max_out = max;
  return true;
}


uc32 RegExpParser::ParseOctalLiteral() {
  ASSERT(('0' <= current() && current() <= '7') || current() == kEndMarker);
  // For compatibility with some other browsers (not all), we parse
  // up to three octal digits with a value below 256.
  uc32 value = current() - '0';
  Advance();
  if ('0' <= current() && current() <= '7') {
    value = value * 8 + current() - '0';
    Advance();
    if (value < 32 && '0' <= current() && current() <= '7') {
      value = value * 8 + current() - '0';
      Advance();
    }
  }
  return value;
}


bool RegExpParser::ParseHexEscape(int length, uc32 *value) {
  int start = position();
  uc32 val = 0;
  bool done = false;
  for (int i = 0; !done; i++) {
    uc32 c = current();
    int d = HexValue(c);
    if (d < 0) {
      Reset(start);
      return false;
    }
    val = val * 16 + d;
    Advance();
    if (i == length - 1) {
      done = true;
    }
  }
  *value = val;
  return true;
}


uc32 RegExpParser::ParseClassCharacterEscape() {
  ASSERT(current() == '\\');
  ASSERT(has_next() && !IsSpecialClassEscape(Next()));
  Advance();
  switch (current()) {
    case 'b':
      Advance();
      return '\b';
    // ControlEscape :: one of
    //   f n r t v
    case 'f':
      Advance();
      return '\f';
    case 'n':
      Advance();
      return '\n';
    case 'r':
      Advance();
      return '\r';
    case 't':
      Advance();
      return '\t';
    case 'v':
      Advance();
      return '\v';
    case 'c': {
      uc32 controlLetter = Next();
      uc32 letter = controlLetter & ~('A' ^ 'a');
      // For compatibility with JSC, inside a character class
      // we also accept digits and underscore as control characters.
      if ((controlLetter >= '0' && controlLetter <= '9') ||
          controlLetter == '_' ||
          (letter >= 'A' && letter <= 'Z')) {
        Advance(2);
        // Control letters mapped to ASCII control characters in the range
        // 0x00-0x1f.
        return controlLetter & 0x1f;
      }
      // We match JSC in reading the backslash as a literal
      // character instead of as starting an escape.
      return '\\';
    }
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7':
      // For compatibility, we interpret a decimal escape that isn't
      // a back reference (and therefore either \0 or not valid according
      // to the specification) as a 1..3 digit octal character code.
      return ParseOctalLiteral();
    case 'x': {
      Advance();
      uc32 value;
      if (ParseHexEscape(2, &value)) {
        return value;
      }
      // If \x is not followed by a two-digit hexadecimal, treat it
      // as an identity escape.
      return 'x';
    }
    case 'u': {
      Advance();
      uc32 value;
      if (ParseHexEscape(4, &value)) {
        return value;
      }
      // If \u is not followed by a four-digit hexadecimal, treat it
      // as an identity escape.
      return 'u';
    }
    default: {
      // Extended identity escape. We accept any character that hasn't
      // been matched by a more specific case, not just the subset required
      // by the ECMAScript specification.
      uc32 result = current();
      Advance();
      return result;
    }
  }
  return 0;
}


CharacterRange RegExpParser::ParseClassAtom(uc16* char_class) {
  ASSERT_EQ(0, *char_class);
  uc32 first = current();
  if (first == '\\') {
    switch (Next()) {
      case 'w': case 'W': case 'd': case 'D': case 's': case 'S': {
        *char_class = Next();
        Advance(2);
        return CharacterRange::Singleton(0);  // Return dummy value.
      }
      case kEndMarker:
        return ReportError(CStrVector("\\ at end of pattern"));
      default:
        uc32 c = ParseClassCharacterEscape(CHECK_FAILED);
        return CharacterRange::Singleton(c);
    }
  } else {
    Advance();
    return CharacterRange::Singleton(first);
  }
}


static const uc16 kNoCharClass = 0;

// Adds range or pre-defined character class to character ranges.
// If char_class is not kInvalidClass, it's interpreted as a class
// escape (i.e., 's' means whitespace, from '\s').
static inline void AddRangeOrEscape(ZoneList<CharacterRange>* ranges,
                                    uc16 char_class,
                                    CharacterRange range,
                                    Zone* zone) {
  if (char_class != kNoCharClass) {
    CharacterRange::AddClassEscape(char_class, ranges, zone);
  } else {
    ranges->Add(range, zone);
  }
}


RegExpTree* RegExpParser::ParseCharacterClass() {
  static const char* kUnterminated = "Unterminated character class";
  static const char* kRangeOutOfOrder = "Range out of order in character class";

  ASSERT_EQ(current(), '[');
  Advance();
  bool is_negated = false;
  if (current() == '^') {
    is_negated = true;
    Advance();
  }
  ZoneList<CharacterRange>* ranges =
      new(zone()) ZoneList<CharacterRange>(2, zone());
  while (has_more() && current() != ']') {
    uc16 char_class = kNoCharClass;
    CharacterRange first = ParseClassAtom(&char_class CHECK_FAILED);
    if (current() == '-') {
      Advance();
      if (current() == kEndMarker) {
        // If we reach the end we break out of the loop and let the
        // following code report an error.
        break;
      } else if (current() == ']') {
        AddRangeOrEscape(ranges, char_class, first, zone());
        ranges->Add(CharacterRange::Singleton('-'), zone());
        break;
      }
      uc16 char_class_2 = kNoCharClass;
      CharacterRange next = ParseClassAtom(&char_class_2 CHECK_FAILED);
      if (char_class != kNoCharClass || char_class_2 != kNoCharClass) {
        // Either end is an escaped character class. Treat the '-' verbatim.
        AddRangeOrEscape(ranges, char_class, first, zone());
        ranges->Add(CharacterRange::Singleton('-'), zone());
        AddRangeOrEscape(ranges, char_class_2, next, zone());
        continue;
      }
      if (first.from() > next.to()) {
        return ReportError(CStrVector(kRangeOutOfOrder) CHECK_FAILED);
      }
      ranges->Add(CharacterRange::Range(first.from(), next.to()), zone());
    } else {
      AddRangeOrEscape(ranges, char_class, first, zone());
    }
  }
  if (!has_more()) {
    return ReportError(CStrVector(kUnterminated) CHECK_FAILED);
  }
  Advance();
  if (ranges->length() == 0) {
    ranges->Add(CharacterRange::Everything(), zone());
    is_negated = !is_negated;
  }
  return new(zone()) RegExpCharacterClass(ranges, is_negated);
}


// ----------------------------------------------------------------------------
// The Parser interface.

ScriptDataImpl::~ScriptDataImpl() {
  if (owns_store_) store_.Dispose();
}


int ScriptDataImpl::Length() {
  return store_.length() * sizeof(unsigned);
}


const char* ScriptDataImpl::Data() {
  return reinterpret_cast<const char*>(store_.start());
}


bool ScriptDataImpl::HasError() {
  return has_error();
}


void ScriptDataImpl::Initialize() {
  // Prepares state for use.
  if (store_.length() >= PreparseDataConstants::kHeaderSize) {
    function_index_ = PreparseDataConstants::kHeaderSize;
    int symbol_data_offset = PreparseDataConstants::kHeaderSize
        + store_[PreparseDataConstants::kFunctionsSizeOffset];
    if (store_.length() > symbol_data_offset) {
      symbol_data_ = reinterpret_cast<byte*>(&store_[symbol_data_offset]);
    } else {
      // Partial preparse causes no symbol information.
      symbol_data_ = reinterpret_cast<byte*>(&store_[0] + store_.length());
    }
    symbol_data_end_ = reinterpret_cast<byte*>(&store_[0] + store_.length());
  }
}


int ScriptDataImpl::ReadNumber(byte** source) {
  // Reads a number from symbol_data_ in base 128. The most significant
  // bit marks that there are more digits.
  // If the first byte is 0x80 (kNumberTerminator), it would normally
  // represent a leading zero. Since that is useless, and therefore won't
  // appear as the first digit of any actual value, it is used to
  // mark the end of the input stream.
  byte* data = *source;
  if (data >= symbol_data_end_) return -1;
  byte input = *data;
  if (input == PreparseDataConstants::kNumberTerminator) {
    // End of stream marker.
    return -1;
  }
  int result = input & 0x7f;
  data++;
  while ((input & 0x80u) != 0) {
    if (data >= symbol_data_end_) return -1;
    input = *data;
    result = (result << 7) | (input & 0x7f);
    data++;
  }
  *source = data;
  return result;
}


// Create a Scanner for the preparser to use as input, and preparse the source.
ScriptDataImpl* PreParserApi::PreParse(Isolate* isolate,
                                       Utf16CharacterStream* source) {
  CompleteParserRecorder recorder;
  HistogramTimerScope timer(isolate->counters()->pre_parse());
  Scanner scanner(isolate->unicode_cache());
  intptr_t stack_limit = isolate->stack_guard()->real_climit();
  PreParser preparser(&scanner, &recorder, stack_limit);
  preparser.set_allow_lazy(true);
  preparser.set_allow_generators(FLAG_harmony_generators);
  preparser.set_allow_for_of(FLAG_harmony_iteration);
  preparser.set_allow_harmony_scoping(FLAG_harmony_scoping);
  preparser.set_allow_harmony_numeric_literals(FLAG_harmony_numeric_literals);
  scanner.Initialize(source);
  PreParser::PreParseResult result = preparser.PreParseProgram();
  if (result == PreParser::kPreParseStackOverflow) {
    isolate->StackOverflow();
    return NULL;
  }

  // Extract the accumulated data from the recorder as a single
  // contiguous vector that we are responsible for disposing.
  Vector<unsigned> store = recorder.ExtractData();
  return new ScriptDataImpl(store);
}


bool RegExpParser::ParseRegExp(FlatStringReader* input,
                               bool multiline,
                               RegExpCompileData* result,
                               Zone* zone) {
  ASSERT(result != NULL);
  RegExpParser parser(input, &result->error, multiline, zone);
  RegExpTree* tree = parser.ParsePattern();
  if (parser.failed()) {
    ASSERT(tree == NULL);
    ASSERT(!result->error.is_null());
  } else {
    ASSERT(tree != NULL);
    ASSERT(result->error.is_null());
    result->tree = tree;
    int capture_count = parser.captures_started();
    result->simple = tree->IsAtom() && parser.simple() && capture_count == 0;
    result->contains_anchor = parser.contains_anchor();
    result->capture_count = capture_count;
  }
  return !parser.failed();
}


bool Parser::Parse() {
  ASSERT(info()->function() == NULL);
  FunctionLiteral* result = NULL;
  if (info()->is_lazy()) {
    ASSERT(!info()->is_eval());
    if (info()->shared_info()->is_function()) {
      result = ParseLazy();
    } else {
      result = ParseProgram();
    }
  } else {
    SetCachedData(info()->cached_data(), info()->cached_data_mode());
    if (info()->cached_data_mode() == CONSUME_CACHED_DATA &&
        (*info()->cached_data())->has_error()) {
      ScriptDataImpl* cached_data = *(info()->cached_data());
      Scanner::Location loc = cached_data->MessageLocation();
      const char* message = cached_data->BuildMessage();
      Vector<const char*> args = cached_data->BuildArgs();
      ParserTraits::ReportMessageAt(loc, message, args);
      DeleteArray(message);
      for (int i = 0; i < args.length(); i++) {
        DeleteArray(args[i]);
      }
      DeleteArray(args.start());
      ASSERT(info()->isolate()->has_pending_exception());
    } else {
      result = ParseProgram();
    }
  }
  info()->SetFunction(result);
  return (result != NULL);
}

} }  // namespace v8::internal
