#include "Python.h"

static struct PyModuleDef module = {
        PyModuleDef_HEAD_INIT,
        "str20",
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

PyObject *
_PyObject_Str(PyObject *v)
{
    PyObject *res;
    if (PyErr_CheckSignals())
        return NULL;
#ifdef USE_STACKCHECK
    if (PyOS_CheckStack()) {
        PyErr_SetString(PyExc_MemoryError, "stack overflow");
        return NULL;
    }
#endif
    if (v == NULL)
        return PyUnicode_FromString("<NULL>");
    if (PyUnicode_CheckExact(v)) {
#ifndef Py_DEBUG
        if (PyUnicode_READY(v) < 0)
            return NULL;
#endif
        Py_INCREF(v);
        return v;
    }
    if (Py_TYPE(v)->tp_str == NULL)
        return PyObject_Repr(v);

#ifdef Py_DEBUG
    /* PyObject_Str() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!PyErr_Occurred());
#endif

    /* It is possible for a type to have a tp_str representation that loops
       infinitely. */
    if (Py_EnterRecursiveCall(" while getting the str of an object"))
        return NULL;
    res = (*Py_TYPE(v)->tp_str)(v);
    Py_LeaveRecursiveCall();
    if (res == NULL)
        return NULL;
    if (!PyUnicode_Check(res)) {
        PyErr_Format(PyExc_TypeError,
                     "__str__ returned non-string (type %.200s)",
                     Py_TYPE(res)->tp_name);
        Py_DECREF(res);
        return NULL;
    }
#ifndef Py_DEBUG
    if (PyUnicode_READY(res) < 0)
        return NULL;
#endif
    assert(_PyUnicode_CheckConsistency(res, 1));
    return res;
}

PyMODINIT_FUNC PyInit_str20(void) {
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

        for (;;) {
            PyObject* item = NULL;

            int nargs = 0;
            PyObject *val = rangeiter_next(it);
            if (val == NULL) {
                goto exit;
            }
            nargs++;

            PyObject *argstuple;

            argstuple = PyTuple_Pack(1, val);
            if (argstuple == NULL) {
                return NULL;
            }

            if (Py_EnterRecursiveCall(" while calling a Python object")) {
                Py_DECREF(argstuple);
                return NULL;
            }

            PyTypeObject* type = &PyUnicode_Type;
            {
                item = _PyObject_Str(PyTuple_GET_ITEM(argstuple, 0));

                item = _Py_CheckFunctionResult((PyObject*)type, item, NULL);
                if (item == NULL)
                    goto done;

                /* If the returned object is not an instance of type,
                   it won't be initialized. */
                if (!PyType_IsSubtype(Py_TYPE(item), type))
                    goto done;

                type = Py_TYPE(item);
                if (type->tp_init != NULL) {
                    int res = type->tp_init(item, argstuple, NULL);
                    if (res < 0) {
                        assert(PyErr_Occurred());
                        Py_DECREF(item);
                        item = NULL;
                    }
                    else {
                        assert(!PyErr_Occurred());
                    }
                }
done:
                ;
            }

            Py_LeaveRecursiveCall();
            Py_DECREF(argstuple);

            item = _Py_CheckFunctionResult(func, item, NULL);

exit:
            if (nargs)
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
