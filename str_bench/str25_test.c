#include "Python.h"

static struct PyModuleDef module = {
        PyModuleDef_HEAD_INIT,
        "str25_test",
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

typedef struct {
        PyObject_HEAD
        long    index;
        long    start;
        long    step;
        long    len;
} rangeiterobject;

static PyObject *
rangeiter_next(rangeiterobject *r)
{
    if (r->index < r->len)
        /* cast to unsigned to avoid possible signed overflow
           in intermediate calculations. */
        return PyLong_FromLong((long)(r->start +
                                      (unsigned long)(r->index++) * r->step));
    return NULL;
}

PyMODINIT_FUNC PyInit_str25_test(void) {
    for (int i = 0; i < 20; i++) {
        PyObject* a = PyTuple_Pack(1, PyLong_FromLong(1000000));
        PyObject* r = PyObject_Call((PyObject*)&PyRange_Type, a, NULL);
        Py_DECREF(a);

        a = PyTuple_Pack(2, (PyObject*)&PyUnicode_Type, r);
        Py_DECREF(r);
        PyObject* m = PyObject_Call((PyObject*)&PyMap_Type, a, NULL);
        Py_DECREF(a);


        mapobject* iter = (mapobject*)m;

        rangeiterobject *it = (rangeiterobject*)PyTuple_GET_ITEM(iter->iters, 0);
        PyObject* func = iter->func;
        PyObject *(*iternext)(PyObject *);
        PyObject *(*tp_str)(PyObject *);
        tp_str = PyLong_Type.tp_str;

        for (;;) {
            PyObject* item = NULL;

            PyObject *val = rangeiter_next(it);
            if (val == NULL) {
                break;
            }

            PyObject* v = val;

            item = (*tp_str)(v);
            if (PyUnicode_READY(item) < 0)
                item = NULL;
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
    }

    PyObject* m = PyModule_Create(&module);
    return m;
}
