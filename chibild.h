#pragma once

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/BinaryFormat/Magic.h"
#include "llvm/Object/ELF.h"
#include "llvm/Object/ELFTypes.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MemoryBuffer.h"
#include "tbb/concurrent_hash_map.h"

#include <cstdlib>
#include <cstdint>
#include <string>

using llvm::ArrayRef;
using llvm::ErrorOr;
using llvm::Expected;
using llvm::MemoryBufferRef;
using llvm::SmallVector;
using llvm::StringRef;
using llvm::Twine;
using llvm::object::ELF64LE;
using llvm::object::ELFFile;

class Symbol;
class InputFile;
class ObjectFile;
class InputSection;

struct Config {
  StringRef output;
};

extern Config config;

[[noreturn]] inline void error(const Twine &msg) {
  llvm::errs() << msg << "\n";
  exit(1);
}

template <class T> T check(ErrorOr<T> e) {
  if (auto ec = e.getError())
    error(ec.message());
  return std::move(*e);
}

template <class T> T check(Expected<T> e) {
  if (!e)
    error(llvm::toString(e.takeError()));
  return std::move(*e);
}

template <class T>
T check2(ErrorOr<T> e, llvm::function_ref<std::string()> prefix) {
  if (auto ec = e.getError())
    error(prefix() + ": " + ec.message());
  return std::move(*e);
}

template <class T>
T check2(Expected<T> e, llvm::function_ref<std::string()> prefix) {
  if (!e)
    error(prefix() + ": " + toString(e.takeError()));
  return std::move(*e);
}

inline std::string toString(const Twine &s) { return s.str(); }

#define CHECK(E, S) check2((E), [&] { return toString(S); })

class Symbol;
class SymbolTable;
class InputSection;
class ObjectFile;

class Symbol {
public:
  StringRef name;
  ObjectFile *file;
};

namespace tbb {
template<>
struct tbb_hash_compare<StringRef> {
  static size_t hash(const StringRef& k) {
    return llvm::hash_value(k);
  }

  static bool equal(const StringRef& k1, const StringRef& k2) {
    return k1 == k2;
  }
};
}

class SymbolTable {
public:
  void add(StringRef key, StringRef val);
  StringRef get(StringRef key);

private:
  typedef tbb::concurrent_hash_map<StringRef, StringRef> MapType;

  MapType map;
};

class InputSection {
public:
  InputSection(ObjectFile *file, ELF64LE::Shdr *hdr, StringRef name);

  void writeTo(uint8_t *buf);

  uint64_t getOffset() const;
  uint64_t getVA() const;

  InputFile *file;
};

class ObjectFile {
public:
  ObjectFile(MemoryBufferRef mb);

  void parse();
  void register_defined_symbols();
  void register_undefined_symbols();
  StringRef getFilename();
 
private:
  MemoryBufferRef mb;
  std::vector<InputSection> sections;
  std::vector<Symbol *> symbols;
  bool is_alive = false;
};

MemoryBufferRef readFile(StringRef path);

std::string toString(ObjectFile *);

extern SymbolTable symbol_table;

void write();
