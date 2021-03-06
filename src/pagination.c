#include <Python.h>
#include <stdio.h>
#include <math.h>

#ifndef PyVarObject_HEAD_INIT
# define PyVarObject_HEAD_INIT(type, size) PyObject_HEAD_INIT(type) size,
#endif

typedef struct {
	PyObject_HEAD
	int totalResult;
	char *prelink;
	int rowsPerPage;
	int pageLinks;
	int current;
	PyObject *translations;
} PaginationObject;

typedef struct {
	PyObject *prelink;
	int current;
	int previous;
	int next;
	int first;
	int last;
	int fromResult;
	int toResult;
	int startPage;
	int endPage;
	int totalResult;
	int pageCount;
} PaginationResult;

const char *LC_FIRST = "FIRST";
const char *LC_PREVIOUS = "PREVIOUS";
const char *LC_NEXT = "LC_NEXT";
const char *LC_LAST = "LAST";
const char *LC_CURRENT_PAGE_REPORT = "CURRENT_PAGE_REPORT";

static PyTypeObject Paginationtype;

static PaginationObject *
newPaginationObject(void) {
	//printf("Call newPaginationObject\n");
	return (PaginationObject *) PyObject_New(PaginationObject, &Paginationtype);
}

static void
Pagination_dealloc(PaginationObject *self) {
	//printf("Call Pagination_dealloc\n");
	PyMem_Free(self->prelink);
	Py_XDECREF(self->translations);
	PyObject_Del(self);
}

static PyObject *
preparePreLink(const char *prelink) {
	int len;
	len = strlen(prelink);
	if (strstr(prelink, "?") != NULL) {
		if (prelink[len-1] != '?' && prelink[len-1] != '&') {
			return PyString_FromFormat("%s&", prelink);
		} else {
			return PyString_FromString(prelink);
		}
	} else {
		return PyString_FromFormat("%s?", prelink);
	}
}

static PaginationResult *
Pagination_calc(PaginationObject *self) {
	//printf("%d %d %d %d %s", self->totalResult, self->pageLinks, self->rowsPerPage, self->current, self->prelink);
	PaginationResult *result;
	int startPage, endPage, half, oldPageLinks;

	oldPageLinks = (self->pageLinks % 2 == 0) ? 1 : 0;

	result = (PaginationResult *) PyMem_Malloc(sizeof(PaginationResult));
	if (result == NULL) {
		return NULL;
	}
	/* initialize values */
	result->prelink = preparePreLink(self->prelink);
	if (result->prelink == NULL) {
		PyMem_Free(result);
		return NULL;
	}
	result->first = 0;
	result->fromResult = 0;
	result->current = (self->current > 0)? self->current: 1;
	result->previous = 0;
	result->next = 0;
	result->first = 0;
	result->last = 0;
	result->fromResult = 0;
	result->toResult = 0;
	result->totalResult = self->totalResult;
	result->pageCount = 0;
	result->startPage = 1;
	result->endPage = 1;

	if (self->rowsPerPage <= 0) {
		return result;
	}
	/* force a floating point devision */
	result->pageCount = ceil((float) self->totalResult / self->rowsPerPage);

	if (result->pageCount < 2) {
		result->fromResult = 1;
		result->toResult = self->totalResult;
		return result;
	}

	/* adjust current in case it gets out of range for some reason */

	if (result->current > result->pageCount) {
		result->current = result->pageCount;
	}

	half = floor((float) self->pageLinks / 2);

	startPage = result->current - half;

	endPage = result->current + half - oldPageLinks;
	/* Adjust startPage and endPage */
	if (startPage < 1) {
		startPage = 1;
		endPage = startPage + self->pageLinks;
		if (endPage > result->pageCount) {
			endPage = result->pageCount;
		}
	}

	if (endPage > result->pageCount) {
		endPage = result->pageCount;
		startPage = endPage - self->pageLinks + 1;
		if (startPage < 1) {
			startPage = 1;
		}
	}

	result->startPage = startPage;
	result->endPage = endPage;

	if (result->current > 1) {
		result->first = 1;
		result->previous = result->current - 1;
	}

	if (result->current < result->pageCount) {
		result->last = result->pageCount;
		result->next = result->current + 1;
	}

	result->fromResult = (result->current - 1) * self->rowsPerPage + 1;

	if (result->current == result->pageCount) {
		result->toResult = self->totalResult;
	} else {
		result->toResult = result->fromResult + self->rowsPerPage - 1;
	}
	return result;
}
/**
 * Create a new Pagination object
 */
static PyObject *
Pagination_new(PyObject *self, PyObject *args, PyObject *kwdict) {
	PaginationObject *new;

	char *prelink = "";
	int totalResult = 0;
	int pageLinks = 5;
	int rowsPerPage = 10;
	int current = 1;
	PyObject *translations = NULL;

	static char *keywords[] = { "totalResult", "prelink", "pageLinks", "rowsPerPage", "current", "translations", NULL };
	//PyObject_Print(args, stdout, 0);
	//PyObject_Print(kwdict, stdout, 0);

	if (!PyArg_ParseTupleAndKeywords(args, kwdict, "i|siiiO", keywords, &totalResult, &prelink, &pageLinks, &rowsPerPage, &current, &translations)) {
		return NULL;
	}

	if ((new = newPaginationObject()) == NULL) {
		return NULL;
	}
	new->totalResult = totalResult;
	new->prelink = PyMem_Malloc(strlen(prelink));
	if (new->prelink == NULL) {
		PyObject_Del(new);
		return NULL;
	}
	strcpy(new->prelink, prelink);

	if ((new->translations = (PyObject *) PyDict_New()) == NULL) {
		PyMem_Free(new->prelink);
		PyObject_Del(new);
		return NULL;
	}
	/* Adding English translations */
	/* Is this check enough? */
	if ((PyDict_SetItemString(new->translations, LC_NEXT, PyString_FromString("Next")) == -1)
		|| (PyDict_SetItemString(new->translations, LC_PREVIOUS, PyString_FromString("Previous")) == -1)
		|| (PyDict_SetItemString(new->translations, LC_FIRST, PyString_FromString("First")) == -1)
		|| (PyDict_SetItemString(new->translations, LC_LAST, PyString_FromString("Last")) == -1)
		|| (PyDict_SetItemString(new->translations, LC_CURRENT_PAGE_REPORT, PyString_FromString("Results %d - %d of %d")) == -1)) {
			PyMem_Free(new->prelink);
			PyObject_Del(new);
			return NULL;
		}

	/* Override with specified translations; overhead and may be dangerous but :-D */
	if (translations && PyDict_Check(translations)) {
		PyDict_Merge(new->translations, translations, 1);
		//PyObject_Print(new->translations, stdout, 0);
	}

	Py_XDECREF(translations);


	new->pageLinks = pageLinks;
	new->rowsPerPage = rowsPerPage;
	new->current = current;

	return (PyObject *) new;
}

static PyObject *
Pagination_renderSearch(PaginationObject *self) {
	PaginationResult * result = NULL;
	const char *startHTML = "<div class=\"paginator\">";
	const char *endHTML = "</div>";
	PyObject *previousHTML = NULL;
	PyObject *nextHTML = NULL;
	char *rangeHTML = NULL;
	PyObject *res = NULL;
	int i;

	result = Pagination_calc(self);
	if (result == NULL) {
		goto on_mem_error;
	}

	if (result->pageCount < 2) {
		if ((res = PyString_FromFormat("%s%s",startHTML, endHTML)) == NULL) {
			goto on_mem_error;
		}
		Py_DECREF(result->prelink);
		PyMem_Free(result);
		return res;
	}

	if (result->previous > 0) {
		previousHTML = PyString_FromFormat("<a href=\"%spage=%d\" class=\"paginator-previous\">%s</a>",
											PyString_AsString(result->prelink),
											result->previous,
											PyString_AsString(PyDict_GetItemString(self->translations, LC_PREVIOUS)));
	} else {
		previousHTML = PyString_FromString("");
	}

	if (previousHTML == NULL) {
		goto on_mem_error;
	}

	if(result->endPage > result->startPage) {
		PyObject *className, *extraClassName, *pageLink;
		rangeHTML = PyMem_Malloc((snprintf(NULL, 0, "<a href=\"%spage=%d\" class=\"%s%s\">%d</a>",
									PyString_AsString(result->prelink),
									result->endPage,
									"paginator-current",
									" paginator-page-first",
									result->endPage) * (result->endPage - result->startPage)) + 1);
		if (rangeHTML == NULL) {
			goto on_mem_error;
		}

		strcpy(rangeHTML, "");

		for(i = result->startPage; i <= result->endPage; i++) {
			if(i == result->current) {
				className = PyString_FromString("paginator-current");
			} else {
				className = PyString_FromString("paginator-page");
			}
			if (className == NULL) {
				goto on_mem_error;
			}
			if (i == result->startPage) {
				extraClassName = PyString_FromString(" paginator-page-first");
			} else if (i == result->endPage) {
				extraClassName = PyString_FromString(" paginator-page-last");
			} else {
				extraClassName = PyString_FromString("");
			}
			if (extraClassName == NULL) {
				Py_DECREF(className);
				goto on_mem_error;
			}
			pageLink = PyString_FromFormat("<a href=\"%spage=%d\" class=\"%s%s\">%d</a>",
									PyString_AsString(result->prelink),
									i,
									PyString_AsString(className),
									PyString_AsString(extraClassName),
									i);
			if (pageLink == NULL) {
				Py_DECREF(className);
				Py_DECREF(extraClassName);
				goto on_mem_error;
			}
			strcat(rangeHTML, PyString_AsString(pageLink));
			Py_DECREF(className);
			Py_DECREF(extraClassName);
			Py_DECREF(pageLink);
		}
	} else {
		if ((rangeHTML = PyMem_Malloc(2)) == NULL) {
			goto on_mem_error;
		}
		strcpy(rangeHTML, "");
	}

	if (result->next > 0) {
		nextHTML = PyString_FromFormat("<a href=\"%spage=%d\" class=\"paginator-next\">%s</a>",
				PyString_AsString(result->prelink),
				result->next,
				PyString_AsString(PyDict_GetItemString(self->translations, LC_NEXT)));
	} else {
		nextHTML = PyString_FromString("");
	}

	if (nextHTML == NULL) {
		goto on_mem_error;
	}

	res = PyString_FromFormat("%s%s%s%s%s", startHTML, PyString_AsString(previousHTML), rangeHTML, PyString_AsString(nextHTML), endHTML);

	Py_DECREF(result->prelink);
	PyMem_Free(result);
	Py_DECREF(previousHTML);
	Py_DECREF(nextHTML);
	PyMem_Free(rangeHTML);

	return res;

on_mem_error:
	/* clean up */
	if (result) {
		Py_DECREF(result->prelink);
		PyMem_Free(result);
	}
	Py_XDECREF(previousHTML);
	if (rangeHTML) {
		PyMem_Free(rangeHTML);
	}
	Py_XDECREF(nextHTML);
	PyErr_SetString(PyExc_MemoryError, "pagination.Pagination_renderSearch");
	return NULL;
}


static PyObject *
Pagination_renderItem(PaginationObject *self) {
	PaginationResult * result = NULL;
	const char *startHTML = "<div class=\"paginator\">";
	const char *endHTML = "</div>";
	PyObject *previousHTML = NULL;
	PyObject *reportHTML = NULL;
	PyObject *nextHTML = NULL;
	PyObject *firstHTML = NULL;
	PyObject *lastHTML = NULL;
	PyObject *res = NULL;

	result = Pagination_calc(self);

	if (result == NULL) {
		goto on_mem_error;
	}

	PyObject *report_tmp = PyString_FromFormat("<span class=\"paginator-current-report\">%s</span>",
			PyString_AsString(PyDict_GetItemString(self->translations, LC_CURRENT_PAGE_REPORT)));

	if (report_tmp == NULL) {
		goto on_mem_error;
	}

	if ((reportHTML = PyString_FromFormat(PyString_AsString(report_tmp),
				result->fromResult, result->toResult, result->totalResult)) == NULL) {
		goto on_mem_error;
	}
	Py_DECREF(report_tmp);

	if (result->first > 0) {
		firstHTML = PyString_FromFormat("<a href=\"%spage=%d\" class=\"paginator-first\">%s</a>",
				PyString_AsString(result->prelink),
				result->first,
				PyString_AsString(PyDict_GetItemString(self->translations, LC_FIRST)));
	} else {
		firstHTML = PyString_FromFormat("<span class=\"paginator-first\">%s</span>",
				PyString_AsString(PyDict_GetItemString(self->translations, LC_FIRST)));
	}
	if (firstHTML == NULL) {
		goto on_mem_error;
	}

	if (result->previous > 0) {
		previousHTML = PyString_FromFormat("<a href=\"%spage=%d\" class=\"paginator-previous\">%s</a>",
				PyString_AsString(result->prelink),
				result->previous,
				PyString_AsString(PyDict_GetItemString(self->translations, LC_PREVIOUS)));
	} else {
		previousHTML = PyString_FromFormat("<span class=\"paginator-previous\">%s</span>",
				PyString_AsString(PyDict_GetItemString(self->translations, LC_PREVIOUS)));
	}
	if (previousHTML == NULL) {
		goto on_mem_error;
	}

	if (result->next > 0) {
		nextHTML = PyString_FromFormat("<a href=\"%spage=%d\" class=\"paginator-next\">%s</a>",
				PyString_AsString(result->prelink),
				result->next,
				PyString_AsString(PyDict_GetItemString(self->translations, LC_NEXT)));
	} else {
		nextHTML = PyString_FromFormat("<span class=\"paginator-next\">%s</span>",
				PyString_AsString(PyDict_GetItemString(self->translations, LC_NEXT)));
	}
	if (nextHTML == NULL) {
		goto on_mem_error;
	}

	if (result->last > 0) {
		lastHTML = PyString_FromFormat("<a href=\"%spage=%d\" class=\"paginator-last\">%s</a>",
				PyString_AsString(result->prelink),
				result->last,
				PyString_AsString(PyDict_GetItemString(self->translations, LC_LAST)));
	} else {
		lastHTML = PyString_FromFormat("<span class=\"paginator-last\">%s</span>",
				PyString_AsString(PyDict_GetItemString(self->translations, LC_LAST)));
	}
	if (lastHTML == NULL) {
		goto on_mem_error;
	}

	res = PyString_FromFormat("%s%s%s%s%s%s%s",
			startHTML,
			PyString_AsString(reportHTML),
			PyString_AsString(firstHTML),
			PyString_AsString(previousHTML),
			PyString_AsString(nextHTML),
			PyString_AsString(lastHTML),
			endHTML);

	Py_DECREF(result->prelink);
	PyMem_Free(result);
	Py_DECREF(firstHTML);
	Py_DECREF(previousHTML);
	Py_DECREF(nextHTML);
	Py_DECREF(lastHTML);
	Py_DECREF(reportHTML);

	return res;

on_mem_error:
	/* clean up */
	if (result) {
		Py_DECREF(result->prelink);
		PyMem_Free(result);
	}
	Py_XDECREF(firstHTML);
	Py_XDECREF(previousHTML);
	Py_XDECREF(nextHTML);
	Py_XDECREF(lastHTML);
	Py_XDECREF(reportHTML);
	PyErr_SetString(PyExc_MemoryError, "pagination.Pagination_renderItem");
	return NULL;
}

static PyObject *
Pagination_render(PaginationObject *self, PyObject *args, PyObject *kwdict) {
	char *pattern = "search";
	static char *keywords[] = {"pattern", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwdict, "|s", keywords, &pattern)) {
		return NULL;
	}

	if (strcmp(pattern, "item") == 0) {
		return Pagination_renderItem(self);
	} else {
		return Pagination_renderSearch(self);
	}
}

static PyObject *
Pagination_getPaginationData(PaginationObject *self) {
	PaginationResult *result;
	PyObject *res;

	result = Pagination_calc(self);
	if (result == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	res = Py_BuildValue("{s:s,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
			"prelink", PyString_AsString(result->prelink),
			"totalResult", result->totalResult,
			"fromResult", result->fromResult,
			"toResult", result->toResult,
			"current", result->current,
			"endPage", result->endPage,
			"first", result->first,
			"last", result->last,
			"next", result->next,
			"pageCount", result->pageCount,
			"previous", result->previous,
			"startPage", result->startPage);

	Py_DECREF(result->prelink);
	PyMem_Free(result);
	return res;
}


/**
 * Method to bind to class
 */
static PyMethodDef Pagination_methods[] = {
	{ "render", (PyCFunction) Pagination_render, METH_KEYWORDS, PyDoc_STR("Render pagination") },
	{ "getPaginationData", (PyCFunction) Pagination_getPaginationData, METH_NOARGS, PyDoc_STR("Get pagination data as a dictionary") },
	{ NULL, NULL } /* sentinel */
};

static PyTypeObject Paginationtype = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_pagination.pagination", /*tp_name*/
	sizeof(PaginationObject), /*tp_size*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) Pagination_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	0, /*tp_getattr*/
	0, /*tp_setattr*/
	0, /*tp_reserved*/
	0, /*tp_repr*/
	0, /*tp_as_number*/
	0, /*tp_as_sequence*/
	0, /*tp_as_mapping*/
	0, /*tp_hash*/
	0, /*tp_call*/
	0, /*tp_str*/
	0, /*tp_getattro*/
	0, /*tp_setattro*/
	0, /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT, /*tp_flags*/
	0, /*tp_doc*/
	0, /*tp_traverse*/
	0, /*tp_clear*/
	0, /*tp_richcompare*/
	0, /*tp_weaklistoffset*/
	0, /*tp_iter*/
	0, /*tp_iternext*/
	Pagination_methods, /* tp_methods */
	NULL, /* tp_members */
	NULL, /* tp_getset */
};

static struct PyMethodDef Pagination_functions[] = {
	{ "Paginator", (PyCFunction) Pagination_new, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("New Pagination") },
	{ NULL, NULL } /* Sentinel */
};


PyMODINIT_FUNC initpagination(void) {
	PyObject *m;

	if (PyType_Ready(&Paginationtype) < 0) {
		return;
	}
	/* class and functions to modules */
	m = Py_InitModule("pagination", Pagination_functions);

	if (m == NULL) {
		return;
	}
	if ((PyModule_AddStringConstant(m, "LC_NEXT", LC_NEXT)) == -1
		|| (PyModule_AddStringConstant(m, "LC_PREVIOUS", LC_PREVIOUS)) == -1
		|| (PyModule_AddStringConstant(m, "LC_FIRST", LC_FIRST)) == -1
		|| (PyModule_AddStringConstant(m, "LC_LAST", LC_LAST)) == -1
		|| (PyModule_AddStringConstant(m, "LC_CURRENT_PAGE_REPORT", LC_CURRENT_PAGE_REPORT)) == -1) {
		return;
	}

	Py_INCREF(&Paginationtype);
}
