#include <string>
#include <vector>

#include <ATen/core/ivalue.h>
#include <c10/util/ArrayRef.h>
#include <torch/csrc/utils/disallow_copy.h>

namespace torch {
namespace jit {

// See Python's pickletools.py for a detailed description of each of these codes
enum class OpCode : char {
  MARK = '(',
  STOP = '.',
  POP = '0',
  POP_MARK = '1',
  DUP = '2',
  FLOAT = 'F',
  INT = 'I',
  BININT = 'J',
  BININT1 = 'K',
  LONG = 'L',
  BININT2 = 'M',
  NONE = 'N',
  PERSID = 'P',
  BINPERSID = 'Q',
  REDUCE = 'R',
  STRING = 'S',
  BINSTRING = 'T',
  SHORT_BINSTRING = 'U',
  UNICODE = 'V',
  BINUNICODE = 'X',
  APPEND = 'a',
  BUILD = 'b',
  GLOBAL = 'c',
  DICT = 'd',
  EMPTY_DICT = '}',
  APPENDS = 'e',
  GET = 'g',
  BINGET = 'h',
  INST = 'i',
  LONG_BINGET = 'j',
  LIST = 'l',
  EMPTY_LIST = ']',
  OBJ = 'o',
  PUT = 'p',
  BINPUT = 'q',
  LONG_BINPUT = 'r',
  SETITEM = 's',
  TUPLE = 't',
  EMPTY_TUPLE = ')',
  SETITEMS = 'u',
  BINFLOAT = 'G',

  // Protocol 2
  PROTO = '\x80',
  NEWOBJ = '\x81',
  EXT1 = '\x82',
  EXT2 = '\x83',
  EXT4 = '\x84',
  TUPLE1 = '\x85',
  TUPLE2 = '\x86',
  TUPLE3 = '\x87',
  NEWTRUE = '\x88',
  NEWFALSE = '\x89',
  LONG1 = '\x8a',
  LONG4 = '\x8b',

  // Protocol 3 (Python 3.x)
  BINBYTES = 'B',
  SHORT_BINBYTES = 'C',

  // Protocol 4
  SHORT_BINUNICODE = '\x8c',
  BINUNICODE8 = '\x8d',
  BINBYTES8 = '\x8e',
  EMPTY_SET = '\x8f',
  ADDITEMS = '\x90',
  FROZENSET = '\x91',
  NEWOBJ_EX = '\x92',
  STACK_GLOBAL = '\x93',
  MEMOIZE = '\x94',
  FRAME = '\x95'
};

enum PicklerClass : uint8_t {
  // A reference to the tensor table
  TENSOR = 0,
  // List[int]
  INTLIST = 1,
  // A tensor that is stored entirely in the pickle file
  LITERAL_TENSOR = 2
};

using ::c10::IValue;

class Pickler {
  TH_DISALLOW_COPY_AND_ASSIGN(Pickler);

 public:
  Pickler(std::vector<at::Tensor>* tensor_table = nullptr)
      : tensor_table_(tensor_table) {}

  const std::vector<char>& stack();
  void start();
  void finish();
  void addIValue(const IValue& ivalue);

 private:
  void pushBinGet(uint32_t memo_id);
  void pushMemoizedString(const IValue& ivalue);
  void pushString(const std::string& string);
  void pushTensor(const IValue& ivalue);
  void pushDouble(const IValue& ivalue);
  void pushMemoization(const void* item);
  void pushMemoization(const IValue& ivalue);
  void pushList(const IValue& ivalue);
  void pushIntList(const IValue& ivalue);
  void pushTuple(const IValue& ivalue);
  void pushDict(const IValue& ivalue);
  void pushClass(PicklerClass cls);
  const void* getPointer(const IValue& ivalue);
  void pushLiteralTensor(const IValue& ivalue);
  void pushTensorReference(const IValue& ivalue);


  void pushUint8(uint8_t value);
  void pushOpCode(OpCode value);
  void pushUint32(uint32_t value);
  void pushInt32(int32_t value);

  // Stack of opcodes/data
  std::vector<char> stack_;

  // Memoization of IValues that have been written (index in table is used for
  // BINPUT opcodes) to enable shared references
  std::unordered_map<const void*, uint32_t> memo_;

  // External table of tensors to serialize. If this is missing, then tensors
  // are serialized directly into the pickle
  std::vector<at::Tensor>* tensor_table_;

  // TODO: only use this if necessary (add a pass to find all shared ivalues,
  // and only memoize those)
  uint32_t memo_id = 0;
};

class Unpickler {
  TH_DISALLOW_COPY_AND_ASSIGN(Unpickler);

 public:
  Unpickler(
      void* data,
      size_t size,
      const std::vector<at::Tensor>* tensor_table)
      : bytes_(static_cast<const uint8_t*>(data)),
        end_ptr_(bytes_ + size),
        tensor_table_(tensor_table) {}

  std::vector<IValue> parse_ivalue_list();

 private:
  // No arguments ensures that a template arugment must be specified
  // so that the number of bytes read / type read is explicit
  template <typename T>
  T read() {
    AT_CHECK(
        bytes_ + sizeof(T) <= end_ptr_,
        "Unpickler overran buffer while reading a value");
    T item;
    std::memcpy(&item, bytes_, sizeof(T));
    bytes_ += sizeof(T);
    return item;
  }

  double readFloat();
  void run();
  OpCode readInstruction();
  std::string readString();
  OpCode readOpCode();
  void readList();

  std::vector<IValue> stack_;
  std::vector<IValue> memo_;
  std::vector<size_t> marks_;
  const uint8_t* bytes_;
  const uint8_t* end_ptr_;
  const std::vector<at::Tensor>* tensor_table_;
  OpCode last_opcode_;
};

} // namespace jit
} // namespace torch
