import unittest
import pagination
import time

class PaginationTest(unittest.TestCase):
    def test_base(self):
        t = time.time()
        max = 10000
        for i in range(1, max):
            p = pagination.SearchPaginator(1000,
                                            prelink= '/test?go=3',
                                            current = 1,
                                            pageLinks = 5,
                                            rowsPerPage = 10,
                                            )
            self.assertEqual(p.render(), '<div class="paginator"><a href="/test?go=3&page=1" class="paginator-current paginator-page-first">1</a><a href="/test?go=3&page=2" class="paginator-page">2</a><a href="/test?go=3&page=3" class="paginator-page">3</a><a href="/test?go=3&page=4" class="paginator-page">4</a><a href="/test?go=3&page=5" class="paginator-page">5</a><a href="/test?go=3&page=6" class="paginator-page paginator-page-last">6</a><a href="/test?go=3&page=2" class="paginator-next">Next</a></div>')
         
        p = pagination.SearchPaginator(1000,
                                            prelink= '/test?go=3',
                                            current = 5,
                                            pageLinks = 5,
                                            rowsPerPage = 10,
                                            )
        for i in range(1, max):
            self.assertEqual(p.render(), '<div class="paginator"><a href="/test?go=3&page=4" class="paginator-previous">Previous</a><a href="/test?go=3&page=3" class="paginator-page paginator-page-first">3</a><a href="/test?go=3&page=4" class="paginator-page">4</a><a href="/test?go=3&page=5" class="paginator-current">5</a><a href="/test?go=3&page=6" class="paginator-page">6</a><a href="/test?go=3&page=7" class="paginator-page paginator-page-last">7</a><a href="/test?go=3&page=6" class="paginator-next">Next</a></div>')
     
        print time.time() - t