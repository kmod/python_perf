#include "Python.h"

static struct PyModuleDef module = {
        PyModuleDef_HEAD_INIT,
        "str16",
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
unicode_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *x = NULL;
    static char *kwlist[] = {"object", "encoding", "errors", 0};
    char *encoding = NULL;
    char *errors = NULL;

    if (type != &PyUnicode_Type)
        abort();
        //return unicode_subtype_new(type, args, kwds);
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oss:str",
                                     kwlist, &x, &encoding, &errors))
        return NULL;
    if (x == NULL)
        abort();
        //_Py_RETURN_UNICODE_EMPTY();
    if (encoding == NULL && errors == NULL)
        return PyObject_Str(x);
    else
        return PyUnicode_FromEncodedObject(x, encoding, errors);
}

PyMODINIT_FUNC PyInit_str16(void) {
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

            PyTypeObject* type = (PyTypeObject*)func;
            {
                if (type->tp_new == NULL) {
                    PyErr_Format(PyExc_TypeError,
                                 "cannot create '%.100s' instances",
                                 type->tp_name);
                    return NULL;
                }

                item = unicode_new(type, argstuple, NULL);
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
