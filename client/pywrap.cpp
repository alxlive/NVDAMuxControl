#include <pybind11/pybind11.h>
#include "kernel_socket.hpp"

namespace py = pybind11;

PYBIND11_MODULE(KernelSocketLib, m) {
    py::class_<KernelSocket>(m, "KernelSocket")
    .def(py::init<>())  
    .def("connect", &KernelSocket::connect)
    .def("setBrightness", &KernelSocket::setBrightness)
    .def("increaseBrightness", &KernelSocket::increaseBrightness)
    .def("decreaseBrightness", &KernelSocket::decreaseBrightness)
    .def("close", &KernelSocket::close)
    ;
}
