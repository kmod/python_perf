#include "Python.h"

static struct PyModuleDef module = {
        PyModuleDef_HEAD_INIT,
        "str09",
        NULL,
        -1,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
};

typedef struct {
    PyObject_HEAD
    PyObject *iters;
    PyObject *func;
} mapobject;

PyMODINIT_FUNC PyInit_str09(void) {
    for (int i = 0; i < 20; i++) {
        PyObject* a = PyTuple_Pack(1, PyLong_FromLong(1000000));
        PyObject* r = PyObject_Call((PyObject*)&PyRange_Type, a, NULL);
        Py_DECREF(a);

        a = PyTuple_Pack(2, (PyObject*)&PyUnicode_Type, r);
        Py_DECREF(r);
        PyObject* m = PyObject_Call((PyObject*)&PyMap_Type, a, NULL);
        Py_DECREF(a);


        mapobject* iter = (mapobject*)m;

        for (;;) {
            PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
            PyObject** stack = small_stack;

            PyObject* item = NULL;

            int nargs = 0;
            PyObject *it = PyTuple_GET_ITEM(iter->iters, 0);
            PyObject *val = Py_TYPE(it)->tp_iternext(it);
            if (val == NULL) {
                goto exit;
            }
            stack[0] = val;
            nargs++;

            item = _PyObject_FastCall(iter->func, stack, 1);

exit:
            for (int i=0; i < nargs; i++) {
                Py_DECREF(stack[i]);
            }

            if (item == NULL) {
                if (PyErr_Occurred()) {
                    if (PyErr_ExceptionMatches(PyExc_StopIteration))
                        PyErr_Clear();
                    else
                        abort();
                }
                break;
            }
            Py_DECREF(item);
        }
    }

    PyObject* m = PyModule_Create(&module);
    return m;
}
