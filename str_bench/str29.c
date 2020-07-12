#include "Python.h"

static struct PyModuleDef module = {
        PyModuleDef_HEAD_INIT,
        "str29",
        NULL,
        -1,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC PyInit_str29(void) {
    for (int i = 0; i < 20; i++) {
        if (Py_EnterRecursiveCall(" while calling a Python object")) {
            return NULL;
        }

        for (int j = 0; j < 1000000; j++) {
            PyObject* item = NULL;
            PyObject *val = PyLong_FromLong(j);

            PyTypeObject* type = &PyUnicode_Type;
            {
                {
                    PyObject* v = val;

                    if (PyErr_CheckSignals()) {
                        item = NULL;
                        goto done3;
                    }
                    item = v;
                    Py_INCREF(item);
done3:
                    ;
                }

                item = _Py_CheckFunctionResult((PyObject*)type, item, NULL);
            }

exit:
            Py_DECREF(val);

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

        Py_LeaveRecursiveCall();
    }

    PyObject* m = PyModule_Create(&module);
    return m;
}
