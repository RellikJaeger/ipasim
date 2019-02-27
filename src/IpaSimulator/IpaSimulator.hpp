#include <LIEF/LIEF.hpp>
#include <Windows.h>
#include <string>
#include <unicorn/unicorn.h>

class DynamicLoader;

class LoadedLibrary {
public:
  virtual ~LoadedLibrary() = default;

  uint64_t StartAddress, Size;

  // TODO: Check that the found symbol is inside range [StartAddress, +Size].
  virtual uint64_t findSymbol(DynamicLoader &DL, const std::string &Name) = 0;
  virtual bool hasUnderscorePrefix() = 0;
  void checkInRange(uint64_t Addr);
};

class LoadedDylib : public LoadedLibrary {
public:
  LIEF::MachO::Binary &Bin;

  LoadedDylib(std::unique_ptr<LIEF::MachO::FatBinary> &&Fat)
      : Fat(move(Fat)), Bin(Fat->at(0)) {}
  uint64_t findSymbol(DynamicLoader &DL, const std::string &Name) override;
  bool hasUnderscorePrefix() override { return true; }

private:
  std::unique_ptr<LIEF::MachO::FatBinary> Fat;
};

class LoadedDll : public LoadedLibrary {
public:
  HMODULE Ptr;

  uint64_t findSymbol(DynamicLoader &DL, const std::string &Name) override;
  bool hasUnderscorePrefix() override { return false; }
};

struct BinaryPath {
  std::string Path;
  bool Relative; // true iff `Path` is relative to install dir
};

class DynamicLoader {
public:
  DynamicLoader(uc_engine *UC) : UC(UC) {}
  LoadedLibrary *load(const std::string &Path);

private:
  void error(const std::string &Msg, bool AppendLastError = false);
  bool canSegmentsSlide(LIEF::MachO::Binary &Bin);
  void mapMemory(uint64_t Addr, uint64_t Size, uc_prot Perms, uint8_t *Mem);
  BinaryPath resolvePath(const std::string &Path);
  LoadedLibrary *loadMachO(const std::string &Path);
  LoadedLibrary *loadPE(const std::string &Path);

  static constexpr int PageSize = 4096;
  static constexpr int R_SCATTERED = 0x80000000; // From `<mach-o/reloc.h>`.
  uc_engine *const UC;
  std::map<std::string, std::unique_ptr<LoadedLibrary>> LIs;
};
