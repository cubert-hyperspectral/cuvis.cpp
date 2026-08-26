// Asks the installed cuvis library whether it provides the CUDA API, for
// find_package(CuvisCpp ... CUDA).
//
// It has to run rather than merely link: the CUDA entry points are looked up in the DLL
// at run time, and the shipped import library is known to lag the DLL, so a link probe
// would answer about the wrong file.
//
// Deliberately does not ask whether a backend is usable. That is a property of the
// machine's GPU and driver, and a build host without one still has an SDK that provides
// the API. cuvis_cuda_ipc_backend_available answers that question, at run time.

#include <cuvis.hpp>
#include <iostream>

int main()
{
  for (auto const& name : cuvis::cuda_missing_symbols())
    std::cout << name << "\n";
  return cuvis::cuda_supported() ? 0 : 1;
}
