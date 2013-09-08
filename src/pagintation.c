#include <Python.h>
#include <stdio.h>
#include <math.h>

#ifndef PyVarObject_HEAD_INIT
# define PyVarObject_HEAD_INIT(type, size) PyObject_HEAD_INIT(type) size,
#endif

typedef struct {
	PyObject_HEAD
	int totalResult;
	const char *prelink;
	int rowsPerPage;
	int pageLinks;
	int current;
} PaginationObject;

typedef struct {
	char *prelink;
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


static PyTypeObject SearchPaginationtype;

static PaginationObject *
newSearchPaginationObject(void) {
	//printf("Call newSearchPaginationObject\n");
	return (PaginationObject *) PyObject_New(PaginationObject, &SearchPaginationtype);
}

static void
Pagination_dealloc(PyObject *ptr) {
	//printf("Call Pagination_dealloc\n");
	PyObject_Del(ptr);
}

static PaginationResult *
Pagination_calc(PaginationObject *self) {
	//printf("%d %d %d %d %s", self->totalResult, self->pageLinks, self->rowsPerPage, self->current, self->prelink);
	PaginationResult *result;
	int startPage, endPage, pageCount, half, oldPageLinks;

	oldPageLinks = (self->pageLinks % 2 == 0) ? 1 : 0;

	result = (PaginationResult *) PyMem_Malloc(sizeof(PaginationResult));
	if (result == NULL) {
		return NULL;
	}
	/* initialize values */
	if ((asprintf(&result->prelink, "%s", self->prelink)) == -1) {
		free(result);
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

	pageCount = ceil(self->totalResult / self->rowsPerPage);

	result->pageCount = pageCount;

	if (pageCount < 2) {
		result->fromResult = 1;
		result->toResult = self->totalResult;
		return result;
	}

	/* adjust current in case it gets out of range for some reason */

	if (result->current > pageCount) {
		result->current = pageCount;
	}

	half = floor(self->pageLinks / 2);

	startPage = result->current - half;

	endPage = result->current + half - oldPageLinks;
	/* Adjust startPage and endPage */
	if (startPage < 1) {
		startPage = 1;
		endPage = startPage + self->pageLinks;
		if (endPage > pageCount) {
			endPage = pageCount;
		}
	}

	if (endPage > pageCount) {
		endPage = pageCount;
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

	if (result->current < pageCount) {
		result->last = pageCount;
		result->next = result->current + 1;
	}

	result->fromResult = (result->current - 1) * self->rowsPerPage + 1;

	if (result->current == pageCount) {
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
SearchPagination_new(PyObject *self, PyObject *args, PyObject *kwdict) {
	PaginationObject *new;

	char *prelink = "?";
	int totalResult = 1000;
	int pageLinks = 14;
	int rowsPerPage = 10;
	int current = 1;
	static char *keywords[] = { "totalResult", "prelink", "pageLinks", "rowsPerPage", "current",  NULL };

	//PyObject_Print(args, stdout, 0);
	//PyObject_Print(kwdict, stdout, 0);

	if (!PyArg_ParseTupleAndKeywords(args, kwdict, "i|siii", keywords, &totalResult, &prelink, &pageLinks, &rowsPerPage, &current)) {
		return NULL;
	}

	if ((new = newSearchPaginationObject()) == NULL) {
		return NULL;
	}
	new->totalResult = totalResult;
	new->prelink = prelink;
	new->pageLinks = pageLinks;
	new->rowsPerPage = rowsPerPage;
	new->current = current;
	// TODO: check and raise if data not expected

	return (PyObject *) new;
}

char *
preparePreLink(char *prelink) {
	char *prlink = NULL;
	int len, asprintf_size;
	len = strlen(prelink);
	if (strstr(prelink, "?") != NULL) {
		if (prelink[len-1] != '?' && prelink[len-1] != '&') {
			asprintf_size = asprintf(&prlink, "%s&", prelink);
		} else {
			asprintf_size = asprintf(&prlink, "%s", prelink);
		}
	} else {
		asprintf_size = asprintf(&prlink, "%s?", prelink);
	}
	if (asprintf_size == -1) {
		return NULL;
	}
	return prlink;
}

static PyObject *
SearchPagination_render(PaginationObject *self) {
	//printf("%d %d %d %d %s", self->totalResult, self->pageLinks, self->rowsPerPage, self->current, self->prelink);
	PaginationResult * result;

	char *startHTML = "<div class=\"paginator\">";
	char *endHTML = "</div>";
	char *previousHTML = NULL;
	char *nextHTML = NULL;
	char *rangeHTML = NULL;
	char *prelink = NULL;
	char *tmp = NULL;
	int asprintf_size;
	PyObject *res;

	result = Pagination_calc(self);
	if (result == NULL) {
		goto on_mem_error;
	}

	//printf("%d %d %d %s", result->totalResult, result->pageCount, result->current, result->prelink);

	if (result->pageCount < 2) {
		if ((asprintf(&tmp,"%s%s",startHTML, endHTML)) == -1) {
			tmp = NULL;
			goto on_mem_error;
		}
		res = PyString_FromString(tmp);
		PyMem_Free(result->prelink);
		PyMem_Free(result);
		free(tmp);
		return res;
	}

	if ((prelink = preparePreLink(result->prelink)) == NULL) {
		goto on_mem_error;
	}

	if (result->previous > 0) {
		asprintf_size = asprintf(&previousHTML, "<a href=\"%spage=%d\" class=\"paginator-previous\">Previous</a>", prelink, result->previous);
	} else {
		asprintf_size = asprintf(&previousHTML, "%s", "");
	}
	if (asprintf_size == -1) {
		previousHTML = NULL;
		goto on_mem_error;
	}

	if(result->endPage > result->startPage) {
		char *className, *extraClassName;
		rangeHTML = PyMem_Malloc((snprintf(NULL, 0, "<a href=\"%spage=%d\" class=\"%s%s\">%d</a>",
									prelink, result->endPage,
									"paginator-current",
									" paginator-page-first", result->endPage) * (result->endPage - result->startPage)) + 1);
		if (rangeHTML == NULL) {
			goto on_mem_error;
		}
		for(int i = result->startPage; i <= result->endPage; i++) {
			if(i == result->current) {
				className = strdup("paginator-current");
			} else {
				className = strdup("paginator-page");
			}
			if (className == NULL) {
				goto on_mem_error;
			}
			if (i == result->startPage) {
				extraClassName = strdup(" paginator-page-first");
			} else if (i == result->endPage) {
				extraClassName = strdup(" paginator-page-last");
			} else {
				extraClassName = strdup("");
			}
			if (extraClassName == NULL) {
				free(className);
				goto on_mem_error;
			}
			asprintf_size = asprintf(&tmp, "<a href=\"%spage=%d\" class=\"%s%s\">%d</a>", prelink, i, className, extraClassName, i);
			if (asprintf_size == -1) {
				free(className);
				free(extraClassName);
				tmp = NULL;
				goto on_mem_error;
			}
			strcat(rangeHTML, tmp);
			free(className);
			free(extraClassName);
			free(tmp);
		}
	} else {
		if ((asprintf(&rangeHTML,"%s", "")) == -1) {
			rangeHTML = NULL;
			goto on_mem_error;
		}
	}

	if (result->next > 0) {
		asprintf_size = asprintf(&nextHTML, "<a href=\"%spage=%d\" class=\"paginator-next\">Next</a>", prelink, result->next);
	} else {
		asprintf_size = asprintf(&nextHTML, "%s", "");
	}
	if (asprintf_size == -1) {
		nextHTML = NULL;
		goto on_mem_error;
	}

	if ((asprintf(&tmp, "%s%s%s%s%s", startHTML, previousHTML, rangeHTML, nextHTML, endHTML)) == -1) {
		tmp = NULL;
		goto on_mem_error;
	}

	res = PyString_FromString(tmp);

	free(prelink);
	PyMem_Free(result->prelink);
	PyMem_Free(result);
	free(previousHTML);
	free(nextHTML);
	PyMem_Free(rangeHTML);
	free(tmp);

	return res;

on_mem_error:
	/* clean up */
	if (tmp) {
		free(tmp);
	}
	if (prelink) {
		free(prelink);
	}
	if (result) {
		PyMem_Free(result->prelink);
		PyMem_Free(result);
	}
	if (previousHTML) {
		free(previousHTML);
	}
	if (rangeHTML) {
		PyMem_Free(rangeHTML);
	}
	if (nextHTML) {
		free(nextHTML);
	}
	PyErr_SetString(PyExc_MemoryError, "pagination.SearchPagination_render");
	return NULL;
}

/**
 * Method to bind to class
 */
static PyMethodDef SearchPagination_methods[] = {
	{ "render", (PyCFunction) SearchPagination_render, METH_NOARGS, "render pagination" },
	{ NULL, NULL } /* sentinel */
};

static PyTypeObject SearchPaginationtype = {
	PyVarObject_HEAD_INIT(NULL, 0) "_pagination.pagination", /*tp_name*/
	sizeof(PaginationObject), /*tp_size*/
	0, /*tp_itemsize*/
	/* methods */
	Pagination_dealloc, /*tp_dealloc*/
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
	SearchPagination_methods, /* tp_methods */
	NULL, /* tp_members */
	NULL, /* tp_getset */
};

static struct PyMethodDef Pagination_functions[] = {
	{ "SearchPaginator", (PyCFunction) SearchPagination_new, METH_VARARGS | METH_KEYWORDS, "New SearchPagination" },
	{ "ItemPaginator", (PyCFunction) SearchPagination_new, METH_VARARGS | METH_KEYWORDS, "New ItemPagination" },
	{ NULL, NULL } /* Sentinel */
};


PyMODINIT_FUNC initpagination(void) {
	PyObject *m;

	if (PyType_Ready(&SearchPaginationtype) < 0) {
		return;
	}
	/* class and functions to modules */
	m = Py_InitModule("pagination", Pagination_functions);

	if (m == NULL) {
		return;
	}

	Py_INCREF(&SearchPaginationtype);
}
