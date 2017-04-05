#include <pybind11/embedded.h>
#include <catch.hpp>

namespace py = pybind11;
using namespace py::literals;

class Widget {
public:
    Widget(std::string message) : message(message) { }
    virtual ~Widget() = default;

    std::string the_message() const { return message; }
    virtual int the_answer() const = 0;

private:
    std::string message;
};

class PyWidget final : public Widget {
    using Widget::Widget;

    int the_answer() const override { PYBIND11_OVERLOAD_PURE(int, Widget, the_answer); }
};

PYBIND11_ADD_EMBEDDED_MODULE(widget_module)(py::module &m) {
    py::class_<Widget, PyWidget>(m, "Widget")
        .def(py::init<std::string>())
        .def_property_readonly("the_message", &Widget::the_message);
}

TEST_CASE("Pass classes and data between modules defined in C++ and Python") {
    auto module = py::module::import("test_interpreter");
    REQUIRE(py::hasattr(module, "DerivedWidget"));

    auto locals = py::dict("hello"_a="Hello, World!", "x"_a=5, **module.attr("__dict__"));
    py::eval<py::eval_statements>(R"(
        widget = DerivedWidget("{} - {}".format(hello, x))
        message = widget.the_message
        )", py::globals(), locals
    );
    REQUIRE(locals["message"].cast<std::string>() == "Hello, World! - 5");

    auto py_widget = module.attr("DerivedWidget")("The question");
    auto message = py_widget.attr("the_message");
    REQUIRE(message.cast<std::string>() == "The question");

    const auto &cpp_widget = py_widget.cast<const Widget &>();
    REQUIRE(cpp_widget.the_answer() == 42);
}
