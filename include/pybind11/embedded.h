/*
    pybind11/embedded.h: Support for embedding the interpreter

    Copyright (c) 2017 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include "pybind11.h"
#include "eval.h"

#if defined(PYPY_VERSION)
#  error Embedding the interpreter is not supported on PyPy
#endif

#if PY_MAJOR_VERSION >= 3
#define PYBIND11_ADD_EMBEDDED_MODULE_IMPL(name)        \
    extern "C" PyObject *pybind11_init_impl_##name() { \
        auto m = pybind11::module(#name);              \
        pybind11_init_##name(m);                       \
        return m.ptr();                                \
    }
#else
#define PYBIND11_ADD_EMBEDDED_MODULE_IMPL(name)   \
    extern "C" void pybind11_init_impl_##name() { \
        auto m = pybind11::module(#name);         \
        pybind11_init_##name(m);                  \
    }
#endif

/** \rst
    Add a new module to the table of builtins for the interpreter. Must be
    defined in global scope.

    .. code-block:: cpp

        PYBIND11_ADD_EMBEDDED_MODULE(example)(py::module &m) {
            // ... initialize functions and classes here
            m.def("foo", []() {
                return "Hello, World!";
            });
        }
 \endrst */
#define PYBIND11_ADD_EMBEDDED_MODULE(name)                                        \
    static void pybind11_init_##name(pybind11::module &);                         \
    PYBIND11_ADD_EMBEDDED_MODULE_IMPL(name)                                       \
    pybind11::detail::add_embedded_module name(#name, pybind11_init_impl_##name); \
    void pybind11_init_##name


NAMESPACE_BEGIN(pybind11)
NAMESPACE_BEGIN(detail)

/// Python 2.7/3.x compatible version of `PyImport_AppendInittab` and error checks.
struct add_embedded_module {
#if PY_MAJOR_VERSION >= 3
    using init_t = PyObject *(*)();
#else
    using init_t = void (*)();
#endif
    add_embedded_module(const char *name, init_t init) {
        if (Py_IsInitialized())
            pybind11_fail("Can't add new modules after the interpreter has been initialized");

        auto result = PyImport_AppendInittab(name, init);
        if (result == -1)
            pybind11_fail("Insufficient memory to add a new module");
    }
};

NAMESPACE_END(detail)

/** \rst
    Initialize the Python interpreter. No other pybind11 or CPython API functions can be
    called before this is done; with the exception of `add_embedded_module`. The optional
    parameter can be used to skip the registration of signal handlers (see the Python
    documentation for details).
 \endrst */
inline void initialize_interpreter(bool init_signal_handlers = true) {
    if (Py_IsInitialized())
        return;

    Py_InitializeEx(init_signal_handlers ? 1 : 0);

    // Make .py files in the working directory available by default
    auto sys_path = reinterpret_borrow<list>(module::import("sys").attr("path"));
    sys_path.append(".");
}

/** \rst
    Shut down the Python interpreter. No pybind11 or CPython API functions can be called
    after this. In addition, pybind11 objects must not outlive the interpreter:

    .. code-block:: cpp

        { // BAD
            py::initialize_interpreter();
            auto hello = py::str("Hello, World!");
            py::finalize_interpreter();
        } // <-- BOOM, hello's destructor is called after interpreter shutdown

        { // GOOD
            py::initialize_interpreter();
            { // scoped
                auto hello = py::str("Hello, World!");
            } // <-- OK, hello is cleaned up properly
            py::finalize_interpreter();
        }

        { // BETTER
            py::scoped_interpreter guard{};
            auto hello = py::str("Hello, World!");
        }

    .. warning::

        Python cannot unload binary extension modules. If `initialize_interpreter` is
        called again to restart the interpreter, the initializers of those modules will
        be executed for a second time and they will fail. This is a known CPython issue.
        See the Python documentation for details.

 \endrst */
inline void finalize_interpreter() { Py_Finalize(); }

/** \rst
    Scope guard version of `initialize_interpreter` and `finalize_interpreter`.

    .. code-block:: cpp

        #include <pybind11/embedded.h>

        int main() {
            py::scoped_interpreter guard{};
            py::print(Hello, World!);
        }
 \endrst */
struct scoped_interpreter {
    scoped_interpreter() { initialize_interpreter(); }
    ~scoped_interpreter() { finalize_interpreter(); }
};

/// Return the ``__main__`` module.
inline module main() { return module::import("__main__"); }

/// Return a dictionary representing the global symbol table, i.e. ``__main__.__dict__``.
inline dict globals() { return main().attr("__dict__").cast<dict>(); }

NAMESPACE_END(pybind11)
