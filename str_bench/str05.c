#include "Python.h"

static struct PyModuleDef module = {
        PyModuleDef_HEAD_INIT,
        "str05",
        NULL,
        -1,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC PyInit_str05(void) {
    for (int i = 0; i < 20; i++) {
        PyObject* a = PyTuple_Pack(1, PyLong_FromLong(1000000));
        PyObject* r = PyObject_Call((PyObject*)&PyRange_Type, a, NULL);
        Py_DECREF(a);

        a = PyTuple_Pack(2, (PyObject*)&PyUnicode_Type, r);
        Py_DECREF(r);
        PyObject* m = PyObject_Call((PyObject*)&PyMap_Type, a, NULL);
        Py_DECREF(a);

        a = PyTuple_Pack(1, m);
        Py_DECREF(m);
        PyObject* l = PyObject_Call((PyObject*)&PyList_Type, a, NULL);
        Py_DECREF(a);

        Py_DECREF(l);
    }

    PyObject* m = PyModule_Create(&module);
    return m;
}
