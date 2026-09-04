// Looks up the CUDA entry points in the loaded cuvis library.
//
// They are resolved here instead of being imported at link time so that the wrapper
// builds and runs against an SDK that predates them: an older library simply reports
// them as missing rather than refusing to load the process.
//
// This is the only file in the wrapper that includes an OS header, which is why it is a
// translation unit rather than part of the header-only interface.

#include "cuvis.hpp"

#ifdef _WIN32
  // Guarded because this file is compiled into consumers that already define them.
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

namespace cuvis::detail
{
  namespace
  {
#ifdef _WIN32
    HMODULE cuvis_module()
    {
      // Loading it explicitly, not just looking it up: a program that links the wrapper
      // but never calls a cuvis function has no cuvis.dll import at all, so the module
      // would not be resident and every symbol would look missing. LoadLibraryW uses the
      // same search order the import would have used, so it resolves the same file, and
      // GetModuleHandleW is tried first so an already-loaded module always wins.
      static HMODULE const module = [] {
        HMODULE loaded = ::GetModuleHandleW(L"cuvis.dll");
        return loaded ? loaded : ::LoadLibraryW(L"cuvis.dll");
      }();
      return module;
    }

    void* find_symbol(char const* name)
    {
      HMODULE const module = cuvis_module();
      return module ? reinterpret_cast<void*>(::GetProcAddress(module, name)) : nullptr;
    }
#else
    void* find_symbol(char const* name)
    {
      if (void* symbol = ::dlsym(RTLD_DEFAULT, name))
        return symbol;

      // Same reason as the Windows branch: nothing may have pulled libcuvis in yet.
      static void* const fallback = ::dlopen("libcuvis.so", RTLD_LAZY | RTLD_GLOBAL);
      return fallback ? ::dlsym(fallback, name) : nullptr;
    }
#endif

    template <typename Fn> void bind(cuda_symbols& syms, Fn& slot, char const* name)
    {
      slot = reinterpret_cast<Fn>(find_symbol(name));
      if (!slot)
        syms.missing.emplace_back(name);
    }

    cuda_symbols resolve()
    {
      cuda_symbols syms;
      bind(syms, syms.measurement_get_data_image_cuda, "cuvis_measurement_get_data_image_cuda");
      bind(syms, syms.mem_get_view, "cuvis_cuda_mem_get_view");
      bind(syms, syms.mem_copy_handle, "cuvis_cuda_mem_copy_handle");
      bind(syms, syms.mem_free, "cuvis_cuda_mem_free");
      bind(syms, syms.ipc_handle_create, "cuvis_cuda_ipc_handle_create");
      bind(syms, syms.ipc_get_descriptor, "cuvis_cuda_ipc_get_descriptor");
      bind(syms, syms.ipc_handle_free, "cuvis_cuda_ipc_handle_free");
      bind(syms, syms.ipc_backend_available, "cuvis_cuda_ipc_backend_available");
      return syms;
    }
  } // namespace

  cuda_symbols const& cuda_syms()
  {
    static cuda_symbols const syms = resolve();
    return syms;
  }
} // namespace cuvis::detail
