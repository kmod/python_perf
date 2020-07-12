#include "Python.h"

static struct PyModuleDef module = {
        PyModuleDef_HEAD_INIT,
        "str12",
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

static PyObject *
__PyObject_FastCallDict(PyObject *callable, PyObject *const *args, Py_ssize_t nargs,
                       PyObject *kwargs)
{
    PyObject *argstuple, *result;
    ternaryfunc call;

    /* Slow-path: build a temporary tuple */
    call = callable->ob_type->tp_call;

    argstuple = PyTuple_Pack(1, args[0]);
    if (argstuple == NULL) {
        return NULL;
    }

    if (Py_EnterRecursiveCall(" while calling a Python object")) {
        Py_DECREF(argstuple);
        return NULL;
    }

    result = (*call)(callable, argstuple, kwargs);

    Py_LeaveRecursiveCall();
    Py_DECREF(argstuple);

    result = _Py_CheckFunctionResult(callable, result, NULL);
    return result;
}

PyMODINIT_FUNC PyInit_str12(void) {
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

        PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
        PyObject** stack = small_stack;
        for (;;) {
            PyObject* item = NULL;

            int nargs = 0;
            PyObject *val = rangeiter_next(it);
            if (val == NULL) {
                goto exit;
            }
            stack[0] = val;
            nargs++;

            item = __PyObject_FastCallDict(func, stack, 1, NULL);

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
