#include "Python.h"

static struct PyModuleDef module = {
        PyModuleDef_HEAD_INIT,
        "str27",
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

#define SIGCHECK(PyTryBlock)                    \
    do {                                        \
        if (PyErr_CheckSignals()) PyTryBlock    \
    } while(0)

static int
long_to_decimal_string_internal(PyObject *aa,
                                PyObject **p_output,
                                _PyUnicodeWriter *writer,
                                _PyBytesWriter *bytes_writer,
                                char **bytes_str)
{
    PyLongObject *scratch, *a;
    PyObject *str = NULL;
    Py_ssize_t size, strlen, size_a, i, j;
    digit *pout, *pin, rem, tenpow;
    int negative;
    int d;
    enum PyUnicode_Kind kind;

    a = (PyLongObject *)aa;
    if (a == NULL || !PyLong_Check(a)) {
        PyErr_BadInternalCall();
        return -1;
    }
    size_a = Py_ABS(Py_SIZE(a));
    negative = Py_SIZE(a) < 0;

    /* quick and dirty upper bound for the number of digits
       required to express a in base _PyLong_DECIMAL_BASE:

         #digits = 1 + floor(log2(a) / log2(_PyLong_DECIMAL_BASE))

       But log2(a) < size_a * PyLong_SHIFT, and
       log2(_PyLong_DECIMAL_BASE) = log2(10) * _PyLong_DECIMAL_SHIFT
                                  > 3.3 * _PyLong_DECIMAL_SHIFT

         size_a * PyLong_SHIFT / (3.3 * _PyLong_DECIMAL_SHIFT) =
             size_a + size_a / d < size_a + size_a / floor(d),
       where d = (3.3 * _PyLong_DECIMAL_SHIFT) /
                 (PyLong_SHIFT - 3.3 * _PyLong_DECIMAL_SHIFT)
    */
    d = (33 * _PyLong_DECIMAL_SHIFT) /
        (10 * PyLong_SHIFT - 33 * _PyLong_DECIMAL_SHIFT);
    assert(size_a < PY_SSIZE_T_MAX/2);
    size = 1 + size_a + size_a / d;
    scratch = _PyLong_New(size);
    if (scratch == NULL)
        return -1;

    /* convert array of base _PyLong_BASE digits in pin to an array of
       base _PyLong_DECIMAL_BASE digits in pout, following Knuth (TAOCP,
       Volume 2 (3rd edn), section 4.4, Method 1b). */
    pin = a->ob_digit;
    pout = scratch->ob_digit;
    size = 0;
    for (i = size_a; --i >= 0; ) {
        digit hi = pin[i];
        for (j = 0; j < size; j++) {
            twodigits z = (twodigits)pout[j] << PyLong_SHIFT | hi;
            hi = (digit)(z / _PyLong_DECIMAL_BASE);
            pout[j] = (digit)(z - (twodigits)hi *
                              _PyLong_DECIMAL_BASE);
        }
        while (hi) {
            pout[size++] = hi % _PyLong_DECIMAL_BASE;
            hi /= _PyLong_DECIMAL_BASE;
        }
        /* check for keyboard interrupt */
        SIGCHECK({
                Py_DECREF(scratch);
                return -1;
            });
    }
    /* pout should have at least one digit, so that the case when a = 0
       works correctly */
    if (size == 0)
        pout[size++] = 0;

    /* calculate exact length of output string, and allocate */
    strlen = negative + 1 + (size - 1) * _PyLong_DECIMAL_SHIFT;
    tenpow = 10;
    rem = pout[size-1];
    while (rem >= tenpow) {
        tenpow *= 10;
        strlen++;
    }
    if (writer) {
        if (_PyUnicodeWriter_Prepare(writer, strlen, '9') == -1) {
            Py_DECREF(scratch);
            return -1;
        }
        kind = writer->kind;
    }
    else if (bytes_writer) {
        *bytes_str = _PyBytesWriter_Prepare(bytes_writer, *bytes_str, strlen);
        if (*bytes_str == NULL) {
            Py_DECREF(scratch);
            return -1;
        }
    }
    else {
        str = PyUnicode_New(strlen, '9');
        if (str == NULL) {
            Py_DECREF(scratch);
            return -1;
        }
        kind = PyUnicode_KIND(str);
    }

#define WRITE_DIGITS(p)                                               \
    do {                                                              \
        /* pout[0] through pout[size-2] contribute exactly            \
           _PyLong_DECIMAL_SHIFT digits each */                       \
        for (i=0; i < size - 1; i++) {                                \
            rem = pout[i];                                            \
            for (j = 0; j < _PyLong_DECIMAL_SHIFT; j++) {             \
                *--p = '0' + rem % 10;                                \
                rem /= 10;                                            \
            }                                                         \
        }                                                             \
        /* pout[size-1]: always produce at least one decimal digit */ \
        rem = pout[i];                                                \
        do {                                                          \
            *--p = '0' + rem % 10;                                    \
            rem /= 10;                                                \
        } while (rem != 0);                                           \
                                                                      \
        /* and sign */                                                \
        if (negative)                                                 \
            *--p = '-';                                               \
    } while (0)

#define WRITE_UNICODE_DIGITS(TYPE)                                    \
    do {                                                              \
        if (writer)                                                   \
            p = (TYPE*)PyUnicode_DATA(writer->buffer) + writer->pos + strlen; \
        else                                                          \
            p = (TYPE*)PyUnicode_DATA(str) + strlen;                  \
                                                                      \
        WRITE_DIGITS(p);                                              \
                                                                      \
        /* check we've counted correctly */                           \
        if (writer)                                                   \
            assert(p == ((TYPE*)PyUnicode_DATA(writer->buffer) + writer->pos)); \
        else                                                          \
            assert(p == (TYPE*)PyUnicode_DATA(str));                  \
    } while (0)

    /* fill the string right-to-left */
    if (bytes_writer) {
        char *p = *bytes_str + strlen;
        WRITE_DIGITS(p);
        assert(p == *bytes_str);
    }
    else if (kind == PyUnicode_1BYTE_KIND) {
        Py_UCS1 *p;
        WRITE_UNICODE_DIGITS(Py_UCS1);
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        Py_UCS2 *p;
        WRITE_UNICODE_DIGITS(Py_UCS2);
    }
    else {
        Py_UCS4 *p;
        assert (kind == PyUnicode_4BYTE_KIND);
        WRITE_UNICODE_DIGITS(Py_UCS4);
    }
#undef WRITE_DIGITS
#undef WRITE_UNICODE_DIGITS

    Py_DECREF(scratch);
    if (writer) {
        writer->pos += strlen;
    }
    else if (bytes_writer) {
        (*bytes_str) += strlen;
    }
    else {
        assert(_PyUnicode_CheckConsistency(str, 1));
        *p_output = (PyObject *)str;
    }
    return 0;
}

static PyObject *
long_to_decimal_string(PyObject *aa)
{
    PyObject *v;
    if (long_to_decimal_string_internal(aa, &v, NULL, NULL, NULL) == -1)
        return NULL;
    return v;
}

PyMODINIT_FUNC PyInit_str27(void) {
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

        if (Py_EnterRecursiveCall(" while calling a Python object")) {
            return NULL;
        }

        for (;;) {
            PyObject* item = NULL;

            int nargs = 0;
            PyObject *val = rangeiter_next(it);
            if (val == NULL) {
                goto exit;
            }
            nargs++;

            PyTypeObject* type = &PyUnicode_Type;
            {
                {
                    PyObject* v = val;

                    if (PyErr_CheckSignals()) {
                        item = NULL;
                        goto done3;
                    }
                    item = long_to_decimal_string(v);
                    if (PyUnicode_READY(item) < 0) {
                        item = NULL;
                        goto done3;
                    }
done3:
                    ;
                }

                item = _Py_CheckFunctionResult((PyObject*)type, item, NULL);
            }

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

        Py_LeaveRecursiveCall();
    }

    PyObject* m = PyModule_Create(&module);
    return m;
}
