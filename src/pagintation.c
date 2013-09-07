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

	result = malloc(sizeof(PaginationResult));
	/* initialize values */
	asprintf(&result->prelink, "%s", self->prelink);
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
	int len;
	len = strlen(prelink);
	if (strstr(prelink, "?") != NULL) {
		if (prelink[len-1] != '?' && prelink[len-1] != '&') {
			asprintf(&prlink, "%s&", prelink);
		} else {
			asprintf(&prlink, "%s", prelink);
		}
	} else {
		asprintf(&prlink, "%s?", prelink);
	}
	return prlink;
}

static PyObject *
SearchPagination_render(PaginationObject *self) {
	//printf("%d %d %d %d %s", self->totalResult, self->pageLinks, self->rowsPerPage, self->current, self->prelink);
	PaginationResult * result;

	char *html = NULL;
	char *prelink = NULL;
	char *tmp = NULL;
	html = malloc(1500);

	PyObject *res;



	if (html == NULL)
		return NULL;

	result = Pagination_calc(self);

	//printf("%d %d %d %s", result->totalResult, result->pageCount, result->current, result->prelink);

	/* find better than strcat */
	strcat(html, "<div class=\"paginator\">");

	if (result->pageCount < 2) {
		strcat(html, "</div>");
		res = PyString_FromStringAndSize(html, strlen(html));
		free(result);
		free(html);
		return res;
	}

	prelink = preparePreLink(result->prelink);

	if (result->previous > 0) {
		strcat(html, "<a href=\"");
		strcat(html, prelink);
		strcat(html, "page=");
		asprintf(&tmp, "%d", result->previous);
		strcat(html, tmp);
		free(tmp);
		strcat(html, "\" class=\"paginator-previous\">");
		strcat(html, "Previous</a>");
	}

	if(result->endPage > result->startPage) {
		char *className;
		for(int i = result->startPage; i <= result->endPage; i++) {
			/* enough to keep the total max length */
			className = malloc(40);
			if(i == result->current) {
				strcat(className, "paginator-current");
			} else {
				strcat(className, "paginator-page");
			}

			if (i == result->startPage) {
				strcat(className, " paginator-page-first");
			} else if (i == result->endPage) {
				strcat(className, " paginator-page-last");
			}

			asprintf(&tmp, "<a href=\"%spage=%d\" class=\"%s\">%d</a>", prelink, i, className, i);
			strcat(html, tmp);
			free(className);
			free(tmp);
		}
	}

	if (result->next > 0) {
		strcat(html, "<a href=\"");
		strcat(html, prelink);
		strcat(html, "page=");
		asprintf(&tmp, "%d", result->next);
		strcat(html, tmp);
		free(tmp);
		strcat(html, "\" class=\"paginator-next\">Next</a>");
	}
	strcat(html, "</div>");

	res = PyString_FromStringAndSize(html, strlen(html));

	free(prelink);
	free(result->prelink);
	free(result);
	free(html);

	return res;
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
