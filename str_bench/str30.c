#include "Python.h"

static struct PyModuleDef module = {
        PyModuleDef_HEAD_INIT,
        "str30",
        NULL,
        -1,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC PyInit_str30(void) {
    for (int i = 0; i < 20; i++) {
        if (Py_EnterRecursiveCall(" while calling a Python object")) {
            return NULL;
        }

        for (int j = 0; j < 1000000; j++) {
        }

        Py_LeaveRecursiveCall();
    }

    PyObject* m = PyModule_Create(&module);
    return m;
}
