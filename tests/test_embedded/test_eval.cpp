#include <pybind11/embedded.h>
#include <catch.hpp>

namespace py = pybind11;
using namespace py::literals;

TEST_CASE("eval_statements") {
    auto locals = py::dict("call_test"_a=py::cpp_function([]() { return 42; }));

    // Regular string literal
    py::eval<py::eval_statements>(
        "message = 'Hello World!'\n"
        "x = call_test()",
        py::globals(), locals
    );
    REQUIRE(locals["x"].cast<int>() == 42);

    // Multi-line raw string literal
    auto result = py::eval<py::eval_statements>(R"(
        if x == 42:
            x = 43
        else:
            raise RuntimeError
        )", py::globals(), locals
    );

    REQUIRE(locals["x"].cast<int>() == 43);
    REQUIRE(result.is_none());
}

TEST_CASE("eval") {
    auto locals = py::dict("x"_a=42);
    auto x = py::eval("x+1", py::globals(), locals);

    REQUIRE(x.cast<int>() == 43);
    REQUIRE_THROWS_WITH(py::eval("nonsense code ..."), Catch::Contains("invalid syntax"));
}

TEST_CASE("eval_single_statement") {
    auto locals = py::dict("call_test"_a=py::cpp_function([]() { return 42; }));
    auto result = py::eval<py::eval_single_statement>("x = call_test()", py::dict(), locals);

    REQUIRE(result.is_none());
    REQUIRE(locals["x"].cast<int>() == 42);
}

TEST_CASE("eval_file") {
    int val_out;
    auto locals = py::dict(
        "y"_a=43,
        "call_test2"_a=py::cpp_function([&](int value) { val_out = value; })
    );
    auto result = py::eval_file("test_eval_call.py", py::globals(), locals);

    REQUIRE(result.is_none());
    REQUIRE(val_out == 43);
    REQUIRE_THROWS_WITH(py::eval_file("non-existing file"),
                        Catch::Contains("could not be opened!"));
}
