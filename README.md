pypagination
============

Pagination in CPython


# Example
	import pagination
	# total 1000 items in result
	p = pagination.Paginator(1000,
                                    prelink= '/test?go=3',
                                    current = 1,
                                    pageLinks = 5,
                                    rowsPerPage = 10,
                                    )
    print p.render()
    # OR
    print p.render(pattern="item")
