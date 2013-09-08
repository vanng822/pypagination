import unittest
import pagination
import time

class PaginationTest(unittest.TestCase):
    def test_render_search(self):
        t = time.time()
        max = 10000
        for i in range(1, max):
            p = pagination.Paginator(1000,
                                    prelink= '/test?go=3',
                                    current = 1,
                                    pageLinks = 5,
                                    rowsPerPage = 10,
                                    )
            self.assertEqual(p.render(), '<div class="paginator"> <a href="/test?go=3&page=1" class="paginator-current paginator-page-first">1</a><a href="/test?go=3&page=2" class="paginator-page">2</a><a href="/test?go=3&page=3" class="paginator-page">3</a><a href="/test?go=3&page=4" class="paginator-page">4</a><a href="/test?go=3&page=5" class="paginator-page">5</a><a href="/test?go=3&page=6" class="paginator-page paginator-page-last">6</a><a href="/test?go=3&page=2" class="paginator-next">Next</a></div>')
        
        print time.time() - t
         
        p = pagination.Paginator(1000,
                                prelink= '/test?go=3',
                                current = 5,
                                pageLinks = 5,
                                rowsPerPage = 10,
                                )
        
        self.assertEqual(p.render(), '<div class="paginator"><a href="/test?go=3&page=4" class="paginator-previous">Previous</a><a href="/test?go=3&page=3" class="paginator-page paginator-page-first">3</a><a href="/test?go=3&page=4" class="paginator-page">4</a><a href="/test?go=3&page=5" class="paginator-current">5</a><a href="/test?go=3&page=6" class="paginator-page">6</a><a href="/test?go=3&page=7" class="paginator-page paginator-page-last">7</a><a href="/test?go=3&page=6" class="paginator-next">Next</a></div>')
     
    def test_get_pagination_data(self):
        p = pagination.Paginator(1000,
                                prelink= '/test?go=3',
                                current = 5,
                                pageLinks = 5,
                                rowsPerPage = 10,
                                )
        self.assertEquals(p.getPaginationData(), {'toResult': 50, 'last': 100, 'endPage': 7, 'pageCount': 100, 'totalResult': 1000, 'next': 6, 'current': 5, 'startPage': 3, 'prelink': '/test?go=3&', 'fromResult': 41, 'previous': 4, 'first': 1})
        
        
    def test_render_item(self):
        t = time.time()
        max = 10000
        for i in range(1, max):
            p = pagination.Paginator(98,
                                    prelink= '/',
                                    current = 5,
                                    pageLinks = 5,
                                    )
            self.assertEqual(p.render(pattern='item'), '<div class="paginator"><span class="paginator-current-report">Results 41 - 50 of 98</span><a href="/?page=1" class="paginator-first">First</a><a href="/?page=4" class="paginator-previous">Previous</a><a href="/?page=6" class="paginator-next">Next</a><a href="/?page=10" class="paginator-last">Last</a></div>')
        
        print time.time() - t
        
        p = pagination.Paginator(98,
                                prelink= '/',
                                pageLinks = 5,
                                current = 1,
                                )
        
        self.assertEqual(p.render(pattern='item'), '<div class="paginator"><span class="paginator-current-report">Results 1 - 10 of 98</span><span class="paginator-first">First</span><span class="paginator-previous">Previous</span><a href="/?page=2" class="paginator-next">Next</a><a href="/?page=10" class="paginator-last">Last</a></div>')
     
        p = pagination.Paginator(98,
                                prelink= '/',
                                pageLinks = 5,
                                current = 10,
                                )
        
        self.assertEqual(p.render(pattern='item'), '<div class="paginator"><span class="paginator-current-report">Results 91 - 98 of 98</span><a href="/?page=1" class="paginator-first">First</a><a href="/?page=9" class="paginator-previous">Previous</a><span class="paginator-next">Next</span><span class="paginator-last">Last</span></div>')
     
     
     
        